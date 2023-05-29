#include <vector>

#include <windows.h>
#include <tlhelp32.h>

#include <spdlog/spdlog.h>

// Get a handle to the first Halo Infinite process found
HANDLE FindHalo();

int main(int argc, char* argv[])
{
    HANDLE haloProcess = FindHalo();
}

HANDLE FindHalo()
{
    HANDLE processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (processSnapshot == INVALID_HANDLE_VALUE)
    {
        SPDLOG_INFO("Failed to create process snapshot");
        return;
    }

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(processSnapshot, &entry))
    {
        do
        {
            
        }
    }
}