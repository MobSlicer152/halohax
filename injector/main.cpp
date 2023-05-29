#include <iostream>
#include <type_traits>
#include <vector>

#include <windows.h>
#include <tlhelp32.h>

#define SPDLOG_ACTIVE_LEVEL 0
#include <spdlog/spdlog.h>

// Name of the Halo Infinite executable to look for
#define HALO_IMAGE_NAME "HaloInfinite.exe"

// Name of the DLL to inject
#define DLL_NAME "halohax.dll"

// Exit the program
void Quit(int exitCode);

// Get a handle to the first Halo Infinite process found
HANDLE FindHalo();

// Inject the specified DLL into the specified process
void InjectDll(HANDLE process, const char* dllName);

int main(int argc, char* argv[])
{
#ifdef Debug
    spdlog::set_level(spdlog::level::debug);
#else
    spdlog::set_level(spdlog::level::info);
#endif

    HANDLE haloProcess = FindHalo();
    InjectDll(haloProcess, DLL_NAME);

    Quit(0);
}

void Quit(int exitCode)
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
        SPDLOG_CRITICAL("Failed to create process snapshot: {0}/{0:X}", error);
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
        SPDLOG_CRITICAL("Failed to open process {0}: {1}/{1:X}", haloPid, error);
        Quit(error);
    }

    return haloProcess;
}

struct ThreadData
{
    char* dllName;
    char* kernel32;
};

__declspec(noinline) void RemoteLoadDll(void* userData)
{

}
__declspec(noinline) void RemoteLoadDllEnd()
{
}

void InjectDll(HANDLE process, const char* dllName)
{
    char kernel32[] = "kernel32.dll";

    SPDLOG_INFO("Injecting DLL {} into process {:X}", dllName, process);

    SPDLOG_DEBUG("Allocating string memory in process");
    ThreadData data = {};
    size_t size = strlen(dllName) + 1 + strlen(kernel32) + 1;
    data.dllName = (char*)VirtualAllocEx(process, NULL, size, MEM_COMMIT, PAGE_READWRITE);
    if (!data.dllName)
    {
        DWORD error = GetLastError();
        SPDLOG_CRITICAL("Failed to allocate {} byte(s) of memory in process: {}", size, error);
        Quit(error);
    }
    data.kernel32 = data.dllName + strlen(dllName) + 1;
    WriteProcessMemory(process, (void*)data.dllName, dllName, strlen(dllName) + 1, NULL);
    WriteProcessMemory(process, (void*)data.kernel32, kernel32, strlen(kernel32) + 1, NULL);
    VirtualProtectEx(process, data.dllName, size, PAGE_READONLY, NULL);

    SPDLOG_DEBUG("Allocating code memory in process");
    size_t codeSize = (uintptr_t)RemoteLoadDllEnd - (uintptr_t)RemoteLoadDll;
    LPTHREAD_START_ROUTINE codeMemory = (LPTHREAD_START_ROUTINE)VirtualAllocEx(process,
                                                                              NULL,
                                                                              codeSize,
                                                                              MEM_COMMIT,
                                                                              PAGE_READWRITE);
    if (!codeMemory)
    {
        DWORD error = GetLastError();
        SPDLOG_CRITICAL("Failed to allocate {} byte(s) of code memory in process: {}", size, error);
        Quit(error);
    }

    SPDLOG_DEBUG("Copying DLL loading code to process");
    WriteProcessMemory(process, codeMemory, RemoteLoadDll, size, NULL);
    VirtualProtectEx(process, codeMemory, size, PAGE_EXECUTE_READ, NULL);

    SPDLOG_DEBUG("Executing remote thread");
    CreateRemoteThread(process, NULL, 0x2000, codeMemory, )
}