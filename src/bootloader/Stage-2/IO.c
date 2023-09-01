#include "IO.h"

#include <stdarg.h>
#include <stdbool.h>
#include "x86.h"
#include "Memory.h"

const unsigned SCREEN_WIDTH = 80;
const unsigned SCREEN_HEIGHT = 25;
const uint8_t DEFAULT_COLOR = 0x7;

uint8_t* svideoBuffer = (uint8_t*)0xB8000;
int screenX = 0, screenY = 0;

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

#define TAB_SIZE 4

// Set position of cursor to (x, y)
void setCursorPositon(int x, int y)
{
    // calculate linear position
    int lpos = (y * SCREEN_WIDTH + x);

    // tell VGA that we are setting cursor location(low byte) register
    x86_outb(CRT_Controller_Address_Register, cursorLocationRegisterAddressLow);
    // set lower byte of cursor location register to lower byte of linear positon
    x86_outb(CRT_Controller_Data_Register, (uint8_t)(lpos & 0xFF));

    // tell VGA that we are setting cursor location(high byte) register
    x86_outb(CRT_Controller_Address_Register, cursorLocationRegisterAddressHigh);
    // set higher byte of cursor location register to higher byte of linear positon
    x86_outb(CRT_Controller_Data_Register, (uint8_t)((lpos >> 8) & 0xFF));
}

// shift buffer up by lines amount
void shiftBufferUp(uint32_t lines)
{
    uint32_t shift = (2 * lines * SCREEN_WIDTH);
    uint8_t* shiftedBuffer = svideoBuffer + shift;

    for(uint32_t i = 0; i < videoBufferSize; i += 2)
    {
        if(i < videoBufferSize - shift)
        {
            svideoBuffer[i + 0] = shiftedBuffer[i + 0];
            svideoBuffer[i + 1] = shiftedBuffer[i + 1];
        }
        else
        {
            svideoBuffer[i + 0] = 0;
            svideoBuffer[i + 1] = DEFAULT_COLOR;
        }
    }
}

void clrscr()
{
    for(uint32_t i = 0; i < videoBufferSize; i += 2)
    {
        svideoBuffer[i + 0] = 0;
        svideoBuffer[i + 1] = DEFAULT_COLOR;
    }

    screenX = 0;
    screenY = 0;

    setCursorPositon(screenX, screenY);
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
        for (int i = 0; i < TAB_SIZE - (screenX % TAB_SIZE); i++)
            putc(' ');
        break;
    case '\r':
        screenX = 0;
        break;
    default:
        svideoBuffer[currPositionChar]  = ch;
        svideoBuffer[currPositionColor] = DEFAULT_COLOR;

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
        shiftBufferUp(1);
    }

    setCursorPositon(screenX, screenY);
}

void puts(const char* str)
{
    uint32_t str_len = 1;

    for(; *str; str++)
    {
        putc(*str);
        str_len++;
    }
}

void putstr(const char* str, uint32_t min_width)
{
    uint32_t str_len = 1;

    for(; *str; str++)
    {
        putc(*str);
        str_len++;
    }

    // string was smaller than min_width, pad with spaces
    if(str_len <= min_width)
    {
        for(; str_len <= min_width; str_len++)
        {
            putc(' ');
        }
    }
}

void put_buffer(const char* str, uint32_t size, uint32_t min_width)
{
    for(uint32_t i = 0; i < size; i++)
    {
        putc(*(str + i));
    }

    // string was smaller than min_width, pad with spaces
    if(size <= min_width)
    {
        for(; size <= min_width; size++)
        {
            putc(' ');
        }
    }
}

#define PRINT_MODE 0
#define FORMAT_LARG_MODE 1 // Take Length
#define FORMAT_TARG_MODE 2 // Take type
#define FORMAT_WARG_MODE 3 // Take Width

#define LARG_NONE 0
#define LARG_SHORT 1
#define LARG_LONG 2
#define LARG_LONG_LONG 3

void print_unsigned(unsigned long long value, uint8_t base, uint32_t min_width)
{
    char buffer[32];
    uint32_t buffer_pos = 0;

    const char hexChars[] = "0123456789abcdef";

    do
    {
        unsigned long long mod = value % base;
        value /= base;
        buffer[buffer_pos++] = hexChars[mod];
    } while (value > 0);

    // if final size is less than min_width, add padding
    if(buffer_pos < min_width)
    {
        for(uint32_t i = buffer_pos; i < min_width; i++)
        {
            putc('0');
        }
    }
    
    while(buffer_pos--)
    {
        putc(buffer[buffer_pos]);
    }
}

