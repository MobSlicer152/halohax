// dllmain.cpp : Defines the entry point for the DLL application.

#include "halohax.h"

// Does stuff when the DLL is attached to a process
void Attach();

BOOL APIENTRY DllMain(HMODULE module,
    DWORD  reason,
    LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        Attach();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        SPDLOG_INFO("HaloHax unloaded");
        break;
    }

    return TRUE;
}

void Attach()
{
    AllocConsole();

    FILE* tempFile;
    freopen_s(&tempFile, "CONIN$", "r", stdin);
    freopen_s(&tempFile, "CONOUT$", "w", stdout);
    freopen_s(&tempFile, "CONOUT$", "w", stderr);
    std::cin.clear();
    std::cout.clear();
    std::cerr.clear();
#ifdef Debug
    spdlog::flush_on(spdlog::level::debug);
#else
    spdlog::flush_on(spdlog::level::info);
#endif

    SPDLOG_INFO("HaloHax loaded");

    HANDLE threadHandle = CreateThread(NULL, 0x2000, HaxStart, NULL, 0, NULL);
    SetThreadDescription(threadHandle, L"HaloHax");
}
