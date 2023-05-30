#include <cinttypes>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <type_traits>
#include <vector>

#include <windows.h>
#include <tlhelp32.h>

#define SPDLOG_ACTIVE_LEVEL 0
#include "spdlog/spdlog.h"

// Data passed to the remote thread
struct ThreadData
{
    char* dllName;
    DWORD (*GetLastError)();
    HMODULE (*LoadLibraryA)(PCSTR);
    void (*ExitThread)(DWORD);
};

// Name of the Halo Infinite executable to look for
#define HALO_IMAGE_NAME "HaloInfinite.exe"

// Name of the executable to source the injection code from
#define LOADER_NAME "loader.bin"

// Name of the DLL to inject
#define DLL_NAME "halohax.dll"

// Exit the program
void Quit(int exitCode);

// Get a handle to the first Halo Infinite process found
HANDLE FindHalo();

// Load the text section of the specified executable
std::vector<uint8_t> LoadShellcode(const std::filesystem::path& shellcodePath);

// Inject the specified DLL into the specified process
void InjectDll(HANDLE process, const std::filesystem::path& dllPath, void* code, size_t codeSize);

int main(int argc, char* argv[])
{
#ifdef Debug
    spdlog::set_level(spdlog::level::debug);
#else
    spdlog::set_level(spdlog::level::info);
#endif

    HANDLE haloProcess = FindHalo();

    auto shellcodePath = std::filesystem::absolute(LOADER_NAME);
    SPDLOG_DEBUG("Loading shellcode from {}", shellcodePath.string());
    std::ifstream codeStream(shellcodePath, std::ios::binary | std::ios::ate);
    if (codeStream.fail())
    {
        DWORD error = GetLastError();
        SPDLOG_CRITICAL("Failed to open {0}: {1}/0x{1:X}", shellcodePath.string(), error);
        Quit(error);
    }

    std::vector<uint8_t> code(codeStream.tellg());
    codeStream.seekg(std::ios::beg);
    codeStream.read((char*)code.data(), code.size());
    codeStream.close();

    auto dllPath = std::filesystem::absolute(DLL_NAME);
    InjectDll(haloProcess, dllPath, code.data(), code.size());

    Quit(0);
}

void Quit(int exitCode)
{
    if (!IsDebuggerPresent())
    {
        std::cout << std::endl << "Press any key to continue . . . " << std::endl;

        HANDLE console = GetStdHandle(STD_INPUT_HANDLE);
        DWORD consoleMode = 0;
        GetConsoleMode(console, &consoleMode);
        consoleMode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        SetConsoleMode(console, consoleMode);

        std::cin.get();

        consoleMode &= ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT;
        SetConsoleMode(console, consoleMode);
    }

    exit(exitCode);
}

HANDLE FindHalo()
{
    SPDLOG_INFO("Finding Halo Infinite process");

    SPDLOG_DEBUG("Creating process snapshot");
    HANDLE processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (processSnapshot == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        SPDLOG_CRITICAL("Failed to create process snapshot: {0}/0x{0:X}", error);
        Quit(error);
    }

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    SPDLOG_DEBUG("Searching processes");
    DWORD haloPid = MAXDWORD;
    if (Process32First(processSnapshot, &entry))
    {
        do
        {
            if (strncmp(entry.szExeFile, HALO_IMAGE_NAME, std::extent<decltype(entry.szExeFile)>::value) == 0)
            {
                haloPid = entry.th32ProcessID;
                SPDLOG_INFO("Found process {} {}", entry.szExeFile, haloPid);
                break;
            }
            else
            {
                SPDLOG_TRACE("Process {} {}", entry.szExeFile, entry.th32ProcessID);
            }
        } while (Process32Next(processSnapshot, &entry));
    }

    if (haloPid == MAXDWORD) {
        SPDLOG_CRITICAL("Failed to find Halo Infinite process");
        Quit(ERROR_OBJECT_NOT_FOUND);
    }

    HANDLE haloProcess = OpenProcess(PROCESS_CREATE_THREAD |
        PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE |
        PROCESS_VM_WRITE,
        false,
        haloPid);
    if (!haloProcess) {
        DWORD error = GetLastError();
        SPDLOG_CRITICAL("Failed to open process {0}: {1}/0x{1:X}", haloPid, error);
        Quit(error);
    }

    return haloProcess;
}

