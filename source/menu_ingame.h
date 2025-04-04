// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _MENU_INGAME_H_
#define _MENU_INGAME_H_

#include "game.h"

void drawMenuIngame();
void drawMenuIngameBG();
void doMenuIngameLogic();
void loadMenuIngameGraphics(enum GameState oldState);
int initMenuIngame();

#endif
