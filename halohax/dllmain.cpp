// dllmain.cpp : Defines the entry point for the DLL application.

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE module,
    DWORD  reason,
    LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        MessageBoxA(NULL, "HaloHax loaded", "Success!", MB_ICONQUESTION | MB_OK);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        MessageBoxA(NULL, "HaloHax unloaded", "Done!", MB_ICONQUESTION | MB_OK);
        break;
    }

    return TRUE;
}
