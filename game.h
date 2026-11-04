//*****************************************************************************
//
// game.h - Mini Doom: raycaster game on the BoosterPack MKII display
//
//*****************************************************************************
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

void game_init(void);   // display + first (title) screen

// one game step: inputs of this frame, now = clock_ms()
//   shoot / use = button was JUST pressed (edge) in this frame
//   joyX / joyY = joystick axes, centre = 0, -2048 .. 2047
void game_update(uint32_t now, bool shoot, bool use, int32_t joyX, int32_t joyY);

void game_draw(uint32_t now);   // render the frame and send it to the display

#endif
