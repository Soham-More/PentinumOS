#include "vga_txt.h"

#include "../priv.h"
#include <io/console.h>

#include <arch/x86.h>

#define currPositionChar  (2 * (screenY * SCREEN_WIDTH + screenX) + 0)
#define currPositionColor (2 * (screenY * SCREEN_WIDTH + screenX) + 1)
#define videoBufferSize   (2 * ((SCREEN_HEIGHT - 1) * SCREEN_WIDTH + SCREEN_WIDTH))

// VGA defines
#define CRT_Controller_Address_Register 0x3D4
#define CRT_Controller_Data_Register    0x3D5

#define cursorLocationRegisterAddressLow   0x0F
#define cursorLocationRegisterAddressHigh  0x0E

#define SCREEN_WIDTH  ((u32)80)
#define SCREEN_HEIGHT ((u32)25)

u8 currentColor = 0xF;  // bright white
u8 tabSize      = 4;
u32 screenX = 0;
u32 screenY = 0;

u8* svideoBuffer = (u8*)0xB8000;

// Set position of cursor to (x, y)
void vgatxt_set_cursor_pos(u32 x, u32 y) {
    // calculate linear position
    int lpos = (y * SCREEN_WIDTH + x);

    // tell VGA that we are setting cursor location(low byte) register
    x86_outb(CRT_Controller_Address_Register, cursorLocationRegisterAddressLow);
    // set lower byte of cursor location register to lower byte of linear positon
    x86_outb(CRT_Controller_Data_Register, (u8)(lpos & 0xFF));

    // tell VGA that we are setting cursor location(high byte) register
    x86_outb(CRT_Controller_Address_Register, cursorLocationRegisterAddressHigh);
    // set higher byte of cursor location register to higher byte of linear positon
    x86_outb(CRT_Controller_Data_Register, (u8)((lpos >> 8) & 0xFF));
}

// shift buffer up by lines amount
void vgatxt_scroll_up(u32 lines) {
    u32 shift = (2 * lines * SCREEN_WIDTH);
    u8* shiftedBuffer = svideoBuffer + shift;

    for(u32 i = 0; i < videoBufferSize; i += 2) {
        if(i < videoBufferSize - shift) {
            svideoBuffer[i + 0] = shiftedBuffer[i + 0];
            svideoBuffer[i + 1] = shiftedBuffer[i + 1];
        } else {
            svideoBuffer[i + 0] = 0;
            svideoBuffer[i + 1] = currentColor;
        }
    }
}

void vgatxt_clr_screen() {
    for(u32 i = 0; i < videoBufferSize; i += 2) {
        svideoBuffer[i + 0] = 0;
        svideoBuffer[i + 1] = currentColor;
    }

    screenX = 0;
    screenY = 0;

    vgatxt_set_cursor_pos(screenX, screenY);
}
void vgatxt_clear_line() {
    for(u32 i = 0; i < SCREEN_WIDTH; i++) {
        screenX = i;

        svideoBuffer[currPositionChar] = 0;
        svideoBuffer[currPositionColor] = currentColor;
    }

    screenX = 0;

    vgatxt_set_cursor_pos(screenX, screenY);
}

void vgatxt_set_txt_color(u8 vga_color) {
    vga_color &= 0x0F;
    currentColor = (currentColor & 0xF0) + vga_color;
}
void vgatxt_set_txt_bgcolor(u8 vga_color) {
    vga_color &= 0x0F;
    currentColor = (currentColor & 0x0F) + (vga_color << 4);
}

void vgatxt_putc(char ch) {
    switch (ch) {
        case '\n':
            screenY++;
            screenX = 0;
            break;
        case '\t':
            for (int i = 0; i < tabSize - (screenX % tabSize); i++)
                vgatxt_putc(' ');
            break;
        case '\r':
            screenX = 0;
            break;
        case '\b':
            if(screenX > 0) {
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
    if(screenX >= SCREEN_WIDTH) {
        screenY++;
        screenX = 0;
    }

    // reached y limit, scroll up
    if(screenY >= SCREEN_HEIGHT) {
        screenY = SCREEN_HEIGHT - 1;
        vgatxt_scroll_up(1);
    }

    vgatxt_set_cursor_pos(screenX, screenY);
}
void vgatxt_write(console_t* console, const char* buffer, usize len) {
    vgatxt_set_txt_color(console->text_color & 0xF);

    for(size_t i = 0; i < len; i++) vgatxt_putc(buffer[i]);
}
void vgatxt_clear(console_t* console) {
    vgatxt_clr_screen();
}

console_t vga_txt_get_earlyconsole() {
    screenX = 0;
    screenY = 0;

    console_t vgatxt_console = {
        .name = "vga-txt0",
        .write = vgatxt_write,
        .read = con_ignore_read,
        .clear = vgatxt_clear,
        .text_color = CON_COLOR_WHITE,
        .data = nullptr,
    };
    return vgatxt_console;
}


err_t initialize_vga_txt_drivers(){
    heap_allocator_t* alloca = get_driver_allocator();

    console_t vgatxt_console = {
        .name = "vgatxt0",
        .write = vgatxt_write,
        .read = con_ignore_read,
        .clear = vgatxt_clear,
        .text_color = CON_COLOR_WHITE,
        .data = nullptr,
    };

    // register a new console
    return register_console(alloca, vgatxt_console);
}
