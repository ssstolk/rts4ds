// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _MENU_GAMEINFO_H_
#define _MENU_GAMEINFO_H_

#include "game.h"

void setGameinfoIsStructure(int on);
int getGameinfoIsStructure();

void drawMenuGameinfo();
void drawMenuGameinfoBG();
void doMenuGameinfoLogic();
void loadMenuGameinfoGraphics(enum GameState oldState);
int initMenuGameinfo();

#endif
