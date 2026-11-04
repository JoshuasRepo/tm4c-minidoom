//*****************************************************************************
//
// buttons.h - BoosterPack buttons S1 (PH1) and S2 (PK6), read by polling
//
//*****************************************************************************
#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>

void buttons_init(void);        // PH1 and PK6 as inputs
bool buttons_s1IsPressed(void); // true while S1 is pressed
bool buttons_s2IsPressed(void); // true while S2 is pressed

#endif
