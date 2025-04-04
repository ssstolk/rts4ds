// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _MENU_LEVELSELECT_H_
#define _MENU_LEVELSELECT_H_

#include "game.h"

void drawMenuLevelSelect();
void drawMenuLevelSelectBG();
void doMenuLevelSelectLogic();
void loadMenuLevelSelectGraphics(enum GameState oldState);
int initMenuLevelSelect();

#endif
