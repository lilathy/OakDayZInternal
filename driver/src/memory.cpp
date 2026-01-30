#include "driver.h"


extern VOID Log(const char* Format, ...);


NTSTATUS ReadProcessMemory(ULONG64 Address, PVOID Buffer, SIZE_T Size)
{
    if (!g_State.TargetProcess || !Address || !Buffer || !Size)
        return STATUS_INVALID_PARAMETER;
    
    SIZE_T bytesRead = 0;
    
    NTSTATUS status = MmCopyVirtualMemory(
        g_State.TargetProcess,  
        (PVOID)Address,         
        PsGetCurrentProcess(),  
        Buffer,                 
        Size,                   
        KernelMode,             
        &bytesRead              
    );
    
    return status;
}


NTSTATUS WriteProcessMemory(ULONG64 Address, PVOID Buffer, SIZE_T Size)
{
    if (!g_State.TargetProcess || !Address || !Buffer || !Size)
        return STATUS_INVALID_PARAMETER;
    
    SIZE_T bytesWritten = 0;
    
    NTSTATUS status = MmCopyVirtualMemory(
        PsGetCurrentProcess(),  
        Buffer,                 
        g_State.TargetProcess,  
        (PVOID)Address,         
        Size,                   
        KernelMode,             
        &bytesWritten           
    );
    
    return status;
}


NTSTATUS AddCloakRegion(ULONG64 Address, ULONG64 Size, PVOID OriginalBytes)
{
    if (g_State.CloakRegionCount >= 32)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    if (!Address || !Size || !OriginalBytes)
        return STATUS_INVALID_PARAMETER;
    
    ULONG idx = g_State.CloakRegionCount++;
    g_State.CloakRegions[idx].Address = Address;
    g_State.CloakRegions[idx].Size = Size;
    g_State.CloakRegions[idx].OriginalBytes = (ULONG64)OriginalBytes;
    
    Log("Added cloak region: 0x%llX, size: %llu", Address, Size);
    return STATUS_SUCCESS;
}
