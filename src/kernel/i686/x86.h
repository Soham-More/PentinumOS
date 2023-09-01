#pragma once
#include <includes.h>

// bus
_import void _asmcall x86_outb(uint16_t port, uint8_t value);
_import uint8_t _asmcall x86_inb(uint16_t port);

_import void _asmcall x86_outw(uint16_t port, uint16_t value);
_import uint16_t _asmcall x86_inw(uint16_t port);

_import void _asmcall x86_outl(uint16_t port, uint32_t value);
_import uint32_t _asmcall x86_inl(uint16_t port);
