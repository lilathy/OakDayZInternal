// kdmapper using Asusgio3 driver
#include "include/kdmapper.hpp"
#include <Windows.h>
#include <iostream>

#include "include/utils.hpp"
#include "include/asusgio3_driver.hpp"
#include "include/nt.hpp"
#include "include/portable_executable.hpp"

void RelocateImageByDelta_Asusgio3(portable_executable::vec_relocs relocs, const ULONG64 delta) {
	for (const auto& current_reloc : relocs) {
		for (auto i = 0u; i < current_reloc.count; ++i) {
			const uint16_t type = current_reloc.item[i] >> 12;
			const uint16_t offset = current_reloc.item[i] & 0xFFF;

			if (type == IMAGE_REL_BASED_DIR64)
				*reinterpret_cast<ULONG64*>(current_reloc.address + offset) += delta;
		}
	}
}

bool FixSecurityCookie_Asusgio3(void* local_image, ULONG64 kernel_image_base)
{
	auto headers = portable_executable::GetNtHeaders(local_image);
	if (!headers)
		return false;

	auto load_config_directory = headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress;
	if (!load_config_directory)
	{
		kdmLog(L"[+] Load config directory wasn't found, probably StackCookie not defined, fix cookie skipped" << std::endl);
		return true;
	}

	auto load_config_struct = (PIMAGE_LOAD_CONFIG_DIRECTORY)((uintptr_t)local_image + load_config_directory);
	auto stack_cookie = load_config_struct->SecurityCookie;
	if (!stack_cookie)
	{
		kdmLog(L"[+] StackCookie not defined, fix cookie skipped" << std::endl);
		return true;
	}

	stack_cookie = stack_cookie - (uintptr_t)kernel_image_base + (uintptr_t)local_image;

	if (*(uintptr_t*)(stack_cookie) != 0x2B992DDFA232) {
		kdmLog(L"[-] StackCookie already fixed!? this probably wrong" << std::endl);
		return false;
	}

	kdmLog(L"[+] Fixing stack cookie" << std::endl);

	auto new_cookie = 0x2B992DDFA232 ^ GetCurrentProcessId() ^ GetCurrentThreadId();
	if (new_cookie == 0x2B992DDFA232)
		new_cookie = 0x2B992DDFA233;

	*(uintptr_t*)(stack_cookie) = new_cookie;
	return true;
}

bool ResolveImports_Asusgio3(portable_executable::vec_imports imports) {
	for (const auto& current_import : imports) {
		ULONG64 Module = kdmUtils::GetKernelModuleAddress(current_import.module_name);
		if (!Module) {
#if !defined(DISABLE_OUTPUT)
			std::cout << "[-] Dependency " << current_import.module_name << " wasn't found" << std::endl;
#endif
			return false;
		}

		for (auto& current_function_data : current_import.function_datas) {
			ULONG64 function_address = asusgio3_driver::GetKernelModuleExport(Module, current_function_data.name);

			if (!function_address) {
				if (Module != asusgio3_driver::ntoskrnlAddr) {
					function_address = asusgio3_driver::GetKernelModuleExport(asusgio3_driver::ntoskrnlAddr, current_function_data.name);
					if (!function_address) {
#if !defined(DISABLE_OUTPUT)
						std::cout << "[-] Failed to resolve import " << current_function_data.name << " (" << current_import.module_name << ")" << std::endl;
#endif
						return false;
					}
				}
			}

			*current_function_data.address = function_address;
		}
	}

	return true;
}

