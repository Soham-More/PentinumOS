#pragma once

#include <includes.h>
#include <std/stdio.h>
#include <Drivers/Drivers.h>

enum Severity
{
    Severity_Debug = 0,
    Severity_Info = 1,
    Severity_Warn = 2,
    Severity_Error = 3,
    Severity_Critical = 4,
};

void sys_logf(uint8_t severity, const char* fmt, ...);

#define log_debug(...)     sys_logf(Severity_Debug   , __VA_ARGS__)
#define log_info(...)      sys_logf(Severity_Info    , __VA_ARGS__)
#define log_warn(...)      sys_logf(Severity_Warn    , __VA_ARGS__)
#define log_error(...)     sys_logf(Severity_Error   , __VA_ARGS__)
#define log_critical(...)  sys_logf(Severity_Critical, __VA_ARGS__)
