#pragma once

#include <includes.h>

#define REMAP_PIC_OFFSET 0x20

void init_pic();

void PIC_EOI(uint8_t irq);

void PIC_irq_mask(uint8_t irq_line);
void PIC_irq_unmask(uint8_t irq_line);

uint16_t PIC_get_isr();
uint16_t PIC_get_irr();
