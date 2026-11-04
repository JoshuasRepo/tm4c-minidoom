//*****************************************************************************
//
// sound.c - non-blocking sound effects on the buzzer
//
// A sound is a list of notes {freq, duration in ms}, ending with {0, 0}.
// freq = 0 inside the list means silence. sound_tick() runs every 1 ms in
// the SysTick ISR and switches to the next note when the time is over, so
// the game loop never waits for a sound.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/interrupt.h"

#include "buzzer.h"
#include "sound.h"

static const Note *volatile current = 0;   // sound that is playing (0 = none)
static volatile uint32_t remaining = 0;     // ms left of the current note

const Note SND_SHOOT[] = { {1400, 25}, {900, 25}, {600, 30}, {350, 40}, {0, 0} };
const Note SND_HIT[]   = { {300, 40}, {180, 60}, {0, 0} };
const Note SND_KILL[]  = { {500, 60}, {380, 60}, {260, 80}, {160, 100}, {100, 150}, {0, 0} };
const Note SND_HURT[]  = { {130, 80}, {0, 20}, {100, 100}, {0, 0} };
const Note SND_DOOR[]  = { {220, 60}, {260, 60}, {330, 80}, {0, 0} };
const Note SND_ALERT[] = { {180, 70}, {240, 90}, {0, 0} };
const Note SND_DEAD[]  = { {392, 300}, {330, 300}, {262, 300}, {196, 700}, {0, 0} };
const Note SND_START[] = { {262, 100}, {330, 100}, {392, 100}, {523, 250}, {0, 0} };

static void startNote(const Note *n)
{
    remaining = n->duration;
    if (n->freq == 0)
    {
        buzzer_stop();
    }
    else
    {
        buzzer_start(n->freq);
    }
}

void sound_play(const Note *seq)
{
    IntMasterDisable();          // the ISR must not step the old sound meanwhile
    current = seq;
    startNote(seq);
    IntMasterEnable();
}

void sound_tick(void)
{
    if (current == 0)
    {
        return;
    }
    if (remaining > 0)
    {
        remaining--;
    }
    if (remaining == 0)
    {
        current++;                       // next note
        if (current->duration == 0)
        {
            current = 0;                 // end of the list
            buzzer_stop();
        }
        else
        {
            startNote(current);
        }
    }
}
