#include "driver.h"

extern VOID Log(const char* Format, ...);


#pragma pack(push, 1)
typedef struct _SHELLCODE_BLOCK {
    UCHAR Code[64];
    ULONG64 DllBase;
    ULONG64 EntryPoint;
} SHELLCODE_BLOCK;
#pragma pack(pop)


static UCHAR g_Shellcode[] = {
    0x55,                         
    0x48, 0x89, 0xE5,             
    0x48, 0x83, 0xEC, 0x30,       
    0x49, 0x89, 0xCA,             
    0x49, 0x8B, 0x4A, 0x40,       
    0x49, 0x8B, 0x42, 0x48,       
    0xBA, 0x01, 0x00, 0x00, 0x00, 
    0x45, 0x31, 0xC0,             
    0xFF, 0xD0,                   
    0x48, 0x83, 0xC4, 0x30,       
    0x5D,                         
    0x31, 0xC0,                   
    0xC3                          
};


NTSTATUS MapDllToProcess(PVOID DllBuffer, SIZE_T DllSize, PULONG64 MappedBase)
{
    UNREFERENCED_PARAMETER(DllSize);
    
    if (!g_State.TargetProcess || !DllBuffer || !MappedBase)
        return STATUS_INVALID_PARAMETER;
    
    *MappedBase = 0;
    NTSTATUS status;
    
    
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)DllBuffer;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return STATUS_INVALID_IMAGE_FORMAT;
    
    PIMAGE_NT_HEADERS64 nt = (PIMAGE_NT_HEADERS64)((PUCHAR)DllBuffer + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return STATUS_INVALID_IMAGE_FORMAT;
    
    SIZE_T imageSize = nt->OptionalHeader.SizeOfImage;
    Log("Image size: 0x%llX", (ULONG64)imageSize);
    
    
    HANDLE processHandle = NULL;
    status = ObOpenObjectByPointer(g_State.TargetProcess, OBJ_KERNEL_HANDLE, NULL,
        PROCESS_ALL_ACCESS, *PsProcessType, KernelMode, &processHandle);
    if (!NT_SUCCESS(status))
    {
        Log("ERROR: ObOpenObjectByPointer: 0x%X", status);
        return status;
    }
    
    
    PVOID remoteBase = NULL;
    SIZE_T regionSize = imageSize;
    status = ZwAllocateVirtualMemory(processHandle, &remoteBase, 0, &regionSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS(status))
    {
        ZwClose(processHandle);
        Log("ERROR: ZwAllocateVirtualMemory: 0x%X", status);
        return status;
    }
    Log("Allocated at 0x%p", remoteBase);
    
    
    KAPC_STATE apc;
    KeStackAttachProcess(g_State.TargetProcess, &apc);
    
    
    __try { RtlCopyMemory(remoteBase, DllBuffer, nt->OptionalHeader.SizeOfHeaders); }
    __except(EXCEPTION_EXECUTE_HANDLER) { Log("ERROR: Header copy failed"); }
    
    
    PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION(nt);
    for (USHORT i = 0; i < nt->FileHeader.NumberOfSections; i++)
    {
        if (sec[i].SizeOfRawData)
        {
            __try {
                RtlCopyMemory((PUCHAR)remoteBase + sec[i].VirtualAddress,
                    (PUCHAR)DllBuffer + sec[i].PointerToRawData, sec[i].SizeOfRawData);
            } __except(EXCEPTION_EXECUTE_HANDLER) {}
        }
    }
    Log("Sections copied");
    
    
    ULONG64 delta = (ULONG64)remoteBase - nt->OptionalHeader.ImageBase;
    if (delta)
    {
        PIMAGE_DATA_DIRECTORY relocDir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (relocDir->Size)
        {
            PIMAGE_BASE_RELOCATION reloc = (PIMAGE_BASE_RELOCATION)((PUCHAR)remoteBase + relocDir->VirtualAddress);
            __try {
                while (reloc->VirtualAddress && reloc->SizeOfBlock)
                {
                    ULONG cnt = (reloc->SizeOfBlock - 8) / 2;
                    PUSHORT list = (PUSHORT)((PUCHAR)reloc + 8);
                    for (ULONG j = 0; j < cnt; j++)
                    {
                        if ((list[j] >> 12) == IMAGE_REL_BASED_DIR64)
                        {
                            PULONG64 p = (PULONG64)((PUCHAR)remoteBase + reloc->VirtualAddress + (list[j] & 0xFFF));
                            *p += delta;
                        }
                    }
                    reloc = (PIMAGE_BASE_RELOCATION)((PUCHAR)reloc + reloc->SizeOfBlock);
                }
            } __except(EXCEPTION_EXECUTE_HANDLER) {}
        }
        Log("Relocations done (delta=0x%llX)", delta);
    }
    
    
    PPEB peb = PsGetProcessPeb(g_State.TargetProcess);
    PIMAGE_DATA_DIRECTORY impDir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (peb && impDir->Size)
    {
        PIMAGE_IMPORT_DESCRIPTOR imp = (PIMAGE_IMPORT_DESCRIPTOR)((PUCHAR)remoteBase + impDir->VirtualAddress);
        __try {
            while (imp->Name)
            {
                char* modName = (char*)((PUCHAR)remoteBase + imp->Name);
                const char* actualName = modName;
                
                
                if (_strnicmp(modName, "api-ms-win-crt-", 15) == 0)
                    actualName = "ucrtbase.dll";
                else if (_strnicmp(modName, "d3dcompiler", 11) == 0)
                { imp++; continue; }
                
                ULONG64 modBase = FindModuleBase(peb, actualName);
                if (modBase)
                {
                    PIMAGE_THUNK_DATA64 oft = (PIMAGE_THUNK_DATA64)((PUCHAR)remoteBase + imp->OriginalFirstThunk);
                    PIMAGE_THUNK_DATA64 ft = (PIMAGE_THUNK_DATA64)((PUCHAR)remoteBase + imp->FirstThunk);
                    while (oft->u1.AddressOfData)
                    {
                        if (!(oft->u1.Ordinal & IMAGE_ORDINAL_FLAG64))
                        {
                            PIMAGE_IMPORT_BY_NAME ibn = (PIMAGE_IMPORT_BY_NAME)((PUCHAR)remoteBase + oft->u1.AddressOfData);
                            ULONG64 func = GetExportByName(modBase, ibn->Name);
                            if (func) ft->u1.Function = func;
                        }
                        oft++; ft++;
                    }
                }
                imp++;
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {}
        Log("Imports resolved");
    }
    
    
    PVOID shellAddr = NULL;
    SIZE_T shellSize = sizeof(SHELLCODE_BLOCK);
    status = ZwAllocateVirtualMemory(ZwCurrentProcess(), &shellAddr, 0, &shellSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS(status))
    {
        KeUnstackDetachProcess(&apc);
        ZwClose(processHandle);
        Log("ERROR: Shellcode alloc: 0x%X", status);
        return status;
    }
    
    
    SHELLCODE_BLOCK block = {0};
    RtlCopyMemory(block.Code, g_Shellcode, sizeof(g_Shellcode));
    block.DllBase = (ULONG64)remoteBase;
    block.EntryPoint = (ULONG64)remoteBase + nt->OptionalHeader.AddressOfEntryPoint;
    RtlCopyMemory(shellAddr, &block, sizeof(block));
    
    Log("Shellcode at 0x%p", shellAddr);
    Log("DllBase=0x%llX", block.DllBase);
    Log("EntryPoint offset=0x%X", nt->OptionalHeader.AddressOfEntryPoint);
    Log("EntryPoint=0x%llX", block.EntryPoint);
    
    
    BOOLEAN entryInSection = FALSE;
    PIMAGE_SECTION_HEADER secCheck = IMAGE_FIRST_SECTION(nt);
    for (USHORT i = 0; i < nt->FileHeader.NumberOfSections; i++)
    {
        ULONG secStart = secCheck[i].VirtualAddress;
        ULONG secEnd = secStart + secCheck[i].Misc.VirtualSize;
        if (nt->OptionalHeader.AddressOfEntryPoint >= secStart && 
            nt->OptionalHeader.AddressOfEntryPoint < secEnd)
        {
            Log("Entry in section %d (%.8s)", i, secCheck[i].Name);
            entryInSection = TRUE;
            break;
        }
    }
    if (!entryInSection)
    {
        Log("WARNING: Entry point not in any section!");
    }
    
    KeUnstackDetachProcess(&apc);
    
    
    UNICODE_STRING fn = RTL_CONSTANT_STRING(L"RtlCreateUserThread");
    typedef NTSTATUS(NTAPI* tRtlCreateUserThread)(HANDLE,PSECURITY_DESCRIPTOR,BOOLEAN,ULONG,PSIZE_T,PSIZE_T,PVOID,PVOID,PHANDLE,PVOID);
    tRtlCreateUserThread pCreate = (tRtlCreateUserThread)MmGetSystemRoutineAddress(&fn);
    
    if (!pCreate)
    {
        ZwClose(processHandle);
        Log("ERROR: RtlCreateUserThread not found");
        return STATUS_NOT_FOUND;
    }
    
    HANDLE threadHandle = NULL;
    status = pCreate(processHandle, NULL, FALSE, 0, NULL, NULL,
        shellAddr,      
        shellAddr,      
        &threadHandle, NULL);
    
    if (NT_SUCCESS(status) && threadHandle)
    {
        Log("Thread created, waiting...");
        LARGE_INTEGER timeout;
        timeout.QuadPart = -100000000LL; 
        ZwWaitForSingleObject(threadHandle, FALSE, &timeout);
        ZwClose(threadHandle);
        Log("DllMain should have run");
    }
    else
    {
        Log("ERROR: Thread creation: 0x%X", status);
    }
    
    ZwClose(processHandle);
    *MappedBase = (ULONG64)remoteBase;
    return STATUS_SUCCESS;
}


ULONG64 FindModuleBase(PPEB Peb, const char* Name)
{
    if (!Peb || !Name) return 0;
    __try {
        PPEB_LDR_DATA_FULL ldr = *(PPEB_LDR_DATA_FULL*)((PUCHAR)Peb + 0x18);
        if (!ldr) return 0;
        PLIST_ENTRY head = &ldr->InLoadOrderModuleList;
        for (PLIST_ENTRY cur = head->Flink; cur != head; cur = cur->Flink)
        {
            PLDR_DATA_TABLE_ENTRY_FULL e = CONTAINING_RECORD(cur, LDR_DATA_TABLE_ENTRY_FULL, InLoadOrderLinks);
            if (e->BaseDllName.Buffer)
            {
                ANSI_STRING as = {0};
                if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&as, &e->BaseDllName, TRUE)))
                {
                    BOOLEAN m = (_stricmp(as.Buffer, Name) == 0);
                    PVOID b = e->DllBase;
                    RtlFreeAnsiString(&as);
                    if (m) return (ULONG64)b;
                }
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
    return 0;
}


ULONG64 GetExportByName(ULONG64 Base, const char* Name)
{
    if (!Base || !Name) return 0;
    __try {
        PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)Base;
        PIMAGE_NT_HEADERS64 nt = (PIMAGE_NT_HEADERS64)(Base + dos->e_lfanew);
        PIMAGE_DATA_DIRECTORY ed = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        if (!ed->Size) return 0;
        PIMAGE_EXPORT_DIRECTORY exp = (PIMAGE_EXPORT_DIRECTORY)(Base + ed->VirtualAddress);
        PULONG names = (PULONG)(Base + exp->AddressOfNames);
        PUSHORT ords = (PUSHORT)(Base + exp->AddressOfNameOrdinals);
        PULONG funcs = (PULONG)(Base + exp->AddressOfFunctions);
        for (ULONG i = 0; i < exp->NumberOfNames; i++)
        {
            if (strcmp((char*)(Base + names[i]), Name) == 0)
                return Base + funcs[ords[i]];
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
    return 0;
}

ULONG64 GetExportByOrdinal(ULONG64 Base, USHORT Ord) { UNREFERENCED_PARAMETER(Base); UNREFERENCED_PARAMETER(Ord); return 0; }
