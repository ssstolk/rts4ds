// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _CUTSCENE_DEBRIEFING_H_
#define _CUTSCENE_DEBRIEFING_H_

#include "game.h"

void drawCutsceneDebriefing();
void drawCutsceneDebriefingBG();
void doCutsceneDebriefingLogic();
void loadCutsceneDebriefingGraphics(enum GameState oldState);
int initCutsceneDebriefing();
void initCutsceneDebriefingFilename(int win);

#endif
