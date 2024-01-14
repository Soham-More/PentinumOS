#include "PS2Keyboard.hpp"

#include "PS2.hpp"
#include <i686/IRQ.h>
#include <i686/x86.h>
#include <i686/PIC.h>
#include <std/logger.h>

#define PS2_KEYBOARD_ACK 0xFA
#define PS2_KEYBOARD_RESEND 0xFE

#define _LeftShift 0x2A
#define _RightShift 0x36
#define _Enter 0x1C
#define _BackSpace 0x0E
#define _Spacebar 0x39

namespace PS2
{
    bool send_KBD_Cmd(uint8_t command)
    {
        for(uint8_t i = 0; i < 3; i++)
        {
            // send command
            PS2_out(PS2_DATA, command);

            uint8_t ack = PS2_in(PS2_DATA);

            // driver timed out
            if(ack == PS2_ERROR)
            {
                return false;
            }

            if(ack == PS2_KEYBOARD_ACK)
            {
                return true;
            }

            if(ack == PS2_KEYBOARD_RESEND)
            {
                continue;
            }
        }

        return false;
    }

    struct KeyInfo
    {
        KeyState state;
    };
    
    volatile KeyInfo keys[256] = { KeyInfo{KeyState::Released} };

    volatile uint8_t latestKey = 0;

    volatile bool waitingForKey = false;
    volatile bool shiftMode = false;

    const char ASCIITable[] = {
         0 ,  0 , '1', '2',
        '3', '4', '5', '6',
        '7', '8', '9', '0',
        '-', '=',  0 ,  0 ,
        'q', 'w', 'e', 'r',
        't', 'y', 'u', 'i',
        'o', 'p', '[', ']',
         0 ,  0 , 'a', 's',
        'd', 'f', 'g', 'h',
        'j', 'k', 'l', ';',
        '\'','`',  0 , '\\',
        'z', 'x', 'c', 'v',
        'b', 'n', 'm', ',',
        '.', '/',  0 , '*',
         0 , ' '
    };

    const char ASCIITableShift[] = {
         0 ,  0 , '!', '@',
        '#', '$', '%', '^',
        '&', '*', '(', ')',
        '_', '+',  0 ,  0 ,
        'Q', 'W', 'E', 'R',
        'T', 'Y', 'U', 'I',
        'O', 'P', '{', '}',
         0 ,  0 , 'A', 'S',
        'D', 'F', 'G', 'H',
        'J', 'K', 'L', ':',
        '\"','~',  0 , '|',
        'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<',
        '>', '?',  0 , '*',
         0 , ' '
    };

    void _no_stack_trace keyboardHandler(Registers* registers)
    {
        // get scan code(Set 1)
        uint8_t scanCode = x86_inb(0x60);

        //log_debug("Received: 0x%x\n", scanCode);

        switch (scanCode)
        {
            case _LeftShift:
                keys[IO::Shift_L].state = KeyState::Pressed;
                shiftMode = true;
                break;
            case _LeftShift + 0x80:
                keys[IO::Shift_L].state = KeyState::Released;
                shiftMode = false;
                break;
            case _RightShift:
                keys[IO::Shift_R].state = KeyState::Pressed;
                shiftMode = true;
                break;
            case _RightShift + 0x80:
                keys[IO::Shift_R].state = KeyState::Released;
                shiftMode = false;
                break;
            case _Enter:
                waitingForKey = false;
                keys[IO::Enter].state = KeyState::Entered;
                latestKey = '\n';
                break;
            case _Spacebar:
                waitingForKey = false;
                keys[IO::Spacebar].state = KeyState::Entered;
                latestKey = ' ';
                break;
            case _BackSpace:
                waitingForKey = false;
                keys[IO::Backspace].state = KeyState::Entered;
                latestKey = '\b';
                break;
            default:
                // key released
                if((scanCode & 0x80) > 0)
                {
                    keys[ASCIITable[scanCode - 0x80]].state = KeyState::Released;
                }
                else
                {
                    waitingForKey = false;
                    keys[ASCIITable[scanCode]].state = KeyState::Pressed;
                    if(shiftMode) latestKey = ASCIITableShift[scanCode];
                    else latestKey = ASCIITable[scanCode];
                }
                
                break;
        }
    }

    char waitTillKeyPress()
    {
        waitingForKey = true;

        while(waitingForKey);

        uint8_t pressedKey = latestKey;
        latestKey = 0;
        return pressedKey;
    }

    void init_keyboard()
    {
        IRQ_registerHandler(1, keyboardHandler);
        PIC_irq_unmask(1);
    }

    void flush_keyboard()
    {
        for(uint32_t i = 0; i < 256; i++)
        {
            keys[i].state = KeyState::Released;
        }
    }

    void flushKeyState(enum IO::KeyCode keyCode)
    {
        keys[keyCode].state = KeyState::Released;
    }

    KeyState getKeyState(enum IO::KeyCode keyCode)
    {
        return keys[keyCode].state;
    }
}
