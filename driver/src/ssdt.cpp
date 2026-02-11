#include "driver.h"


extern VOID Log(const char* Format, ...);


#pragma pack(push, 1)
typedef struct _HOOK_TRAMPOLINE {
    UCHAR OriginalBytes[14];      
    UCHAR JumpBack[14];           
} HOOK_TRAMPOLINE, *PHOOK_TRAMPOLINE;
#pragma pack(pop)


typedef NTSTATUS(*fnMmCopyVirtualMemory)(
    PEPROCESS SourceProcess,
    PVOID SourceAddress,
    PEPROCESS TargetProcess,
    PVOID TargetAddress,
    SIZE_T BufferSize,
    KPROCESSOR_MODE PreviousMode,
    PSIZE_T ReturnSize
);

static fnMmCopyVirtualMemory g_OriginalMmCopy = NULL;
static PHOOK_TRAMPOLINE g_Trampoline = NULL;
static PVOID g_HookAddress = NULL;
static UCHAR g_OriginalBytes[14] = { 0 };
static BOOLEAN g_HookInstalled = FALSE;


static KIRQL DisableWriteProtection()
{
    KIRQL irql = KeRaiseIrqlToDpcLevel();
    ULONG64 cr0 = __readcr0();
    cr0 &= ~0x10000;
    __writecr0(cr0);
    _disable();
    return irql;
}

static void EnableWriteProtection(KIRQL irql)
{
    ULONG64 cr0 = __readcr0();
    cr0 |= 0x10000;
    _enable();
    __writecr0(cr0);
    KeLowerIrql(irql);
}


static PCLOAK_REGION FindCloakRegion(ULONG64 Address, SIZE_T Size)
{
    if (g_State.CloakRegionCount == 0) return NULL;
    
    ULONG64 readEnd = Address + Size;
    
    for (ULONG i = 0; i < g_State.CloakRegionCount; i++)
    {
        ULONG64 regionStart = g_State.CloakRegions[i].Address;
        ULONG64 regionEnd = regionStart + g_State.CloakRegions[i].Size;
        
        if (Address < regionEnd && readEnd > regionStart)
        {
            return &g_State.CloakRegions[i];
        }
    }
    
    return NULL;
}


