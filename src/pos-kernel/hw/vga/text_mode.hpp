#pragma once
#include <includes.h>

#include <arch/x86.h>
#include <io/console.hpp>

namespace VGA
{
    struct Postion
    {
        u32 x;
        u32 y;
    };

    // initialize the driver
    void init();

    // Set position of cursor to pos
    void setCursorPositon(Postion pos);

    // Get position of cursor
    Postion getCursorPositon();

    // scroll up by lines
    void scrollUp(u32 lines);

    // clear screen
    void clearScreen();

    // clear current line
    void clearLine();

    // set text color
    void setTextColor(u8 vga_color);

    // set background color
    void setBackgroundColor(u8 vga_color);

    // put charecter on screen
    void putc(char ch);

    // get a console
    con::console get_console();
}
