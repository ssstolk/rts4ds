// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _SPLASH_H_
#define _SPLASH_H_

#include "game.h"

#ifdef DISABLE_MENU_RTS4DS
void drawSplash();
void drawSplashBG();
void doSplashLogic();
void loadSplashGraphics(enum GameState oldState);
int initSplash();
#endif

#endif
