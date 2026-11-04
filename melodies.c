//*****************************************************************************
//
// melodies.c - The melody table
//
// Every melody ends with {0, 0}. The main loop stops when it reaches it.
//
//*****************************************************************************

#include "melodies.h"

// note lengths for "Last Christmas" (about 107 beats per minute -> quarter = 560 ms)
#define LC_E    280    // eighth
#define LC_Q    560    // quarter
#define LC_DQ   840    // dotted quarter
#define LC_H   1120    // half

const Note melodies[MELODY_COUNT][MAX_NOTES] =
{
    // melody 0: "Alle meine Entchen"
    {
        {C4, QUARTER}, {D4, QUARTER}, {E4, QUARTER}, {F4, QUARTER},
        {G4, HALF}, {G4, HALF},
        {A4, QUARTER}, {A4, QUARTER}, {A4, QUARTER}, {A4, QUARTER},
        {G4, WHOLE},
        {A4, QUARTER}, {A4, QUARTER}, {A4, QUARTER}, {A4, QUARTER},
        {G4, WHOLE},
        {F4, QUARTER}, {F4, QUARTER}, {F4, QUARTER}, {F4, QUARTER},
        {E4, HALF}, {E4, HALF},
        {D4, QUARTER}, {D4, QUARTER}, {D4, QUARTER}, {D4, QUARTER},
        {C4, WHOLE},
        {0, 0}
    },

    // melody 1: "Haenschen klein"
    {
        {G4, QUARTER}, {E4, QUARTER}, {E4, HALF},
        {F4, QUARTER}, {D4, QUARTER}, {D4, HALF},
        {C4, QUARTER}, {D4, QUARTER}, {E4, QUARTER}, {F4, QUARTER},
        {G4, QUARTER}, {G4, QUARTER}, {G4, HALF},
        {G4, QUARTER}, {E4, QUARTER}, {E4, HALF},
        {F4, QUARTER}, {D4, QUARTER}, {D4, HALF},
        {C4, QUARTER}, {E4, QUARTER}, {G4, QUARTER}, {G4, QUARTER},
        {C4, WHOLE},
        {0, 0}
    },

    // melody 2: "Ode to Joy" (Beethoven)
    {
        {E4, QUARTER}, {E4, QUARTER}, {F4, QUARTER}, {G4, QUARTER},
        {G4, QUARTER}, {F4, QUARTER}, {E4, QUARTER}, {D4, QUARTER},
        {C4, QUARTER}, {C4, QUARTER}, {D4, QUARTER}, {E4, QUARTER},
        {E4, 450}, {D4, 150}, {D4, HALF},
        {E4, QUARTER}, {E4, QUARTER}, {F4, QUARTER}, {G4, QUARTER},
        {G4, QUARTER}, {F4, QUARTER}, {E4, QUARTER}, {D4, QUARTER},
        {C4, QUARTER}, {C4, QUARTER}, {D4, QUARTER}, {E4, QUARTER},
        {D4, 450}, {C4, 150}, {C4, HALF},
        {0, 0}
    },

    // melody 3: Super Mario Bros. "course clear" fanfare (flag reached)
    {
        // C major: fast run up, then two long notes
        {G3, FAST}, {C4, FAST}, {E4, FAST}, {G4, FAST}, {C5, FAST}, {E5, FAST},
        {G5, QUARTER}, {E5, QUARTER},
        // A flat major: same pattern
        {GS3, FAST}, {C4, FAST}, {DS4, FAST}, {GS4, FAST}, {C5, FAST}, {DS5, FAST},
        {GS5, QUARTER}, {DS5, QUARTER},
        // B flat major: run up, three short notes, long final note
        {AS3, FAST}, {D4, FAST}, {F4, FAST}, {AS4, FAST}, {D5, FAST}, {F5, FAST},
        {AS5, QUARTER},
        {AS5, FAST}, {AS5, FAST}, {AS5, FAST},
        {C6, WHOLE},
        {0, 0}
    },

    // melody 4: "Last Christmas" (Wham!), chorus
    // F major; notes from noobnotes.net (letter notes), rhythm from the lyrics
    {
        // "Last Christmas, I gave you my heart"
        {G4, LC_Q}, {G4, LC_DQ}, {F4, LC_E}, {C4, LC_E},
        {G4, LC_E}, {G4, LC_E}, {A4, LC_E}, {F4, LC_H},
        // "but the very next day you gave it away"
        {D4, LC_E}, {D4, LC_E}, {G4, LC_E}, {G4, LC_E}, {A4, LC_E}, {F4, LC_Q},
        {D4, LC_E}, {E4, LC_E}, {F4, LC_E}, {E4, LC_E}, {D4, LC_H},
        // "this year, to save me from tears"
        {A4, LC_Q}, {G4, LC_DQ}, {D4, LC_E}, {A4, LC_E},
        {AS4, LC_E}, {A4, LC_E}, {G4, LC_H},
        // "I'll give it to someone special"
        {F4, LC_E}, {E4, LC_E}, {F4, LC_E}, {E4, LC_Q},
        {E4, LC_E}, {F4, LC_E}, {E4, LC_E}, {C4, LC_H},
        {0, 0}
    }
};

const char *const melodyNames[MELODY_COUNT] =
{
    "Alle meine Entchen",
    "Haenschen klein",
    "Ode to Joy",
    "Super Mario",
    "Last Christmas"
};
