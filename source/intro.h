// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _INTRO_H_
#define _INTRO_H_

#include "game.h"

void drawIntro();
void drawIntroBG();
void doIntroLogic();
void loadIntroGraphics(enum GameState oldState);
int initIntro();

#endif
