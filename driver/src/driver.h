#pragma once
#include <ntifs.h>
#include <ntddk.h>
#include <wdm.h>
#include <intrin.h>
#include <ntimage.h>


#define DRIVER_TAG 'kaoD'  
#define TARGET_PROCESS_NAME "DayZ_x64.exe"


#define IOCTL_INIT              0x800
#define IOCTL_ATTACH_PROCESS    0x801
#define IOCTL_READ_MEMORY       0x802
#define IOCTL_WRITE_MEMORY      0x803
#define IOCTL_MAP_DLL           0x804
#define IOCTL_ENABLE_CLOAK      0x805
#define IOCTL_ADD_CLOAK_REGION  0x806
#define IOCTL_UNLOAD            0x807


typedef struct _ATTACH_REQUEST {
    ULONG ProcessId;
} ATTACH_REQUEST, *PATTACH_REQUEST;

typedef struct _MEMORY_REQUEST {
    ULONG64 Address;
    ULONG64 Buffer;
    ULONG64 Size;
} MEMORY_REQUEST, *PMEMORY_REQUEST;

typedef struct _MAP_DLL_REQUEST {
    ULONG64 DllBuffer;
    ULONG64 DllSize;
    ULONG64 MappedBase;
} MAP_DLL_REQUEST, *PMAP_DLL_REQUEST;

typedef struct _CLOAK_REGION {
    ULONG64 Address;
    ULONG64 Size;
    ULONG64 OriginalBytes;
} CLOAK_REGION, *PCLOAK_REGION;


typedef struct _DRIVER_STATE {
    PEPROCESS TargetProcess;
    HANDLE TargetProcessId;
    BOOLEAN Attached;
    BOOLEAN CloakEnabled;
    
    CLOAK_REGION CloakRegions[32];
    ULONG CloakRegionCount;
    
    PVOID OriginalNtReadVirtualMemory;
    PVOID OriginalMmCopyVirtualMemory;
    
    
    ULONG64 InjectedBase;
} DRIVER_STATE, *PDRIVER_STATE;

extern DRIVER_STATE g_State;


extern "C" {
    NTKERNELAPI NTSTATUS MmCopyVirtualMemory(
        PEPROCESS SourceProcess,
        PVOID SourceAddress,
        PEPROCESS TargetProcess,
        PVOID TargetAddress,
        SIZE_T BufferSize,
        KPROCESSOR_MODE PreviousMode,
        PSIZE_T ReturnSize
    );
    
    NTKERNELAPI PPEB PsGetProcessPeb(PEPROCESS Process);
    NTKERNELAPI PVOID PsGetProcessSectionBaseAddress(PEPROCESS Process);
    NTKERNELAPI PCHAR PsGetProcessImageFileName(PEPROCESS Process);
}


typedef struct _PEB_LDR_DATA_FULL {
    ULONG Length;
    BOOLEAN Initialized;
    PVOID SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA_FULL, *PPEB_LDR_DATA_FULL;

typedef struct _LDR_DATA_TABLE_ENTRY_FULL {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    LIST_ENTRY HashLinks;
    ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY_FULL, *PLDR_DATA_TABLE_ENTRY_FULL;


NTSTATUS FindProcessByName(const char* ProcessName, PEPROCESS* Process, PHANDLE ProcessId);
NTSTATUS AttachToProcess(ULONG ProcessId);
ULONG64 GetProcessBaseAddress(PEPROCESS Process);


NTSTATUS ReadProcessMemory(ULONG64 Address, PVOID Buffer, SIZE_T Size);
NTSTATUS WriteProcessMemory(ULONG64 Address, PVOID Buffer, SIZE_T Size);


NTSTATUS MapDllToProcess(PVOID DllBuffer, SIZE_T DllSize, PULONG64 MappedBase);
NTSTATUS RegisterHookRegions();
ULONG64 FindModuleBase(PPEB Peb, const char* ModuleName);
ULONG64 GetExportByName(ULONG64 ModuleBase, const char* FuncName);
ULONG64 GetExportByOrdinal(ULONG64 ModuleBase, USHORT Ordinal);


NTSTATUS EnableMemoryCloaking();
NTSTATUS DisableMemoryCloaking();
NTSTATUS AddCloakRegion(ULONG64 Address, ULONG64 Size, PVOID OriginalBytes);
NTSTATUS InstallSsdtHook();
NTSTATUS RemoveSsdtHook();
