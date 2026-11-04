//*****************************************************************************
//
// melodies.h - Note frequencies and the melody table
//
//*****************************************************************************
#ifndef MELODIES_H
#define MELODIES_H

#include <stdint.h>

// frequencies in Hz (A4 = 440 Hz), S = sharp (e.g. GS3 = G#3)
#define G3   196
#define GS3  208
#define AS3  233
#define C4   262
#define D4   294
#define DS4  311
#define E4   330
#define FS4  370
#define F4   349
#define G4   392
#define GS4  415
#define A4   440
#define AS4  466
#define B4   494   // English B = German H
#define C5   523
#define D5   587
#define DS5  622
#define E5   659
#define F5   698
#define FS5  740
#define G5   784
#define GS5  831
#define AS5  932
#define C6  1047

// note lengths in ms
#define EIGHTH   150
#define QUARTER  300
#define HALF     600
#define WHOLE   1200
#define FAST     100   // fast notes of the Super Mario fanfare

#define MELODY_COUNT  5    // number of melodies
#define MAX_NOTES    40    // maximum number of notes per melody (incl. end marker)

// one note: frequency in Hz and duration in ms
typedef struct
{
    uint16_t freq;
    uint16_t duration;     // 0 = end of the melody
} Note;

// melodies[m][n] = note n of melody m
extern const Note melodies[MELODY_COUNT][MAX_NOTES];

// name of every melody (for the display)
extern const char *const melodyNames[MELODY_COUNT];

#endif
