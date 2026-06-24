#include "gfmt.h"

bool gfmt_is_num(u8 c) {
    return (c >= (u8)'0') && (c <= (u8)'9');
}
usize gfmt_cstr_len(const char* cstr) {
    usize len = 0;
    while(cstr[len]) len++;
    return len;
}

void gfmt_putcstr(void* ctx, const char* cstr) {
    while(*cstr) {
        gfmt_putc(ctx, *cstr);
        cstr++;
    }
}
void gfmt_putstr(void* ctx, const char* cstr, usize len) {
    for(usize i = 0; i < len; i++) {
        gfmt_putc(ctx, cstr[i]);
    }
}

// base can only be 2, 10, 16
// returns the length of the number
u8 gfmt_format_num_buf(u32 value, u8 buf[32], u8 base, bool use_capital) {
    const u8 map_small[] = "0123456789abcdef";
    const u8 map_big[] = "0123456789ABCDEF";

    const u8* map = use_capital ? map_big : map_small;

    if(value == 0) { buf[0] = '0'; return 1; }

    usize i = 0;
    for(; value; i++) {
        buf[i] = map[value % base];
        value = value / base;
    }
    usize len = i;
    // reverse the buffer
    for(usize j = 0; j < len / 2; j++) {
        u8 temp = buf[j];
        buf[j] = buf[len - 1 - j];
        buf[len - 1 - j] = temp;
    }
    return len;
}

#define GFMT_OK 0
#define GFMT_ERR_NOTYPE 1
#define GFMT_ERR_UNEXP_TYPE 2
#define GFMT_ERR_UNEXP_SIZE 3
#define GFMT_ERR_USUP_TYPE 4
const char* gfmt_error_strings[] = {
    "all ok",
    "no type specified",
    "unexpected character after datatype",
    "unexpected character after datasize",
    "unsupported datatype(only uixXvsSczp)",
};

gfmt_fmt_spec gfmt_create_spec(u8 error) {
    return (gfmt_fmt_spec) {
        .error=error, 
        .bytesize = 0, 
        .type=0, 
        .width=0,
        .sign='*',
        .align='<',
        .fill=' ',
    };
}

gfmt_fmt_spec gfmt_parse_fmt_spec(const char* fmt_spec, usize size) {
/*
What do I need:
decimal, hexadecimal, pointer, error?, bitfield view

types: u8, u16, u32(default), u64, usize, 

format spec: {[width][type][size]%[sign][align[fill]]}

{
type: (u, i, x, X, v) v=bitfield repr
size: (8, 16, 32, 64)
%
sign=+,space,*, align=<, >, fill character
}
*/
#define GFMT_PRE_WIDTH 0x0
#define GFMT_PRE_TYPE  0x1
#define GFMT_PRE_SIZE  0x2
#define GFMT_PRE_DONE  0x3

    gfmt_fmt_spec spec = gfmt_create_spec(GFMT_OK);
    
    u32 processed_count = 0;
    const char* c = fmt_spec;
    u8 state = GFMT_PRE_WIDTH;
    while(processed_count < size && *c != '%'){
        switch (state) {
            case GFMT_PRE_WIDTH:
                if(!gfmt_is_num(*c)) {
                    state = GFMT_PRE_TYPE;
                    __attribute__((fallthrough));
                } else {
                    spec.width = (spec.width * 10) + (*c - (u8)'0');
                    break;
                }
            case GFMT_PRE_TYPE:
                spec.type = *c;
                state = GFMT_PRE_SIZE;
                break;
            case GFMT_PRE_SIZE:
                if(!gfmt_is_num(*c)) {
                    state = GFMT_PRE_DONE;
                } else {
                    spec.bytesize = (spec.bytesize * 10) + (*c - (u8)'0');
                }
                break;
            default:
                break;
        }
        c++;
        processed_count++;
    }

    // validate the type
    const char accepted_types[] = "uixXvsSczp";
    bool is_ok = false;
    for(usize i = 0; i < sizeof(accepted_types) - 1; i++) {
        is_ok |= (accepted_types[i] == spec.type);
    }
    if(!is_ok) return gfmt_create_spec(GFMT_ERR_USUP_TYPE);

    if(spec.bytesize == 0) spec.bytesize = 32; // default size

    // validate the size
    if((spec.bytesize & (spec.bytesize - 1)) != 0 || spec.bytesize < 8 || spec.bytesize > 64) {
        return gfmt_create_spec(GFMT_ERR_UNEXP_SIZE);
    }

// %[sign][align[fill]]
// sign=+,space,*, align=<, >, fill character

    if(*c == '%') {processed_count++; c++;}
    u8 op_size = size - processed_count;
    
    if(op_size > 0) {
        switch (*c)
        {
        case '*':
        case '+':
        case ' ':
            spec.sign = *c; 
            c++; op_size--;
            __attribute__((fallthrough));
        case '<':
        case '>':
            if(op_size == 0) break;
            spec.align = *c;
            c++; op_size--;
            __attribute__((fallthrough));
        default:
            if(op_size == 0) break;
            spec.fill = *c;
            break;
        }
    }

    return spec;
}

void gfmt_print_formatted_string(void* ctx, const char* str, usize len, gfmt_fmt_spec spec) {
    // spec.width, spec.align, spec.fill
    usize padding = spec.width > len ? spec.width - len : 0;
    if(spec.align == '>') {
        for(usize i = 0; i < padding; i++) {
            gfmt_putc(ctx, spec.fill);
        }
    }
    gfmt_putstr(ctx, str, len);
    if(spec.align == '<') {
        for(usize i = 0; i < padding; i++) {
            gfmt_putc(ctx, spec.fill);
        }
    }
}

