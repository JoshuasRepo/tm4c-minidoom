//*****************************************************************************
//
// main.c - Mini Doom on the EK-TM4C129EXL + Educational BoosterPack MKII (slot 2)
//
//   Joystick  : up/down = walk, left/right = turn
//   S1 (PH1)  : shoot (title screen: start)
//   S2 (PK6)  : open/close the door in front of you, restart after the end
//   Buzzer    : sound effects (sound.c, stepped every 1 ms by SysTick)
//   Display   : 3D view (raycaster) + status bar (game.c)
//
// Every pass of the loop is one frame (~30 ms, mostly sending the
// frame buffer to the display). Buttons are polled once per frame,
// which also debounces them.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"

#include "buttons.h"
#include "buzzer.h"
#include "clock.h"
#include "game.h"
#include "joystick.h"

int main(void)
{
    // System clock: 25 MHz crystal, PLL -> 120 MHz (the display driver needs 120 MHz)
    uint32_t sysClock = SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
                                           SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480,
                                           120000000);
    bool s1Last = false;
    bool s2Last = false;

    buttons_init();
    joystick_init();
    buzzer_init(sysClock);
    clock_init(sysClock);
    game_init();
    IntMasterEnable();                 // buzzer timer + SysTick

    while (1)
    {
        uint32_t now = clock_ms();
        bool s1Now = buttons_s1IsPressed();
        bool s2Now = buttons_s2IsPressed();
        int32_t joyX, joyY;

        joystick_raw(&joyX, &joyY);
        game_update(now, s1Now && !s1Last, s2Now && !s2Last, joyX, joyY);
        game_draw(now);

        s1Last = s1Now;
        s2Last = s2Now;
    }
}
