#include "stdio.h"

#include <stdarg.h>
#include <stdbool.h>

#include <i686/x86.h>
#include <Drivers/Drivers.h>
#include <std/memory.h>

#define IO_MODE_OUT 0
#define IO_MODE_IN  1

static uint8_t std_ostream = STDIO_Video;
static uint8_t std_iomode  = IO_MODE_OUT;

static VGA::Postion lastOUTPosition = VGA::Postion{0 , 0};

void clrscr()
{
    VGA::clearScreen();
}

void clrline()
{
    VGA::clearLine();
}

void putc(char ch)
{
    if(std_ostream == STDIO_Video) VGA::putc(ch);
    else if(std_ostream == STDIO_E9) DCOM::putc(ch);
    else if(std_ostream == STDERR) { VGA::putc(ch); DCOM::putc(ch); }

    if(std_iomode == IO_MODE_OUT) lastOUTPosition = VGA::getCursorPositon();
}

void puts(const char* str)
{
    for(; *str; str++)
    {
        putc(*str);
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

void vprintf(const char* fmt, va_list vargs)
{
    uint8_t mode = PRINT_MODE;
    uint8_t larg = LARG_NONE;

    bool shouldFormatValue = false;
    uint8_t base = 0;
    bool isSigned = false;

    uint32_t width = 0;

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
}

char getch()
{
	char key = PS2::waitTillKeyPress();

	if(PS2::getKeyState(IO::Enter) == PS2::KeyState::Entered)
	{
		PS2::flushKeyState(IO::Enter);
		return '\n';
	}

	if(PS2::getKeyState(IO::Spacebar) == PS2::KeyState::Entered)
	{
		PS2::flushKeyState(IO::Spacebar);
		return ' ';
	}

	PS2::KeyState keyState = PS2::getKeyState(IO::BackSlash);

	if(keyState == PS2::KeyState::Entered)
	{
		PS2::flushKeyState(IO::BackSlash);
		return '\b';
	}

	return key;
}

char getchar()
{
    std_iomode = IO_MODE_IN;

	char c = getch();

    if(c == '\b')
    {
        // if it is not currently at last printed position
        // then back
        if(lastOUTPosition.x < VGA::getCursorPositon().x)
        {
            putc(c);
        }
    }
    else
    {
        putc(c);
    }

    std_iomode = IO_MODE_OUT;
	
	return c;
}

void scanf(const char* fmt, ...)
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
}


void set_ostream(uint8_t out)
{
    std_ostream = out;
}