void gfmt_print_spec(void* ctx, gfmt_fmt_spec spec, va_list* vargs) {
    // print variable according to spec
    if(spec.error != GFMT_OK) {
        gfmt_putc(ctx, '\n');
        gfmt_putcstr(ctx, "[gfmt_printer] invalid format specifier: ");
        gfmt_putcstr(ctx, gfmt_error_strings[spec.error]);
        gfmt_putc(ctx, '\n');
        return;
    }

    const char* str;
    u8 buf[32];
    usize len = 0;
    usize base = 0;
    bool is_negative = false;

    switch (spec.type)
    {
    case 's':
        // print cstring
        str = va_arg(*vargs, const char*);
        gfmt_print_formatted_string(ctx, str, gfmt_cstr_len(str), spec);
        return;
    case 'S':
        // print string of length
        str = va_arg(*vargs, const char*);
        len = va_arg(*vargs, usize);
        gfmt_print_formatted_string(ctx, str, len, spec);
        return;
    case 'c':
        // print char
        char ch = (char)va_arg(*vargs, int);
        gfmt_print_formatted_string(ctx, &ch, 1, spec);
        return;
    case 'x':
    case 'X':
    case 'u':
        if(spec.type == 'x' || spec.type == 'X') {
            base = 16;
        } else {
            base = 10;
        }

        if(spec.bytesize <= 32) {
            u32 value = va_arg(*vargs, u32);
            len = gfmt_format_num_buf(value, buf, base, spec.type == 'X');
            gfmt_print_formatted_string(ctx, (const char*)buf, len, spec);
        } else if (spec.bytesize == 64) {
            u64 value = va_arg(*vargs, u64);
            len = gfmt_format_num_buf(value, buf, base, spec.type == 'X');
            gfmt_print_formatted_string(ctx, (const char*)buf, len, spec);
            return;
        }
        return;
    case 'i':
        if(spec.bytesize <= 32) {
            i32 value = va_arg(*vargs, i32);
            len = gfmt_format_num_buf(value < 0 ? -value : value, buf, 10, false);
            is_negative = value < 0;
        } else if (spec.bytesize == 64) {
            i64 value = va_arg(*vargs, i64);
            len = gfmt_format_num_buf(value < 0 ? -value : value, buf, 10, false);
            is_negative = value < 0;
        }
        
        if(is_negative) {
            gfmt_putc(ctx, '-');
        } else if (spec.sign == '+') {
            gfmt_putc(ctx, '+');
        } else if (spec.sign == ' ') {
            gfmt_putc(ctx, ' ');
        }
        gfmt_print_formatted_string(ctx, (const char*)buf, len, spec);
        return;
    case 'p':
        // print pointer as hex
        if(spec.bytesize <= 32) {
            u32 value = va_arg(*vargs, u32);
            len = gfmt_format_num_buf(value, buf, 16, false);
        } else if (spec.bytesize == 64) {
            u64 value = va_arg(*vargs, u64);
            len = gfmt_format_num_buf(value, buf, 16, false);
        }
        {
            u8 big_buf[64];
            big_buf[0] = '0';
            big_buf[1] = 'x';
            usize zero_padding = (spec.bytesize / 4) > len ? (spec.bytesize / 4) - len : 0;
            for(usize i = 0; i < zero_padding; i++) {
                big_buf[2 + i] = '0';
            }
            for(usize i = 0; i < len; i++) {
                big_buf[2 + zero_padding + i] = buf[i];
            }
            gfmt_print_formatted_string(ctx, (const char*)big_buf, 2 + zero_padding + len, spec);
        }
        return;
    case 'v':
        // TODO: bitfield view
        return;
    }
}

void gfmt_printer(void* ctx, const char* fmt, usize len, va_list* vargs) {
    bool in_spec = false;
    usize spec_start = 0;

    for(usize i = 0; i < len; i++) {
        if(in_spec) {
            if(fmt[i] == '{') {
                if(spec_start - i > 0) {
                    gfmt_putc(ctx, '\n');
                    gfmt_putcstr(ctx, "[gfmt_printer] invalid format specifier: unexpected '{' in specifier\n");
                    return;
                }
                gfmt_putc(ctx, '{');
                in_spec = false;
                continue;
            } else if(fmt[i] == '}') {
                in_spec = false;
                gfmt_fmt_spec spec = gfmt_parse_fmt_spec(fmt + spec_start, i - spec_start);
                gfmt_print_spec(ctx, spec, vargs);
            } else {
                continue;
            }
        } else if(fmt[i] == '{') {
            in_spec = true;
            spec_start = i + 1;
        } else if(fmt[i] == '}') {
            if(i + 1 < len && fmt[i + 1] == '}') {
                gfmt_putc(ctx, '}');
                i++;
                continue;
            }
            gfmt_putc(ctx, '\n');
            gfmt_putcstr(ctx, "[gfmt_printer] invalid format specifier: unexpected '}'\n");
            return;
        } else {
            gfmt_putc(ctx, fmt[i]);
        }
    }
}
void gfmt_print(void* ctx, const char* fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);
    gfmt_printer(ctx, fmt, gfmt_cstr_len(fmt), &vargs);
    va_end(vargs);
}


