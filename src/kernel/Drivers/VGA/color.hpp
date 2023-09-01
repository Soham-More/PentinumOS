#pragma once
#include <includes.h>

namespace VGA
{
    enum Colors
    {
        Red = 0x04,
        Green = 0x02,
        Blue = 0x01,
        White = 0x7,
    };

    enum Brightness
    {
        LOW = 0x0,
        HIGH = 0x8
    };

    struct Color
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    uint8_t toVGATextColor(bool red, bool green, bool blue, bool brigtness);
}
