// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _RADAR_H_
#define _RADAR_H_

#define RADAR_MAX_SIZE 176

void setTileDirtyRadarDirtyBitmap(int tile);
void setAllDirtyRadarDirtyBitmap();

void displayHitRadar(int x, int y);
void doHitBoxLogic();

void positionRadar();
void drawRadar();
void inputRadar(int x, int y);
void drawRadarBG();
void doRadarLogic();
void loadRadarGraphics(int baseBg, int *offsetBg, int *offsetSp);
void initRadar();

void dataSaveGameRadar(unsigned short *data);

#endif
