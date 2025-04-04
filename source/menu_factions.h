// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _MENU_FACTIONS_H_
#define _MENU_FACTIONS_H_

#include "game.h"

void drawMenuFactions();
void drawMenuFactionsBG();
void doMenuFactionsLogic();
void loadMenuFactionsGraphics(enum GameState oldState);
int initMenuFactions();

#endif