namespace kdmapper_asusgio3 {

ULONG64 MapDriver(BYTE* data, ULONG64 param1, ULONG64 param2, bool free, bool destroyHeader, kdmapper::AllocationMode mode, bool PassAllocationAddressAsFirstParam, kdmapper::mapCallback callback, NTSTATUS* exitCode) {

	const PIMAGE_NT_HEADERS64 nt_headers = portable_executable::GetNtHeaders(data);

	if (!nt_headers) {
		kdmLog(L"[-] Invalid format of PE image" << std::endl);
		return 0;
	}

	if (nt_headers->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
		kdmLog(L"[-] Image is not 64 bit" << std::endl);
		return 0;
	}

	ULONG32 image_size = nt_headers->OptionalHeader.SizeOfImage;

	void* local_image_base = VirtualAlloc(nullptr, image_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!local_image_base)
		return 0;

	DWORD TotalVirtualHeaderSize = (IMAGE_FIRST_SECTION(nt_headers))->VirtualAddress;
	image_size = image_size - (destroyHeader ? TotalVirtualHeaderSize : 0);

	ULONG64 kernel_image_base = 0;
	if (mode == kdmapper::AllocationMode::AllocateIndependentPages)
	{
		kernel_image_base = asusgio3_driver::MmAllocateIndependentPagesEx(image_size);
	}
	else {
		kernel_image_base = asusgio3_driver::AllocatePool(nt::POOL_TYPE::NonPagedPool, image_size);
	}

	if (!kernel_image_base) {
		kdmLog(L"[-] Failed to allocate remote image in kernel" << std::endl);

		VirtualFree(local_image_base, 0, MEM_RELEASE);
		return 0;
	}

	do {
		kdmLog(L"[+] Image base has been allocated at 0x" << reinterpret_cast<void*>(kernel_image_base) << std::endl);

		memcpy(local_image_base, data, nt_headers->OptionalHeader.SizeOfHeaders);

		const PIMAGE_SECTION_HEADER current_image_section = IMAGE_FIRST_SECTION(nt_headers);

		for (auto i = 0; i < nt_headers->FileHeader.NumberOfSections; ++i) {
			if ((current_image_section[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) > 0)
				continue;
			auto local_section = reinterpret_cast<void*>(reinterpret_cast<ULONG64>(local_image_base) + current_image_section[i].VirtualAddress);
			memcpy(local_section, reinterpret_cast<void*>(reinterpret_cast<ULONG64>(data) + current_image_section[i].PointerToRawData), current_image_section[i].SizeOfRawData);
		}

		ULONG64 realBase = kernel_image_base;
		if (destroyHeader) {
			kernel_image_base -= TotalVirtualHeaderSize;
			kdmLog(L"[+] Skipped 0x" << std::hex << TotalVirtualHeaderSize << L" bytes of PE Header" << std::endl);
		}

		RelocateImageByDelta_Asusgio3(portable_executable::GetRelocs(local_image_base), kernel_image_base - nt_headers->OptionalHeader.ImageBase);

		if (!FixSecurityCookie_Asusgio3(local_image_base, kernel_image_base))
		{
			kdmLog(L"[-] Failed to fix cookie" << std::endl);
			return 0;
		}

		if (!ResolveImports_Asusgio3(portable_executable::GetImports(local_image_base))) {
			kdmLog(L"[-] Failed to resolve imports" << std::endl);
			kernel_image_base = realBase;
			break;
		}

		if (!asusgio3_driver::WriteMemory(realBase, (PVOID)((uintptr_t)local_image_base + (destroyHeader ? TotalVirtualHeaderSize : 0)), image_size)) {
			kdmLog(L"[-] Failed to write local image to remote image" << std::endl);
			kernel_image_base = realBase;
			break;
		}

		if (mode == kdmapper::AllocationMode::AllocateIndependentPages)
		{
			for (int i = 0; i < nt_headers->FileHeader.NumberOfSections; i++) {
				auto sec = &IMAGE_FIRST_SECTION(nt_headers)[i];
				uintptr_t secAddr = kernel_image_base + sec->VirtualAddress;
				uint32_t secSize = sec->Misc.VirtualSize;

				if (secSize <= 0)
					continue;

				ULONG prot = PAGE_READONLY;

				if (sec->Characteristics & IMAGE_SCN_MEM_EXECUTE) {
					prot = (sec->Characteristics & IMAGE_SCN_MEM_WRITE) ?
						PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
				}
				else if (sec->Characteristics & IMAGE_SCN_MEM_WRITE) {
					prot = PAGE_READWRITE;
				}

				asusgio3_driver::MmSetPageProtection(secAddr, secSize, prot);
			}
		}

		const ULONG64 address_of_entry_point = kernel_image_base + nt_headers->OptionalHeader.AddressOfEntryPoint;

		kdmLog(L"[<] Calling DriverEntry 0x" << reinterpret_cast<void*>(address_of_entry_point) << std::endl);

		if (callback) {
			if (!callback(&param1, &param2, realBase, image_size)) {
				kdmLog(L"[-] Callback returns false, failed!" << std::endl);
				kernel_image_base = realBase;
				break;
			}
		}

		NTSTATUS status = 0;
		if (!asusgio3_driver::CallKernelFunction(&status, address_of_entry_point, (PassAllocationAddressAsFirstParam ? realBase : param1), param2)) {
			kdmLog(L"[-] Failed to call driver entry" << std::endl);
			kernel_image_base = realBase;
			break;
		}

		if (exitCode)
			*exitCode = status;

		kdmLog(L"[+] DriverEntry returned 0x" << std::hex << status << std::endl);

		if (free) {
			kdmLog(L"[+] Freeing memory" << std::endl);
			bool free_status = false;

			if (mode == kdmapper::AllocationMode::AllocateIndependentPages)
			{
				free_status = asusgio3_driver::MmFreeIndependentPages(realBase, image_size);
			}
			else {
				free_status = asusgio3_driver::FreePool(realBase);
			}

			if (free_status) {
				kdmLog(L"[+] Memory has been released" << std::endl);
			}
			else {
				kdmLog(L"[-] WARNING: Failed to free memory!" << std::endl);
			}
		}

		VirtualFree(local_image_base, 0, MEM_RELEASE);
		return realBase;

	} while (false);

	VirtualFree(local_image_base, 0, MEM_RELEASE);

	kdmLog(L"[+] Freeing memory" << std::endl);
	bool free_status = false;

	if (mode == kdmapper::AllocationMode::AllocateIndependentPages)
	{
		free_status = asusgio3_driver::MmFreeIndependentPages(kernel_image_base, image_size);
	}
	else {
		free_status = asusgio3_driver::FreePool(kernel_image_base);
	}

	return 0;
}

} // namespace kdmapper_asusgio3
