// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _PLAYSCREEN_H_
#define _PLAYSCREEN_H_

#include <nds.h>

enum PlayScreenTouchModifier { PSTM_NONE, PSTM_ATTACK, PSTM_MOVE, PSTM_DEPLOY_STRUCTURE, PSTM_PLACE_STRUCTURE, PSTM_SELL_STRUCTURE, PSTM_RALLYPOINT_STRUCTURE };
void setModifierPlaceStructure(int queueNr);
void setModifierNone();
enum PlayScreenTouchModifier getModifier();
int getDeployNrUnit();
int getSellNrStructure();

void inputPlayScreenFriendlyForcedAttack(int tile, int forceHeal);
void inputPlayScreenFriendlyForcedMove(int tile);

void inputPlayScreen();
void doPlayScreenLogic();
void drawPlayScreen();
void drawPlayScreenBG();
void loadPlayScreenGraphics();
void preloadPlayScreen3DGraphics();
void initPlayScreen();

#endif
