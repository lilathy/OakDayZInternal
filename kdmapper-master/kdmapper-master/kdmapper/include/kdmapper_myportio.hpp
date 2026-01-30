#pragma once

#include <Windows.h>
#include "nt.hpp"
#include "kdmapper.hpp"

namespace kdmapper_myportio {
    // Map driver using MyPortIO instead of Intel driver
    ULONG64 MapDriver(
        BYTE* data, 
        ULONG64 param1 = 0, 
        ULONG64 param2 = 0, 
        bool free = false, 
        bool destroyHeader = true, 
        kdmapper::AllocationMode mode = kdmapper::AllocationMode::AllocatePool, 
        bool PassAllocationAddressAsFirstParam = false, 
        kdmapper::mapCallback callback = nullptr, 
        NTSTATUS* exitCode = nullptr
    );
}
