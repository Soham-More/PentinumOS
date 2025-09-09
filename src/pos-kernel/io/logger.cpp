#include "logger.h"

#include "console.hpp"
#include "stdio.h"

void sys_logf(u8 severity, const char* fmt, ...)
{
    va_list vlist;
    va_start(vlist, fmt);

    u8 color;

    switch (severity)
    {
    case Severity_Debug:
        color = CON_COLOR_GREEN | CON_COLOR_BLUE;
        break;
    case Severity_Info:
        color = CON_COLOR_GREEN;
        break;
    case Severity_Warn:
        color = CON_COLOR_BLUE;
        break;
    case Severity_Error:
        color = CON_COLOR_RED;
        break;
    case Severity_Critical:
        color = CON_COLOR_RED;
        break;
    default:
        color = CON_COLOR_WHITE;
        break;
    }

    con::get_default()->text_color = color;

    vprintf(fmt, vlist);

    con::get_default()->text_color = CON_COLOR_WHITE;
}
