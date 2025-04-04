// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _ANIMATION_H_
#define _ANIMATION_H_

#include "game.h"

void drawAnimation();
void drawAnimationBG();
int doAnimationLogic();
void loadAnimationGraphics(enum GameState oldState);
int initAnimation(char *filename, int keepMainFree, int keepMusicFree, int keepSoundFree);

#endif
