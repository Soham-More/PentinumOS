#include "text_mode.hpp"

#include <stdarg.h>
#include <stdbool.h>

#include <arch/x86.h>

// x, y coords to linear address

// liner address for charecter byte
#define currPositionChar  2 * (screenY * SCREEN_WIDTH + screenX) + 0
#define currPositionColor 2 * (screenY * SCREEN_WIDTH + screenX) + 1
#define videoBufferSize   2 * ((SCREEN_HEIGHT - 1) * SCREEN_WIDTH + SCREEN_WIDTH)

// VGA defines
#define CRT_Controller_Address_Register 0x3D4
#define CRT_Controller_Data_Register    0x3D5

#define cursorLocationRegisterAddressLow   0x0F
#define cursorLocationRegisterAddressHigh  0x0E

namespace VGA
{
    const unsigned SCREEN_WIDTH = 80;
    const unsigned SCREEN_HEIGHT = 25;
    static u8 currentColor = 0xF;  // bright white
    static u8 tabSize      = 4;
    static u32 screenX = 0;
    static u32 screenY = 0;

    u8* svideoBuffer = (u8*)0xB8000;

    void init()
    {
        screenX = 0;
        screenY = 0;
    }

    // Set position of cursor to (x, y)
    void setCursorPositon(Postion pos)
    {
        // calculate linear position
        int lpos = (pos.y * SCREEN_WIDTH + pos.x);

        // tell VGA that we are setting cursor location(low byte) register
        x86_outb(CRT_Controller_Address_Register, cursorLocationRegisterAddressLow);
        // set lower byte of cursor location register to lower byte of linear positon
        x86_outb(CRT_Controller_Data_Register, (u8)(lpos & 0xFF));

        // tell VGA that we are setting cursor location(high byte) register
        x86_outb(CRT_Controller_Address_Register, cursorLocationRegisterAddressHigh);
        // set higher byte of cursor location register to higher byte of linear positon
        x86_outb(CRT_Controller_Data_Register, (u8)((lpos >> 8) & 0xFF));
    }

    // Get position of cursor
    Postion getCursorPositon()
    {
        return {screenX, screenY};
    }

    // shift buffer up by lines amount
    void scrollUp(u32 lines)
    {
        u32 shift = (2 * lines * SCREEN_WIDTH);
        u8* shiftedBuffer = svideoBuffer + shift;

        for(u32 i = 0; i < videoBufferSize; i += 2)
        {
            if(i < videoBufferSize - shift)
            {
                svideoBuffer[i + 0] = shiftedBuffer[i + 0];
                svideoBuffer[i + 1] = shiftedBuffer[i + 1];
            }
            else
            {
                svideoBuffer[i + 0] = 0;
                svideoBuffer[i + 1] = currentColor;
            }
        }
    }

    void clearScreen()
    {
        for(u32 i = 0; i < videoBufferSize; i += 2)
        {
            svideoBuffer[i + 0] = 0;
            svideoBuffer[i + 1] = currentColor;
        }

        screenX = 0;
        screenY = 0;

        setCursorPositon({screenX, screenY});
    }

    void clearLine()
    {
        for(u32 i = 0; i < SCREEN_WIDTH; i++)
        {
            screenX = i;

            svideoBuffer[currPositionChar] = 0;
            svideoBuffer[currPositionColor] = currentColor;
        }

        screenX = 0;

        setCursorPositon({screenX, screenY});
    }

    void setTextColor(u8 vga_color)
    {
        vga_color &= 0x0F;
        currentColor = (currentColor & 0xF0) + vga_color;
    }

    void setBackgroundColor(u8 vga_color)
    {
        vga_color &= 0x0F;
        currentColor = (currentColor & 0x0F) + (vga_color << 4);
    }

    void putc(char ch)
    {
        switch (ch)
        {
            case '\n':
                screenY++;
                screenX = 0;
                break;
            case '\t':
                for (int i = 0; i < tabSize - (screenX % tabSize); i++)
                    putc(' ');
                break;
            case '\r':
                screenX = 0;
                break;
            case '\b':
                if(screenX > 0)
                {
                    screenX--;

                    svideoBuffer[currPositionChar]  = 0;
                    svideoBuffer[currPositionColor] = currentColor;
                }
                break;
            default:
                svideoBuffer[currPositionChar]  = ch;
                svideoBuffer[currPositionColor] = currentColor;

                screenX++;
                break;
        }

        // reached limit, start from next line
        if(screenX >= SCREEN_WIDTH)
        {
            screenY++;
            screenX = 0;
        }

        // reached y limit, scroll up
        if(screenY >= SCREEN_HEIGHT)
        {
            screenY = SCREEN_HEIGHT - 1;
            scrollUp(1);
        }

        setCursorPositon({screenX, screenY});
    }

    void clear(con::console* console)
    {
        clearScreen();
    }

    void write(con::console* console, const char* buffer, size_t len)
    {
        setTextColor(console->text_color & 0xF);

        for(size_t i = 0; i < len; i++)
        {
            putc(buffer[i]);
        }
    }

    con::console get_console()
    {
        return con::console{
            .name = "vga0",
            .write = write,
            .read = con::ignore_read,
            .clear = clear,
            .text_color = CON_COLOR_WHITE,
            .data = nullptr,
        };
    }
}
