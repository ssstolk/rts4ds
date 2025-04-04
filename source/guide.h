// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _GUIDE_H_
#define _GUIDE_H_

#include "game.h"

void doGuideLogic();
int initGuide();
void drawGuide();
void drawGuideBG();
void loadGuideGraphics(enum GameState oldState);

#endif
