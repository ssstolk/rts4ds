// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _CUTSCENE_INTRODUCTION_H_
#define _CUTSCENE_INTRODUCTION_H_

#include "game.h"

void drawCutsceneIntroduction();
void drawCutsceneIntroductionBG();
void doCutsceneIntroductionLogic();
void loadCutsceneIntroductionGraphics(enum GameState oldState);
int initCutsceneIntroduction();
void initCutsceneIntroductionFilename();

#endif
