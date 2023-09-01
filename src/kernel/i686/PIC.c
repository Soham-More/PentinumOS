#include "PIC.h"

#include "x86.h"

#define MASTER_PIC_COMMAND 0x20
#define MASTER_PIC_DATA 0x21
#define SLAVE_PIC_COMMAND 0x20
#define SLAVE_PIC_DATA 0x21

#define PIC_EOI_VAL 0x20

/* reinitialize the PIC controllers, giving them specified vector offsets
   rather than 8h and 70h, as configured by default */
 
#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */
 
#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

#define PIC_READ_IRR                0x0a    /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR                0x0b    /* OCW3 irq service next CMD read */

void io_wait()
{
    x86_outb(0x80, 0);
}

/* Helper func */
static uint16_t __pic_get_irq_reg(int ocw3)
{
    /* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
     * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
    x86_outb(MASTER_PIC_COMMAND, ocw3);
    x86_outb(SLAVE_PIC_COMMAND, ocw3);
    return (x86_inb(SLAVE_PIC_COMMAND) << 8) | x86_inb(MASTER_PIC_COMMAND);
}

/*
arguments:
	offset1 - vector offset for master PIC
		vectors on the master become offset1..offset1+7
	offset2 - same for slave PIC: offset2..offset2+7
*/
void PIC_remap(int offset1, int offset2)
{
	x86_outb(MASTER_PIC_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
    io_wait();
	x86_outb(SLAVE_PIC_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
	x86_outb(MASTER_PIC_DATA, offset1);                 // ICW2: Master PIC vector offset
    io_wait();
	x86_outb(SLAVE_PIC_DATA, offset2);                 // ICW2: Slave PIC vector offset
    io_wait();
	x86_outb(MASTER_PIC_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    io_wait();
	x86_outb(SLAVE_PIC_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();
 
	x86_outb(MASTER_PIC_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    io_wait();
	x86_outb(SLAVE_PIC_DATA, ICW4_8086);
    io_wait();
 
    // mask all
	x86_outb(MASTER_PIC_DATA, 0xFF);
    io_wait();
	x86_outb(SLAVE_PIC_DATA, 0xFF);
    io_wait();
}

void init_pic()
{
    // remap pic
    PIC_remap(REMAP_PIC_OFFSET, REMAP_PIC_OFFSET + 8);
}

void PIC_irq_mask(uint8_t irq_line)
{
    uint16_t port = 0;

    if(irq_line < 8)
    {
        port = MASTER_PIC_DATA;
    }
    else
    {
        port = SLAVE_PIC_DATA;
        irq_line -= 8;
    }

    uint8_t mask = x86_inb(port);
    x86_outb(port, mask | (1 << irq_line));
}

void PIC_irq_unmask(uint8_t irq_line)
{
    uint16_t port = 0;

    if(irq_line < 8)
    {
        port = MASTER_PIC_DATA;
    }
    else
    {
        port = SLAVE_PIC_DATA;
        irq_line -= 8;
    }

    uint8_t mask = x86_inb(port);
    x86_outb(port, mask & ~(1 << irq_line));
}

void PIC_EOI(uint8_t irq)
{
    // came from slave PIC
    if(irq >= 8)
        x86_outb(SLAVE_PIC_COMMAND, PIC_EOI_VAL);

    x86_outb(MASTER_PIC_COMMAND, PIC_EOI_VAL);
}

uint16_t PIC_get_isr()
{
    return __pic_get_irq_reg(PIC_READ_ISR);
}

uint16_t PIC_get_irr()
{
    return __pic_get_irq_reg(PIC_READ_IRR);
}
