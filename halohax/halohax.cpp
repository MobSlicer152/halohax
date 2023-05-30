#include "halohax.h"

DWORD HaxStart(void* unused)
{
#ifdef Debug
    spdlog::set_level(spdlog::level::debug);
#else
    spdlog::set_level(spdlog::level::info);
#endif

    SPDLOG_DEBUG("HaloHax thread started");

    SPDLOG_DEBUG("Attempting to inject NVIDIA Nsight");
    uint32_t installationCount = 0;
    NGFX_Injection_EnumerateInstallations(&installationCount, nullptr);
    std::vector<NGFX_Injection_InstallationInfo> installations(installationCount);
    NGFX_Injection_EnumerateInstallations(&installationCount, installations.data());

    if (installationCount)
    {
        uint32_t activityCount = 0;
        NGFX_Injection_EnumerateActivities(&installations[0], &activityCount, nullptr);
        std::vector<NGFX_Injection_Activity> activities(activityCount);
        NGFX_Injection_EnumerateActivities(&installations[0], &activityCount, activities.data());

        NGFX_Injection_Activity* frameDebugActivity = nullptr;
        for (auto& activity : activities)
        {
            if (activity.type == NGFX_INJECTION_ACTIVITY_FRAME_DEBUGGER)
            {
                frameDebugActivity = &activity;
            }
        }

        if (frameDebugActivity)
        {
            NGFX_Injection_InjectToProcess(&installations[0], frameDebugActivity);
            SPDLOG_DEBUG("Nsight attached");
        }
    }

    SPDLOG_DEBUG("HaloHax thread completed");

    return 0;
}
