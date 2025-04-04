// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _MENU_MAIN_H_
#define _MENU_MAIN_H_

#include "game.h"

void drawMenuMain();
void drawMenuMainBG();
void doMenuMainLogic();
void loadMenuMainGraphics(enum GameState oldState);
int initMenuMain();

#endif
