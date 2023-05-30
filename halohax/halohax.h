#pragma once

#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <iostream>

#include <windows.h>

#include "NGFX_Injection.h"

#define SPDLOG_ACTIVE_LEVEL 0
#include "spdlog/spdlog.h"

// Real starting point of the DLL, after DllMain
extern DWORD HaxStart(void* unused);
