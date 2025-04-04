// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _MENU_CONTINUE_H_
#define _MENU_CONTINUE_H_

#include "game.h"

void drawMenuContinue();
void drawMenuContinueBG();
void doMenuContinueLogic();
void loadMenuContinueGraphics(enum GameState oldState);
int initMenuContinue();

#endif
