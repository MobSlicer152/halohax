#pragma once

#include <cstdio>
#include <iostream>

#include <windows.h>

#define SPDLOG_ACTIVE_LEVEL 0
#include "spdlog/spdlog.h"

// Real starting point of the DLL, after DllMain
extern DWORD HaxStart(void* unused);
