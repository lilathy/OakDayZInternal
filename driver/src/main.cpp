#include "driver.h"
#include <stdarg.h>
#include <stdio.h>
#include <ntstrsafe.h>


DRIVER_STATE g_State = { 0 };


#define CHEAT_DLL_PATH L"\\??\\C:\\oak\\dayz_internal.dll"


VOID LogWrite(const char* msg)
{
    DbgPrintEx(0, 0, "%s", msg);
}

VOID LogInit()
{
}

VOID LogClose()
{
}

VOID Log(const char* Format, ...)
{
    va_list args;
    va_start(args, Format);
    
    char buffer[512];
    RtlStringCbVPrintfA(buffer, sizeof(buffer) - 16, Format, args);
    va_end(args);
    
    
    char finalBuffer[600];
    RtlStringCbPrintfA(finalBuffer, sizeof(finalBuffer), "[oak] %s\r\n", buffer);
    
    
    DbgPrintEx(0, 0, "%s", finalBuffer);
    
    
    if (KeGetCurrentIrql() == PASSIVE_LEVEL)
    {
        LogWrite(finalBuffer);
    }
}


NTSTATUS ReadFileToPool(PUNICODE_STRING FilePath, PVOID* Buffer, PSIZE_T Size)
{
    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, FilePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatus;
    
    NTSTATUS status = ZwOpenFile(
        &fileHandle,
        GENERIC_READ,
        &objAttr,
        &ioStatus,
        FILE_SHARE_READ,
        FILE_SYNCHRONOUS_IO_NONALERT
    );
    
    if (!NT_SUCCESS(status))
    {
        Log("Failed to open file: 0x%X", status);
        return status;
    }
    
    
    FILE_STANDARD_INFORMATION fileInfo;
    status = ZwQueryInformationFile(fileHandle, &ioStatus, &fileInfo, sizeof(fileInfo), FileStandardInformation);
    if (!NT_SUCCESS(status))
    {
        Log("Failed to query file info: 0x%X", status);
        ZwClose(fileHandle);
        return status;
    }
    
    SIZE_T fileSize = (SIZE_T)fileInfo.EndOfFile.QuadPart;
    Log("File size: %llu bytes", fileSize);
    
    
    PVOID buffer = ExAllocatePoolWithTag(NonPagedPool, fileSize, DRIVER_TAG);
    if (!buffer)
    {
        Log("Failed to allocate %llu bytes", fileSize);
        ZwClose(fileHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    
    LARGE_INTEGER byteOffset = { 0 };
    status = ZwReadFile(fileHandle, NULL, NULL, NULL, &ioStatus, buffer, (ULONG)fileSize, &byteOffset, NULL);
    
    ZwClose(fileHandle);
    
    if (!NT_SUCCESS(status))
    {
        Log("Failed to read file: 0x%X", status);
        ExFreePoolWithTag(buffer, DRIVER_TAG);
        return status;
    }
    
    *Buffer = buffer;
    *Size = fileSize;
    
    Log("File read successfully");
    return STATUS_SUCCESS;
}


VOID DoInjection(PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
    
    Log("=== DEBUG: Injection thread started ===");
    Log("Step 1: Waiting 3 seconds...");
    
    LARGE_INTEGER delay;
    delay.QuadPart = -30000000LL; 
    KeDelayExecutionThread(KernelMode, FALSE, &delay);
    
    Log("Step 2: Starting process search...");
    
    
    PEPROCESS process = NULL;
    HANDLE processId = NULL;
    
    for (int i = 0; i < 120; i++)  
    {
        
        for (ULONG pid = 4; pid < 65536; pid += 4)
        {
            PEPROCESS testProc = NULL;
            if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)pid, &testProc)))
            {
                PCHAR imageName = (PCHAR)PsGetProcessImageFileName(testProc);
                if (imageName && _stricmp(imageName, TARGET_PROCESS_NAME) == 0)
                {
                    process = testProc;
                    processId = (HANDLE)(ULONG_PTR)pid;
                    break;  
                }
                ObDereferenceObject(testProc);
            }
        }
        
        if (process) break;
        
        if (i == 0 || i == 30 || i == 60 || i == 90)
        {
            Log("Searching... (%d/120)", i);
        }
        
        delay.QuadPart = -10000000LL; 
        KeDelayExecutionThread(KernelMode, FALSE, &delay);
    }
    
    if (!process)
    {
        Log("ERROR: DayZ not found");
        PsTerminateSystemThread(STATUS_NOT_FOUND);
        return;
    }
    
    g_State.TargetProcess = process;
    g_State.TargetProcessId = processId;
    
    Log("Step 3: Found DayZ PID=%d", (ULONG)(ULONG_PTR)processId);
    
    
    while (PsGetProcessExitStatus(process) != STATUS_PENDING)
    {
        Log("Process is terminating, waiting for fresh instance...");
        ObDereferenceObject(process);
        process = NULL;
        processId = NULL;
        
        
        for (int retry = 0; retry < 60; retry++)  
        {
            delay.QuadPart = -10000000LL; 
            KeDelayExecutionThread(KernelMode, FALSE, &delay);
            
            
            for (ULONG pid = 4; pid < 65536; pid += 4)
            {
                PEPROCESS testProc = NULL;
                if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)pid, &testProc)))
                {
                    PCHAR imageName = (PCHAR)PsGetProcessImageFileName(testProc);
                    if (imageName && _stricmp(imageName, TARGET_PROCESS_NAME) == 0)
                    {
                        if (PsGetProcessExitStatus(testProc) == STATUS_PENDING)
                        {
                            process = testProc;
                            processId = (HANDLE)(ULONG_PTR)pid;
                            break;
                        }
                    }
                    ObDereferenceObject(testProc);
                }
            }
            
            if (process)
            {
                Log("Found fresh process PID=%d", (ULONG)(ULONG_PTR)processId);
                break;
            }
            
            if (retry % 10 == 0)
                Log("Still waiting for DayZ... (%d/60)", retry);
        }
        
        if (!process)
        {
            Log("ERROR: Timeout waiting for DayZ process");
            PsTerminateSystemThread(STATUS_NOT_FOUND);
            return;
        }
    }
    
    Log("Step 4: Waiting 10 seconds for game to fully init...");
    
    delay.QuadPart = -100000000LL; 
    KeDelayExecutionThread(KernelMode, FALSE, &delay);
    
    
    if (PsGetProcessExitStatus(process) != STATUS_PENDING)
    {
        Log("ERROR: Process terminated during wait");
        ObDereferenceObject(process);
        PsTerminateSystemThread(STATUS_PROCESS_IS_TERMINATING);
        return;
    }
    
    
    g_State.TargetProcess = process;
    g_State.TargetProcessId = processId;
    g_State.Attached = TRUE;
    Log("Process verified and attached");
    
    
    Log("Step 5: Reading DLL...");
    UNICODE_STRING dllPath = RTL_CONSTANT_STRING(CHEAT_DLL_PATH);
    PVOID dllBuffer = NULL;
    SIZE_T dllSize = 0;
    
    NTSTATUS status = ReadFileToPool(&dllPath, &dllBuffer, &dllSize);
    if (!NT_SUCCESS(status))
    {
        Log("ERROR: DLL read failed: 0x%X", status);
        PsTerminateSystemThread(status);
        return;
    }
    
    Log("Step 6: DLL loaded (%llu bytes)", dllSize);
    Log("Step 7: Mapping DLL...");
    
    ULONG64 mappedBase = 0;
    status = MapDllToProcess(dllBuffer, dllSize, &mappedBase);
    
    ExFreePoolWithTag(dllBuffer, DRIVER_TAG);
    
    if (!NT_SUCCESS(status))
    {
        Log("ERROR: Mapping failed: 0x%X", status);
        PsTerminateSystemThread(status);
        return;
    }
    
    Log("Step 8: DLL mapped at 0x%llX", mappedBase);
    
    
    Log("Waiting 3 seconds for cheat to initialize...");
    delay.QuadPart = -30000000LL; 
    KeDelayExecutionThread(KernelMode, FALSE, &delay);
    
    
    Log("NOTE: Memory cloaking is disabled for debugging");
    
    Log("=======================================");
    Log("INJECTION COMPLETE!");
    Log("Cheat should now be running.");
    Log("Press INSERT to open menu.");
    Log("=======================================");
    
    
    while (TRUE)
    {
        delay.QuadPart = -100000000LL; 
        KeDelayExecutionThread(KernelMode, FALSE, &delay);
        
        
        if (!g_State.TargetProcess)
        {
            Log("Target process terminated, cleaning up...");
            break;
        }
    }
    
    
    DisableMemoryCloaking();
    LogClose();
    
    PsTerminateSystemThread(STATUS_SUCCESS);
}


