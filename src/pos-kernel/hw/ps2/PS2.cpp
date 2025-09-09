#include "PS2.hpp"

#include <arch/x86.h>
#include <io/io.h>
#include "../pit/PIT.h"

#define PS2_CCB_READ 0x20
#define PS2_CCB_WRITE 0x60
#define PS2_COMMAND 0x64
#define PS2_STATUS_REGISTER 0x64

#define PS2_COMMAND_ACK 0xFA

enum PS2_CCB_FLAGS
{
    PS2_FIRST_INTERRUPT_ENABLE = 0x1,
    PS2_SECOND_INTERRUPT_ENABLE = 0x2,
    PS2_SYSTEM_POST = 0x4,
    PS2_FIRST_PORT_CLK_DISABLE = 0x10,
    PS2_SECOND_PORT_CLK_DISABLE = 0x20,
    PS2_FIRST_PORT_TRANSLATION = 0x40,
};

u8 PS2_out(u8 port, u8 byte)
{
    PIT_setTimeout(3);

    // wait till status in is clear or till timeout
    while((x86_inb(PS2_STATUS_REGISTER) & 0x02) > 0 && !PIT_hasTimedOut());

    if(PIT_hasTimedOut())
    {
        log_error("PS2 Driver Timeout\n");
        return 0xFC;
    }
    else
    {
        x86_outb(port, byte);
    }

    return 0;
}

u8 PS2_in(u8 port)
{
    PIT_setTimeout(3);

    // wait till status out is set or timeout
    while((x86_inb(PS2_STATUS_REGISTER) & 0x01) == 0 && !PIT_hasTimedOut());

    if(PIT_hasTimedOut())
    {
        log_error("\nPS2 Driver Timeout\n");
        return 0xFC;
    }

    return x86_inb(port);
}

void PS2_init()
{
    // disable devices
    PS2_out(PS2_COMMAND, 0xAD);
    PS2_out(PS2_COMMAND, 0xA7);

    // flush buffer
    x86_inb(PS2_DATA);

    // set CCB
    PS2_out(PS2_COMMAND, PS2_CCB_READ);
    u8 ccb = PS2_in(PS2_DATA);

    // disable IRQs and translation from PS2
    ccb &= ~( PS2_FIRST_INTERRUPT_ENABLE + PS2_SECOND_INTERRUPT_ENABLE + PS2_FIRST_PORT_TRANSLATION );

    PS2_out(PS2_COMMAND, PS2_CCB_WRITE);
    PS2_out(PS2_DATA, ccb);

    bool isDualChannel = false;

    if((ccb & (1 << 5)) > 0)
    {
        isDualChannel = true;
    }

    // perform PS2 self test
    PS2_out(PS2_COMMAND, 0xAA);
    u8 test = PS2_in(PS2_DATA);

    if(test != 0x55)
    {
        log_warn("PS2 self test failed: test = %2x\n", test);
        return;
    }

    // enable PS2 port 1
    PS2_out(PS2_COMMAND, 0xAE);
    PS2_out(PS2_COMMAND, 0xA8);

    // enable IRQs and translation
    ccb |= PS2_FIRST_INTERRUPT_ENABLE | PS2_FIRST_PORT_TRANSLATION;
    PS2_out(PS2_COMMAND, PS2_CCB_WRITE);
    PS2_out(PS2_DATA, ccb);

    // reset PS2 1 controller
    PS2_out(PS2_DATA, 0xFF);

    u8 ack = PS2_in(PS2_DATA);

    if(ack == PS2_COMMAND_ACK)
    {
        if(PS2_in(PS2_DATA) != 0xAA)
        {
            log_warn("PS2 reset failed: ccb = %2x\n", ccb);
            return;
        }
    }
    else
    {
        log_warn("PS2 reset failed: ccb = %2x\n", ccb);
        return;
    }
}
