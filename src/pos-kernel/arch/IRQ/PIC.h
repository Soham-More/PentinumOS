#pragma once

#include <includes.h>

#define REMAP_PIC_OFFSET 0x20

void init_pic();

void PIC_EOI(u8 irq);

void PIC_irq_mask(u8 irq_line);
void PIC_irq_unmask(u8 irq_line);

u16 PIC_get_isr();
u16 PIC_get_irr();