NTSTATUS HookedMmCopyVirtualMemory(
    PEPROCESS SourceProcess,
    PVOID SourceAddress,
    PEPROCESS TargetProcess,
    PVOID TargetAddress,
    SIZE_T BufferSize,
    KPROCESSOR_MODE PreviousMode,
    PSIZE_T ReturnSize)
{
    
    NTSTATUS status = g_OriginalMmCopy(
        SourceProcess, SourceAddress, TargetProcess, TargetAddress,
        BufferSize, PreviousMode, ReturnSize);
    
    if (!NT_SUCCESS(status) || !g_State.CloakEnabled)
        return status;
    
    
    if (SourceProcess != g_State.TargetProcess)
        return status;
    
    
    PCLOAK_REGION region = FindCloakRegion((ULONG64)SourceAddress, BufferSize);
    if (!region)
        return status;
    
    
    ULONG64 readStart = (ULONG64)SourceAddress;
    ULONG64 readEnd = readStart + BufferSize;
    ULONG64 regionStart = region->Address;
    ULONG64 regionEnd = regionStart + region->Size;
    
    ULONG64 overlapStart = (readStart > regionStart) ? readStart : regionStart;
    ULONG64 overlapEnd = (readEnd < regionEnd) ? readEnd : regionEnd;
    
    SIZE_T targetOffset = (SIZE_T)(overlapStart - readStart);
    SIZE_T regionOffset = (SIZE_T)(overlapStart - regionStart);
    SIZE_T overlapSize = (SIZE_T)(overlapEnd - overlapStart);
    
    
    __try
    {
        PVOID originalBytes = (PVOID)region->OriginalBytes;
        if (originalBytes && overlapSize > 0)
        {
            RtlCopyMemory(
                (PUCHAR)TargetAddress + targetOffset,
                (PUCHAR)originalBytes + regionOffset,
                overlapSize
            );
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        
    }
    
    return status;
}


static NTSTATUS CreateTrampoline()
{
    
    g_Trampoline = (PHOOK_TRAMPOLINE)ExAllocatePool2(
        POOL_FLAG_NON_PAGED | POOL_FLAG_UNINITIALIZED,
        sizeof(HOOK_TRAMPOLINE),
        DRIVER_TAG
    );
    
    if (!g_Trampoline)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    
    RtlCopyMemory(g_Trampoline->OriginalBytes, g_HookAddress, 14);
    
    
    g_Trampoline->JumpBack[0] = 0xFF;
    g_Trampoline->JumpBack[1] = 0x25;
    g_Trampoline->JumpBack[2] = 0x00;
    g_Trampoline->JumpBack[3] = 0x00;
    g_Trampoline->JumpBack[4] = 0x00;
    g_Trampoline->JumpBack[5] = 0x00;
    *(PULONG64)&g_Trampoline->JumpBack[6] = (ULONG64)g_HookAddress + 14;
    
    g_OriginalMmCopy = (fnMmCopyVirtualMemory)(PVOID)g_Trampoline->OriginalBytes;
    
    return STATUS_SUCCESS;
}


NTSTATUS InstallSsdtHook()
{
    if (g_HookInstalled)
        return STATUS_SUCCESS;
    
    
    UNICODE_STRING funcName = RTL_CONSTANT_STRING(L"MmCopyVirtualMemory");
    g_HookAddress = MmGetSystemRoutineAddress(&funcName);
    
    if (!g_HookAddress)
    {
        Log("ERROR: MmCopyVirtualMemory not found");
        return STATUS_NOT_FOUND;
    }
    
    Log("MmCopyVirtualMemory at: %p", g_HookAddress);
    
    
    RtlCopyMemory(g_OriginalBytes, g_HookAddress, 14);
    
    
    NTSTATUS status = CreateTrampoline();
    if (!NT_SUCCESS(status))
    {
        Log("ERROR: Failed to create trampoline");
        return status;
    }
    
    
    UCHAR hookBytes[14];
    hookBytes[0] = 0xFF;
    hookBytes[1] = 0x25;
    hookBytes[2] = 0x00;
    hookBytes[3] = 0x00;
    hookBytes[4] = 0x00;
    hookBytes[5] = 0x00;
    *(PULONG64)&hookBytes[6] = (ULONG64)HookedMmCopyVirtualMemory;
    
    
    KIRQL irql = DisableWriteProtection();
    RtlCopyMemory(g_HookAddress, hookBytes, 14);
    EnableWriteProtection(irql);
    
    g_HookInstalled = TRUE;
    Log("Memory cloaking hook installed successfully");
    
    return STATUS_SUCCESS;
}


NTSTATUS RemoveSsdtHook()
{
    if (!g_HookInstalled)
        return STATUS_SUCCESS;
    
    
    KIRQL irql = DisableWriteProtection();
    RtlCopyMemory(g_HookAddress, g_OriginalBytes, 14);
    EnableWriteProtection(irql);
    
    
    if (g_Trampoline)
    {
        ExFreePoolWithTag(g_Trampoline, DRIVER_TAG);
        g_Trampoline = NULL;
    }
    
    g_HookInstalled = FALSE;
    Log("Memory cloaking hook removed");
    
    return STATUS_SUCCESS;
}


NTSTATUS EnableMemoryCloaking()
{
    if (g_State.CloakEnabled)
        return STATUS_SUCCESS;
    
    NTSTATUS status = InstallSsdtHook();
    if (NT_SUCCESS(status))
    {
        g_State.CloakEnabled = TRUE;
        Log("Memory cloaking ENABLED");
    }
    
    return status;
}

NTSTATUS DisableMemoryCloaking()
{
    if (!g_State.CloakEnabled)
        return STATUS_SUCCESS;
    
    NTSTATUS status = RemoveSsdtHook();
    g_State.CloakEnabled = FALSE;
    
    Log("Memory cloaking DISABLED");
    return status;
}

