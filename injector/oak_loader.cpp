
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <vector>


#include "nt.hpp"
#include "utils.hpp"
#include "intel_driver.hpp"
#include "kdmapper.hpp"

std::vector<BYTE> ReadFileToMemory(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<BYTE> buffer((size_t)size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return {};
    }

    return buffer;
}

bool EnsureDriverLoaded() {
    
    if (intel_driver::IsRunning()) {
        std::wcout << L"intel driver already loaded" << std::endl;
        return true;
    }
    
    
    std::wcout << L"loading intel driver..." << std::endl;
    NTSTATUS status = intel_driver::Load();
    if (!NT_SUCCESS(status)) {
        std::wcout << L"failed to load intel driver, status 0x" << std::hex << status << std::endl;
        return false;
    }
    
    std::wcout << L"intel driver loaded" << std::endl;
    return true;
}

int wmain(int argc, wchar_t* argv[]) {
    std::wcout << L"========================================" << std::endl;
    std::wcout << L"      Oak Loader (Intel Driver)" << std::endl;
    std::wcout << L"========================================" << std::endl;
    std::wcout << std::endl;

    
    std::wstring driverPath;
    if (argc > 1) {
        driverPath = argv[1];
    } else {
        wchar_t currentDir[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, currentDir);
        driverPath = std::wstring(currentDir) + L"\\oak.sys";
    }

    
    if (GetFileAttributesW(driverPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::wcout << L"driver not found: " << driverPath << std::endl;
        std::wcout << L"put oak.sys here or give path" << std::endl;
        std::wcout << std::endl;
        std::wcout << L"press enter to exit..." << std::endl;
        std::wcin.get();
        return 1;
    }

    std::wcout << L"driver: " << driverPath << std::endl;

    
    std::wcout << L"checking intel driver..." << std::endl;
    if (!EnsureDriverLoaded()) {
        std::wcout << L"failed to load intel driver" << std::endl;
        std::wcout << std::endl;
        std::wcout << L"press enter to exit..." << std::endl;
        std::wcin.get();
        return 1;
    }

    std::wcout << L"intel driver ready" << std::endl;

    
    std::wcout << L"reading driver file..." << std::endl;
    auto driverData = ReadFileToMemory(driverPath);
    if (driverData.empty()) {
        std::wcout << L"failed to read driver" << std::endl;
        intel_driver::Unload();
        std::wcout << std::endl;
        std::wcout << L"press enter to exit..." << std::endl;
        std::wcin.get();
        return 1;
    }

    std::wcout << L"read " << driverData.size() << L" bytes" << std::endl;

    
    std::wcout << L"mapping driver..." << std::endl;
    
    NTSTATUS exitCode = 0;
    ULONG64 result = kdmapper::MapDriver(driverData.data(), 0, 0, false, true);

    if (result) {
        std::wcout << L"driver mapped at 0x" << std::hex << result << std::endl;
        std::wcout << std::endl;
        std::wcout << L"========================================" << std::endl;
        std::wcout << L"           DRIVER LOADED!" << std::endl;
        std::wcout << L"========================================" << std::endl;
        std::wcout << std::endl;
        std::wcout << L"start dayz then inject" << std::endl;
        std::wcout << L"need C:\\oak\\dayz_internal.dll" << std::endl;
    } else {
        std::wcout << L"failed to map driver" << std::endl;
    }

    
    intel_driver::Unload();

    std::wcout << std::endl;
    std::wcout << L"press enter to exit..." << std::endl;
    std::wcin.get();

    return result ? 0 : 1;
}
