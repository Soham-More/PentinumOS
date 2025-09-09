#pragma once

#include <includes.h>

namespace IO
{
    enum KeyCode
    {
        Invalid = 0,
        Shift_L,
        Shift_R,
        ctrl_L,
        ctrl_R,
        CapsLock,
        Esc,
        Backspace,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        Enter = '\n',
        Spacebar = ' ',
        A = 'a', B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        NUM0 = '0', NUM1, NUM2, NUM3, NUM4, NUM5, NUM6, NUM7, NUM8, NUM9,
        Hyphen = '-',
        Equals = '=',
        SquareBracketOpen = '[',
        SquareBracketClose = ']',
        SemiColon = ';',
        Comma = ',',
        Dot = '.',
        Slash = '/',
        BackSlash = '\\',
        Quote = '\'',
        BackTick = '`',
    };
}

namespace PS2
{
    enum class KeyState : u8
    {
        Pressed,
        Released,
        Entered,
    };

    void init_keyboard();

    void flush_keyboard();

    void setKeyHandled(enum IO::KeyCode keyCode);

    char waitTillKeyPress();

    KeyState getKeyState(enum IO::KeyCode keyCode);
    void flushKeyState(enum IO::KeyCode keyCode);
}
