#pragma once

#include <includes.h>

// io bus
_import void _asmcall x86_outb(u16 port, u8 value);
_import u8 _asmcall x86_inb(u16 port);

_import void _asmcall x86_outw(u16 port, u16 value);
_import u16 _asmcall x86_inw(u16 port);

_import void _asmcall x86_outl(u16 port, u32 value);
_import u32 _asmcall x86_inl(u16 port);

// cache and paging

// returns cr3 value
_import u32 _asmcall x86_flushTLB();

void _cdecl x86_set_page_directory(void* page_directory_ptr);
u32 _cdecl x86_get_cr0_register();
u32 _cdecl x86_get_cr3_register();

_import u32 _asmcall x86_flushCache();

// interrupts and exceptions
_import void _asmcall x86_Panic();
_import void _asmcall x86_EnableInterrupts();
_import void _asmcall x86_DisableInterrupts();

_import void _asmcall x86_raise(u32 error_code);

