// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _OUTTRO_H_
#define _OUTTRO_H_

#include "game.h"

void drawOuttro();
void drawOuttroBG();
void doOuttroLogic();
void loadOuttroGraphics(enum GameState oldState);
int initOuttro();

#endif
