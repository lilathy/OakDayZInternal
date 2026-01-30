#include "driver.h"


extern VOID Log(const char* Format, ...);


NTSTATUS FindProcessByName(const char* ProcessName, PEPROCESS* Process, PHANDLE ProcessId)
{
    if (!ProcessName || !Process || !ProcessId)
        return STATUS_INVALID_PARAMETER;
    
    *Process = NULL;
    *ProcessId = NULL;
    
    int foundCount = 0;
    PEPROCESS bestProcess = NULL;
    HANDLE bestPid = NULL;
    ULONG64 bestBase = 0;
    
    
    for (ULONG pid = 4; pid < 65536; pid += 4)
    {
        PEPROCESS process = NULL;
        NTSTATUS status = PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)pid, &process);
        
        if (NT_SUCCESS(status))
        {
            
            PCHAR imageName = (PCHAR)PsGetProcessImageFileName(process);
            
            if (imageName && imageName[0] != '\0')
            {
                
                if ((imageName[0] == 'D' || imageName[0] == 'd') &&
                    (imageName[1] == 'a' || imageName[1] == 'A') &&
                    (imageName[2] == 'y' || imageName[2] == 'Y'))
                {
                    ULONG64 base = (ULONG64)PsGetProcessSectionBaseAddress(process);
                    Log("Found process: '%s' (PID %d, Base: 0x%llX)", imageName, pid, base);
                    foundCount++;
                }
                
                
                if (_stricmp(imageName, ProcessName) == 0)
                {
                    ULONG64 base = (ULONG64)PsGetProcessSectionBaseAddress(process);
                    Log("MATCH: '%s' (PID %d, Base: 0x%llX)", imageName, pid, base);
                    
                    
                    if (bestProcess)
                        ObDereferenceObject(bestProcess);
                    
                    bestProcess = process;
                    bestPid = (HANDLE)(ULONG_PTR)pid;
                    bestBase = base;
                    continue;  
                }
            }
            
            ObDereferenceObject(process);
        }
    }
    
    if (bestProcess)
    {
        Log("Selected: PID %d with base 0x%llX", (ULONG)(ULONG_PTR)bestPid, bestBase);
        *Process = bestProcess;
        *ProcessId = bestPid;
        return STATUS_SUCCESS;
    }
    
    Log("Scan complete. Found %d Day* processes, no suitable match for '%s'", foundCount, ProcessName);
    return STATUS_NOT_FOUND;
}


NTSTATUS AttachToProcess(ULONG ProcessId)
{
    if (ProcessId == 0)
        return STATUS_INVALID_PARAMETER;
    
    
    if (g_State.TargetProcess)
    {
        ObDereferenceObject(g_State.TargetProcess);
        g_State.TargetProcess = NULL;
    }
    
    PEPROCESS process = NULL;
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)ProcessId, &process);
    
    if (!NT_SUCCESS(status))
    {
        Log("Failed to find process %d: 0x%X", ProcessId, status);
        return status;
    }
    
    g_State.TargetProcess = process;
    g_State.TargetProcessId = (HANDLE)(ULONG_PTR)ProcessId;
    g_State.Attached = TRUE;
    
    Log("Attached to process %d", ProcessId);
    return STATUS_SUCCESS;
}


ULONG64 GetProcessBaseAddress(PEPROCESS Process)
{
    if (!Process)
        return 0;
    
    return (ULONG64)PsGetProcessSectionBaseAddress(Process);
}
