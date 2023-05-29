#include <iostream>
#include <type_traits>
#include <vector>

#include <windows.h>
#include <tlhelp32.h>

#define SPDLOG_ACTIVE_LEVEL 0
#include <spdlog/spdlog.h>

// Name of the Halo Infinite executable to look for
#define HALO_IMAGE_NAME "HaloInfinite.exe"

// Exit the program
void Quit(int exitCode);

// Get a handle to the first Halo Infinite process found
HANDLE FindHalo();

int main(int argc, char* argv[])
{
#ifdef Debug
    spdlog::set_level(spdlog::level::debug);
#else
    spdlog::set_level(spdlog::level::info);
#endif

    HANDLE haloProcess = FindHalo();

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

    HANDLE haloProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_WRITE, false, haloPid);
    if (!haloProcess) {
        DWORD error = GetLastError();
        SPDLOG_CRITICAL("Failed to open process {0}: {1}/{1:X}", haloPid, error);
        Quit(error);
    }

    return haloProcess;
}