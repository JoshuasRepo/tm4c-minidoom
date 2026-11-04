//*****************************************************************************
//
// joystick.h - BoosterPack MKII joystick: X = PD2 (AIN13), Y = PK1 (AIN17),
//              push button SEL = PD4 (active low)
//
//*****************************************************************************
#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>
#include <stdbool.h>

// direction of the stick (only one at a time, the stronger axis wins)
typedef enum
{
    JOY_NONE,
    JOY_UP,
    JOY_DOWN,
    JOY_LEFT,
    JOY_RIGHT
} JoyDir;

void joystick_init(void);
JoyDir joystick_direction(void);    // current direction (with hysteresis)
bool joystick_isPressed(void);      // true while the stick is pressed down
void joystick_raw(int32_t *x, int32_t *y);   // both axes, centre = 0 (-2048..2047)

#endif
