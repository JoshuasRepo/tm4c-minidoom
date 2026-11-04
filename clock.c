//*****************************************************************************
//
// clock.c - 1 ms time base with the SysTick interrupt
//
// Counts real milliseconds (also while the display is being drawn) and
// steps the sound effects (sound.c) with exact timing.
//
// SysTick load = sysClock / 1000 = 120 000 ticks = 1 ms
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/systick.h"

#include "clock.h"
#include "sound.h"

static volatile uint32_t milliseconds = 0;   // changed in the ISR -> volatile

void clock_init(uint32_t sysClock)
{
    SysTickPeriodSet(sysClock / 1000);       // 1 ms
    SysTickIntEnable();
    SysTickEnable();
}

uint32_t clock_ms(void)
{
    return milliseconds;
}

void SysTickIntHandler(void)
{
    milliseconds++;                          // SysTick clears its flag itself
    sound_tick();                            // step the sound effects every 1 ms
}
