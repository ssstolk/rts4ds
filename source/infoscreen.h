// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _INFOSCREEN_H_
#define _INFOSCREEN_H_

#include <nds.h>

#define BUTTON_HOTKEYS_X1      91
#define BUTTON_HOTKEYS_X2      118
#define BUTTON_HOTKEYS_Y1      176
#define BUTTON_HOTKEYS_Y2      192

#define BUTTON_BUILDQUEUE_X1   138
#define BUTTON_BUILDQUEUE_X2   165
#define BUTTON_BUILDQUEUE_Y1   176
#define BUTTON_BUILDQUEUE_Y2   192

#define BUTTON_SWAPSCREENS_X1  120
#define BUTTON_SWAPSCREENS_X2  136
#define BUTTON_SWAPSCREENS_Y1  176
#define BUTTON_SWAPSCREENS_Y2  192
  

void setInfoScreenBuildInfo(int isStructure, int info);
void setInfoScreenRadar();

void createBarStructures();
//void createBarBuildables();

void inputInfoScreen();
void doInfoScreenLogic();
void drawInfoScreen();
void drawInfoScreenBG();
void loadInfoScreenGraphics();
void initInfoScreen();

#endif
