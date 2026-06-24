#pragma once

#include <includes.h>
#include <resources/tty.h>

enum log_severity_t
{
    Severity_Debug = 0,
    Severity_Info  = 1,
    Severity_Warn  = 2,
    Severity_Error = 3,
    Severity_Critical = 4,
};

void sys_logf(u8 severity, const char* fmt, ...);

// lock and unlock the tty
// when you need guarenteed exclusive access to the tty
tty_t* logging_lock();
void logging_unlock(tty_t** tty);
#define LOG_AQUIRE_SCOPED_LOCK() tty_t* __log_lock __attribute__((cleanup(logging_unlock))) = logging_lock();

#define log_debug(...)     sys_logf(Severity_Debug   , __VA_ARGS__)
#define log_info(...)      sys_logf(Severity_Info    , __VA_ARGS__)
#define log_warn(...)      sys_logf(Severity_Warn    , __VA_ARGS__)
#define log_error(...)     sys_logf(Severity_Error   , __VA_ARGS__)
#define log_critical(...)  sys_logf(Severity_Critical, __VA_ARGS__)

void logging_set_tty(tty_t* tty);
bool logging_is_initialized();