void InjectDll(HANDLE process, const std::filesystem::path& dllPath, void* code, size_t codeSize)
{
    std::string path = dllPath.string();

    SPDLOG_INFO("Injecting DLL {} into process 0x{:X}", path, (uintptr_t)process);

    SPDLOG_DEBUG("Allocating memory in process");
    size_t size = sizeof(ThreadData) + path.size() + 1;
    ThreadData* remoteData = (ThreadData*)VirtualAllocEx(process, nullptr, size, MEM_COMMIT, PAGE_READWRITE);
    if (!remoteData)
    {
        DWORD error = GetLastError();
        SPDLOG_CRITICAL("Failed to allocate {0} byte(s) of memory in process: {1}/0x{1:X}", size, error);
        Quit(error);
    }

    SPDLOG_DEBUG("Allocated memory at 0x{:X}", (uintptr_t)remoteData);

    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    ThreadData localData = {};
    localData.dllName = (char*)remoteData + sizeof(ThreadData);
    *(PROC*)&localData.GetLastError = GetProcAddress(kernel32, "GetLastError");
    *(PROC*)&localData.LoadLibraryA = GetProcAddress(kernel32, "LoadLibraryA");
    *(PROC*)&localData.ExitThread = GetProcAddress(kernel32, "ExitThread");
    WriteProcessMemory(process, (void*)remoteData, &localData, sizeof(ThreadData), nullptr);
    WriteProcessMemory(process, (void*)localData.dllName, path.c_str(), path.size(), nullptr);
    VirtualProtectEx(process, (void*)remoteData, size, PAGE_READONLY, nullptr);

    SPDLOG_DEBUG("Allocating code memory in process");
    LPTHREAD_START_ROUTINE codeMemory = (LPTHREAD_START_ROUTINE)VirtualAllocEx(process,
        nullptr,
        codeSize,
        MEM_COMMIT,
        PAGE_EXECUTE_READWRITE);
    if (!codeMemory)
    {
        DWORD error = GetLastError();
        SPDLOG_CRITICAL("Failed to allocate {0} byte(s) of code memory in process: {1}/0x{1:X}", codeSize, error);
        Quit(error);
    }

    SPDLOG_DEBUG("Allocated code memory at 0x{:X}", (uintptr_t)codeMemory);

    SPDLOG_DEBUG("Copying DLL loading code to process");
    WriteProcessMemory(process, codeMemory, code, codeSize, nullptr);
    FlushInstructionCache(process, codeMemory, codeSize);

    SPDLOG_INFO("Executing remote thread");
    DWORD threadId = 0;
    HANDLE threadHandle = CreateRemoteThread(process, nullptr, 0x1000, codeMemory, remoteData, 0, &threadId);
    if (!threadHandle)
    {
        DWORD error = GetLastError();
        SPDLOG_CRITICAL("Failed to create remote thread in process 0x{0:X}: {1}/{1:X}", (uintptr_t)process, error);
        Quit(error);
    }
    SetThreadDescription(threadHandle, L"HaloHax injector thread");
    WaitForSingleObject(threadHandle, INFINITE);

    DWORD threadExitCode = 0;
    GetExitCodeThread(threadHandle, &threadExitCode);
    if (threadExitCode == ERROR_SUCCESS)
    {
        SPDLOG_INFO("Thread completed successfully 0x{:X}", threadExitCode);

        SPDLOG_INFO("Cleaning up");
        VirtualFreeEx(process, codeMemory, codeSize, MEM_FREE);
        VirtualFreeEx(process, remoteData, size, MEM_FREE);
        CloseHandle(threadHandle);
    }
    else
    {
        SPDLOG_WARN("Thread returned unexpected value 0x{:X}", threadExitCode);
    }
}