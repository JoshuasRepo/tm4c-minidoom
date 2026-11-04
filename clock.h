//*****************************************************************************
//
// clock.h - 1 ms time base with the SysTick interrupt
//
//*****************************************************************************
#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

void clock_init(uint32_t sysClock);   // SysTick interrupt every 1 ms
uint32_t clock_ms(void);              // milliseconds since clock_init()
void SysTickIntHandler(void);         // ISR, entered in tm4c129encpdt_startup_ccs.c

#endif
