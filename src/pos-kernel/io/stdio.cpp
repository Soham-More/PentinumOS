#include "stdio.h"

#include <stdarg.h>
#include <stdbool.h>
#include <c/string.h>

#include <arch/x86.h>
#include "console.hpp"

static char putc_buffer[32];
static u32 putc_buffer_pos = 0;

void clrscr()
{
    con::clear();
}

void flush()
{
    con::write(putc_buffer, putc_buffer_pos);
    putc_buffer_pos = 0;
}

void putc(char ch)
{
    if(putc_buffer_pos == sizeof(putc_buffer))
    {
        con::write(putc_buffer, 32);
        putc_buffer_pos = 0;
    }

    putc_buffer[putc_buffer_pos] = ch;
    putc_buffer_pos++;
}

void puts(const char* str)
{
    flush();
    con::write(str, strlen(str));
}

void putstr(const char* str, u32 min_width)
{
    u32 str_len = strlen(str);

    flush();
    con::write(str, str_len);

    // string was smaller than min_width, pad with spaces
    if(str_len <= min_width)
    {
        for(; str_len <= min_width; str_len++)
        {
            putc(' ');
        }
    }
}

void put_buffer(const char* str, u32 size, u32 min_width)
{
    flush();
    con::write(str, size);

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
#define INPUT_MODE 0
#define FORMAT_LARG_MODE 1 // Take Length
#define FORMAT_TARG_MODE 2 // Take type
#define FORMAT_WARG_MODE 3 // Take Width

#define LARG_NONE 0
#define LARG_SHORT 1
#define LARG_LONG 2
#define LARG_LONG_LONG 3

void print_unsigned(u64 value, u8 base, u32 min_width)
{
    char buffer[32];
    u32 buffer_pos = 0;

    const char hexChars[] = "0123456789abcdef";

    do
    {
        u64 mod = value % base;
        value /= base;
        buffer[buffer_pos++] = hexChars[mod];
    } while (value > 0);

    // if final size is less than min_width, add padding
    if(buffer_pos < min_width)
    {
        for(u32 i = buffer_pos; i < min_width; i++)
        {
            putc('0');
        }
    }

    flush();
    con::write(buffer, buffer_pos);
}

void print_signed(signed long long value, u8 base, u32 min_width)
{
    if(value < 0)
    {
        putc('-');
        value *= -1;
    }

    if(min_width == 0) min_width++;

    print_unsigned(value, base, min_width - 1);
}

void vprintf(const char* fmt, va_list vargs)
{
    u8 mode = PRINT_MODE;
    u8 larg = LARG_NONE;

    bool shouldFormatValue = false;
    u8 base = 0;
    bool isSigned = false;

    u32 width = 0;

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
                    {
                        const char* buffer = va_arg(vargs, const char*);
                        unsigned int size = va_arg(vargs, unsigned int);
                        put_buffer(buffer, size, width);
                        width = 0;
                    }
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
                            print_unsigned(va_arg(vargs, unsigned int), base, width);
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

    // flush buffers
    flush();
}

void printf(const char* fmt, ...)
{
    u8 mode = PRINT_MODE;
    u8 larg = LARG_NONE;

    bool shouldFormatValue = false;
    u8 base = 0;
    bool isSigned = false;

    u32 width = 0;

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
                    {
                        const char* buffer = va_arg(vargs, const char*);
                        unsigned int size = va_arg(vargs, unsigned int);
                        put_buffer(buffer, size, width);
                        width = 0;
                    }
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
                        putc('0');
                        putc('x');
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
                            print_unsigned(va_arg(vargs, unsigned int), base, width);
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

    flush();
}
