#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>

DWORD GetProcessIdByName(const char* processName)
{
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (snapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(pe32);
        
        if (Process32First(snapshot, &pe32))
        {
            do
            {
                if (_stricmp(pe32.szExeFile, processName) == 0)
                {
                    pid = pe32.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &pe32));
        }
        CloseHandle(snapshot);
    }
    
    return pid;
}

bool InjectDll(DWORD processId, const char* dllPath)
{
    
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess)
    {
        std::cout << "[!] Failed to open process. Error: " << GetLastError() << std::endl;
        return false;
    }

    
    size_t pathLen = strlen(dllPath) + 1;
    LPVOID remotePath = VirtualAllocEx(hProcess, nullptr, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remotePath)
    {
        std::cout << "[!] Failed to allocate memory. Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    
    if (!WriteProcessMemory(hProcess, remotePath, dllPath, pathLen, nullptr))
    {
        std::cout << "[!] Failed to write memory. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    
    LPVOID loadLibAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLibAddr)
    {
        std::cout << "[!] Failed to get LoadLibraryA address." << std::endl;
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, remotePath, 0, nullptr);
    if (!hThread)
    {
        std::cout << "[!] Failed to create remote thread. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    
    WaitForSingleObject(hThread, 5000);

    
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return true;
}

int main(int argc, char* argv[])
{
    std::cout << "========================================" << std::endl;
    std::cout << "       DayZ Internal Injector" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    
    char dllPath[MAX_PATH];
    if (argc > 1)
    {
        strcpy_s(dllPath, argv[1]);
    }
    else
    {
        
        GetCurrentDirectoryA(MAX_PATH, dllPath);
        strcat_s(dllPath, "\\dayz_internal.dll");
    }

    
    if (GetFileAttributesA(dllPath) == INVALID_FILE_ATTRIBUTES)
    {
        std::cout << "[!] DLL not found: " << dllPath << std::endl;
        std::cout << "[*] Place dayz_internal.dll in the same folder as injector" << std::endl;
        std::cout << std::endl;
        std::cout << "Press any key to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    std::cout << "[*] DLL: " << dllPath << std::endl;
    std::cout << "[*] Waiting for DayZ_x64.exe..." << std::endl;

    
    DWORD pid = 0;
    while (pid == 0)
    {
        pid = GetProcessIdByName("DayZ_x64.exe");
        if (pid == 0)
        {
            Sleep(1000);
            std::cout << "." << std::flush;
        }
    }

    std::cout << std::endl;
    std::cout << "[+] Found DayZ_x64.exe (PID: " << pid << ")" << std::endl;

    
    std::cout << "[*] Waiting for game to initialize..." << std::endl;
    Sleep(5000);

    
    std::cout << "[*] Injecting..." << std::endl;
    if (InjectDll(pid, dllPath))
    {
        std::cout << "[+] Injection successful!" << std::endl;
        std::cout << std::endl;
        std::cout << "[*] Controls:" << std::endl;
        std::cout << "    INSERT - Toggle Menu" << std::endl;
        std::cout << "    END    - Unload" << std::endl;
        std::cout << "    DELETE - Panic Unload" << std::endl;
    }
    else
    {
        std::cout << "[!] Injection failed!" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Press any key to exit..." << std::endl;
    std::cin.get();

    return 0;
}
