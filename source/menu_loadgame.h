// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _MENU_LOADGAME_H_
#define _MENU_LOADGAME_H_

#include "game.h"

void drawMenuLoadGame();
void drawMenuLoadGameBG();
void doMenuLoadGameLogic();
void loadMenuLoadGameGraphics(enum GameState oldState);
int initMenuLoadGame();

#endif
