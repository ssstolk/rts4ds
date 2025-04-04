// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _MENU_SAVEGAME_H_
#define _MENU_SAVEGAME_H_

#include "game.h"

void drawMenuSaveGame();
void drawMenuSaveGameBG();
void doMenuSaveGameLogic();
void loadMenuSaveGameGraphics(enum GameState oldState);
int initMenuSaveGame();

#endif
