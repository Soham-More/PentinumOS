#include "logger.h"

void sys_logf(uint8_t severity, const char* fmt, ...)
{
    va_list vlist;
    va_start(vlist, fmt);

    uint8_t color;

    switch (severity)
    {
    case Severity_Debug:
        color = VGA::Green | VGA::Blue | VGA::HIGH;
        break;
    case Severity_Info:
        color = VGA::Green | VGA::HIGH;
        break;
    case Severity_Warn:
        color = VGA::Blue | VGA::HIGH;
        break;
    case Severity_Error:
        color = VGA::Red | VGA::HIGH;
        break;
    case Severity_Critical:
        color = VGA::Red | VGA::HIGH;
        break;
    default:
        color = VGA::White | VGA::HIGH;
        break;
    }

    VGA::setTextColor(color);

    vprintf(fmt, vlist);

    VGA::setTextColor(VGA::White | VGA::HIGH);
}
