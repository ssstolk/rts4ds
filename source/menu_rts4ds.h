// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _MENU_RTS4DS_H_
#define _MENU_RTS4DS_H_

#include "game.h"

#ifndef DISABLE_MENU_RTS4DS
void drawMenuRts4ds();
void drawMenuRts4dsBG();
void doMenuRts4dsLogic();
void loadMenuRts4dsGraphics(enum GameState oldState);
int initMenuRts4ds();
#endif

#endif
