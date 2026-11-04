//*****************************************************************************
//
// sound.h - non-blocking sound effects on the buzzer
//
//*****************************************************************************
#ifndef SOUND_H
#define SOUND_H

#include "melodies.h"     // Note {freq, duration}, list ends with duration 0

void sound_play(const Note *seq);   // start a sound (replaces the current one)
void sound_tick(void);              // called every 1 ms from the SysTick ISR

// the effects of the game
extern const Note SND_SHOOT[];
extern const Note SND_HIT[];
extern const Note SND_KILL[];
extern const Note SND_HURT[];
extern const Note SND_DOOR[];
extern const Note SND_ALERT[];
extern const Note SND_DEAD[];
extern const Note SND_START[];

#endif
