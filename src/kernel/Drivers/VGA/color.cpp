#include "color.hpp"

uint8_t VGA::toVGATextColor(bool red, bool green, bool blue, bool brigtness)
{
    return blue + (green << 1) + (red << 2) + (brigtness << 3);
}