void print_signed(signed long long value, uint8_t base, uint32_t min_width)
{
    if(value < 0)
    {
        putc('-');
        value *= -1;
    }

    if(min_width == 0) min_width++;

    print_unsigned(value, base, min_width - 1);
}

void putHex(uint32_t byte)
{
    print_unsigned(byte, 16, 0);
}

void printf(const char* fmt, ...)
{
    uint8_t mode = PRINT_MODE;
    uint8_t larg = LARG_NONE;

    bool shouldFormatValue = false;
    uint8_t base = 0;
    bool isSigned = false;

    uint32_t width = 0;

    va_list vargs;
    va_start(vargs, fmt);

    while (*fmt)
    {
        switch (mode)
        {
            case FORMAT_LARG_MODE:
                switch (*fmt)
                {
                    case 'h':
                        larg = LARG_SHORT;
                        fmt++;
                        break;
                    case 'l':
                        larg = LARG_LONG;

                        if(*(fmt + 1) == 'l')
                        {
                            larg = LARG_LONG_LONG;
                            fmt++;
                        }
                        fmt++;
                        break;
                    default:
                        break;
                }

                mode = FORMAT_TARG_MODE;
                continue;

                break;
            case FORMAT_TARG_MODE:
                switch (*fmt)
                {
                    case 'c':
                        putc((char)va_arg(vargs, int));
                        break;
                    case 's':
                        putstr(va_arg(vargs, const char*), width);
                        width = 0;
                        break;
                    case 'S':
                        const char* buffer = va_arg(vargs, const char*);
                        unsigned int size = va_arg(vargs, unsigned int);
                        put_buffer(buffer, size, width);
                        width = 0;
                        break;
                    case '%':
                        putc('%');
                        break;
                    case 'd':
                    case 'i':
                        shouldFormatValue = true;
                        base = 10;
                        isSigned = true;
                        break;
                    case 'u':
                        shouldFormatValue = true;
                        base = 10;
                        isSigned = false;
                        break;
                    case 'X':
                    case 'x':
                    case 'p':
                        shouldFormatValue = true;
                        base = 16;
                        isSigned = false;
                        break;
                    default:
                        break;
                }

                if(shouldFormatValue)
                {
                    if(isSigned)
                    {
                        switch (larg)
                        {
                        case LARG_SHORT:
                        case LARG_NONE:
                            print_signed(va_arg(vargs, int), base, width);
                            break;
                        case LARG_LONG:
                            print_signed(va_arg(vargs, long), base, width);
                            break;
                        case LARG_LONG_LONG:
                            print_signed(va_arg(vargs, long long), base, width);
                            break;
                        default:
                            break;
                        }
                    }
                    else
                    {
                        switch (larg)
                        {
                        case LARG_SHORT:
                        case LARG_NONE:
                            unsigned int value = va_arg(vargs, unsigned int);
                            print_unsigned(value, base, width);
                            break;
                        case LARG_LONG:
                            print_unsigned(va_arg(vargs, unsigned long), base, width);
                            break;
                        case LARG_LONG_LONG:
                            print_unsigned(va_arg(vargs, unsigned long long), base, width);
                            break;
                        default:
                            break;
                        }
                    }

                    shouldFormatValue = false;
                    isSigned = false;
                    base = 0;
                    width = 0;
                }

                mode = PRINT_MODE;
                larg = LARG_NONE;
                fmt++;
                continue;

                break;

            case FORMAT_WARG_MODE:
                // take width from vargs
                if(*fmt == '*')
                {
                    width = va_arg(vargs, unsigned int);
                }

                // if this is not a width(ie a number or '*')
                if(!(*fmt >= '0' && *fmt <= '9'))
                {
                    mode = FORMAT_LARG_MODE;
                    continue;
                }

                width = (width * 10) + (*fmt - '0');

                fmt++;
                break;

            case PRINT_MODE:

                if(*fmt == '%')
                {
                    mode = FORMAT_WARG_MODE;
                    fmt++;
                    continue;
                }

                putc(*fmt);
                fmt++;
                break;
            default:
                break;
        }
    }
}
