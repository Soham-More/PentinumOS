#pragma once
#include <includes.h>

#include <i686/x86.h>
#include <std/stdmem.hpp>

namespace VGA
{
    struct Postion
    {
        uint32_t x;
        uint32_t y;
    };

    // initialise the driver
    void init();

    // Set position of cursor to pos
    void setCursorPositon(Postion pos);

    // Get position of cursor
    Postion getCursorPositon();

    // scroll up by lines
    void scrollUp(uint32_t lines);

    // clear screen
    void clearScreen();

    // clear current line
    void clearLine();

    // set text color
    void setTextColor(uint8_t vga_color);

    // set background color
    void setBackgroundColor(uint8_t vga_color);

    // put charecter on screen
    void putc(char ch);
}
