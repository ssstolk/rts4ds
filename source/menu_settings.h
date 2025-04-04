// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _MENU_SETTINGS_H_
#define _MENU_SETTINGS_H_

#include "game.h"

void drawMenuSettings();
void drawMenuSettingsBG();
void doMenuSettingsLogic();
void loadMenuSettingsGraphics(enum GameState oldState);
int initMenuSettings();

#endif
