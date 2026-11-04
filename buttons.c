//*****************************************************************************
//
// buttons.c - BoosterPack buttons S1 (PH1) and S2 (PK6), read by polling
//
// The pull-up resistors are on the BoosterPack. The buttons are active low:
// released = 1, pressed = 0.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "buttons.h"

void buttons_init(void)
{
    // switch on the clock of port H and K and wait until they are ready
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOH) ||
           !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK))
    {
    }

    GPIOPinTypeGPIOInput(GPIO_PORTH_BASE, GPIO_PIN_1);   // S1
    GPIOPinTypeGPIOInput(GPIO_PORTK_BASE, GPIO_PIN_6);   // S2
}

bool buttons_s1IsPressed(void)
{
    return GPIOPinRead(GPIO_PORTH_BASE, GPIO_PIN_1) == 0;   // pressed = 0
}

bool buttons_s2IsPressed(void)
{
    return GPIOPinRead(GPIO_PORTK_BASE, GPIO_PIN_6) == 0;   // pressed = 0
}