#pragma pack(push, 1)
typedef struct _HOOK_REGION_INFO {
    ULONG64 Address;
    ULONG64 Size;
    UCHAR OriginalBytes[32];
    UCHAR Valid;
} HOOK_REGION_INFO, *PHOOK_REGION_INFO;
#pragma pack(pop)


NTSTATUS RegisterHookRegions()
{
    Log("Reading hook regions from file...");
    
    
    UNICODE_STRING filePath = RTL_CONSTANT_STRING(L"\\??\\C:\\oak\\hooks.dat");
    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &filePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatus;
    
    NTSTATUS status = ZwOpenFile(
        &fileHandle,
        GENERIC_READ,
        &objAttr,
        &ioStatus,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_SYNCHRONOUS_IO_NONALERT
    );
    
    if (!NT_SUCCESS(status))
    {
        Log("hooks.dat not found (0x%X) - this is normal if cheat hasn't registered hooks", status);
        return status;
    }
    
    
    FILE_STANDARD_INFORMATION fileInfo;
    status = ZwQueryInformationFile(fileHandle, &ioStatus, &fileInfo, sizeof(fileInfo), FileStandardInformation);
    if (!NT_SUCCESS(status))
    {
        ZwClose(fileHandle);
        return status;
    }
    
    SIZE_T fileSize = (SIZE_T)fileInfo.EndOfFile.QuadPart;
    ULONG numEntries = (ULONG)(fileSize / sizeof(HOOK_REGION_INFO));
    
    Log("Found %lu hook entries", numEntries);
    
    if (numEntries == 0)
    {
        ZwClose(fileHandle);
        return STATUS_SUCCESS;
    }
    
    
    PHOOK_REGION_INFO entries = (PHOOK_REGION_INFO)ExAllocatePoolWithTag(
        NonPagedPool, 
        numEntries * sizeof(HOOK_REGION_INFO), 
        DRIVER_TAG
    );
    
    if (!entries)
    {
        ZwClose(fileHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    
    LARGE_INTEGER byteOffset = { 0 };
    status = ZwReadFile(fileHandle, NULL, NULL, NULL, &ioStatus, 
        entries, (ULONG)(numEntries * sizeof(HOOK_REGION_INFO)), &byteOffset, NULL);
    
    ZwClose(fileHandle);
    
    if (!NT_SUCCESS(status))
    {
        ExFreePoolWithTag(entries, DRIVER_TAG);
        return status;
    }
    
    
    for (ULONG i = 0; i < numEntries && i < 32; i++)
    {
        if (!entries[i].Valid) continue;
        
        
        PVOID origBytes = ExAllocatePoolWithTag(NonPagedPool, 32, DRIVER_TAG);
        if (!origBytes) continue;
        
        RtlCopyMemory(origBytes, entries[i].OriginalBytes, 32);
        
        
        AddCloakRegion(entries[i].Address, entries[i].Size, origBytes);
        
        Log("Registered hook region: 0x%llX (size: %llu)", entries[i].Address, entries[i].Size);
    }
    
    ExFreePoolWithTag(entries, DRIVER_TAG);
    
    Log("Hook regions registered successfully");
    return STATUS_SUCCESS;
}


extern "C" NTSTATUS DriverEntry(
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
)
{
    UNREFERENCED_PARAMETER(Param1);
    UNREFERENCED_PARAMETER(Param2);
    
    
    LogInit();
    
    Log("Oak Driver v1.0 - Starting");
    
    
    RtlZeroMemory(&g_State, sizeof(g_State));
    
    Log("State initialized, creating thread...");
    
    
    HANDLE threadHandle = NULL;
    NTSTATUS status = PsCreateSystemThread(
        &threadHandle,
        THREAD_ALL_ACCESS,
        NULL,
        NULL,
        NULL,
        DoInjection,
        NULL
    );
    
    if (!NT_SUCCESS(status))
    {
        Log("ERROR: Failed to create thread: 0x%X", status);
        LogClose();
        return status;
    }
    
    ZwClose(threadHandle);
    
    Log("Thread created - waiting for DayZ process...");
    return STATUS_SUCCESS;
}
