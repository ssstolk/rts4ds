// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _MENU_GAMEINFO_ITEM_SELECT_H_
#define _MENU_GAMEINFO_ITEM_SELECT_H_

#include "game.h"

void setGameinfoItemNr(int nr);
int getGameinfoItemNr();

void drawMenuGameinfoItemSelect();
void drawMenuGameinfoItemSelectBG();
void doMenuGameinfoItemSelectLogic();
void loadMenuGameinfoItemSelectGraphics(enum GameState oldState);
int initMenuGameinfoItemSelect();

#endif
