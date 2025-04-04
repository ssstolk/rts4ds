// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _CUTSCENE_BRIEFING_H_
#define _CUTSCENE_BRIEFING_H_

#include "game.h"

void drawCutsceneBriefing();
void drawCutsceneBriefingBG();
void doCutsceneBriefingLogic();
void loadCutsceneBriefingGraphics(enum GameState oldState);
int initCutsceneBriefing();
void initCutsceneBriefingFilename();

#endif
