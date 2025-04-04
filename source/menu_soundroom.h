// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _MENU_SOUNDROOM_H_
#define _MENU_SOUNDROOM_H_

#include "game.h"

void drawMenuSoundroom();
void drawMenuSoundroomBG();
void doMenuSoundroomLogic();
void loadMenuSoundroomGraphics(enum GameState oldState);
int initMenuSoundroom();

#endif
