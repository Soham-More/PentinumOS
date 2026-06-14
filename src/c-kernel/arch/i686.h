#pragma once

#include <stdint.h>
#include <arch/GDT/GDT.h>
#include <arch/IDT/IDT.h>
#include <arch/ISR/ISR.h>
#include <arch/IRQ/IRQ.h>
#include <arch/x86.h>

void initialize_i686();