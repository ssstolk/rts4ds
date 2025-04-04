// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _MENU_REGIONS_H_
#define _MENU_REGIONS_H_

#include "game.h"

void drawMenuRegions();
void drawMenuRegionsBG();
void doMenuRegionsLogic();
void loadMenuRegionsGraphics(enum GameState oldState);
int initMenuRegions();

#endif
