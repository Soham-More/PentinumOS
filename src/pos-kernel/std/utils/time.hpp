#pragma once

#include <includes.h>

namespace std
{
    class time_t
    {
        u16 year;
        u8 seconds;
        u8 minutes;
        u8 hours;
        u8 day;
        u8 month;
    }_packed;
}

