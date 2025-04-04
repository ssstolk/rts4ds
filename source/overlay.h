// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _OVERLAY_H_
#define _OVERLAY_H_

#include "shared.h"

#define MAX_OVERLAY_ON_MAP   200
#define MAX_PERMANENT_OVERLAY_TYPES 1

enum OverlayType { OT_TRACKS, OT_TRACKS2, OT_SMOKE, OT_GRUBS, OT_GRUBS2, OT_DESTROYED, OT_ROCK_SHOT, OT_SAND_SHOT, OT_DEAD_FOOT, OT_BLOOD, OT_PERMANENT };
#define NUMBER_OF_OVERLAY_TYPES (OT_PERMANENT + 1)

struct Overlay {
    int enabled;
    enum OverlayType type;
    int x;
    int y;
    int frame;
    int timer;
};

void addStructureDestroyedOverlay(int x, int y, int width, int height);
void clearStructureDestroyedOverlay(int x, int y, int width, int height);

int addOverlay(int x, int y, enum OverlayType type, int frame);
void drawOverlay();
void doOverlayLogic();
void loadOverlayGraphicsBG(int baseBg, int *offsetBg);
void loadOverlayGraphicsSprites(int *offsetSp);
void initOverlayWithScenario();
void initOverlay();

struct Overlay *getOverlay();
int getOverlaySaveSize(void);
int getOverlaySaveData(void *dest, int max_size);

#endif
