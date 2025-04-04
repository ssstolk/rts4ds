// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _EXPLOSIONS_H_
#define _EXPLOSIONS_H_

#include <nds.h>

#define MAX_DIFFERENT_EXPLOSIONS   12
#define MAX_EXPLOSION_NAME_LENGTH  30

#define MAX_EXPLOSIONS_ON_MAP      150 /* usually approximately the amount of units */

#define MAX_EXPLOSION_BRIGHTNESS_INTENSITY             10
#define EXPLOSION_BRIGHTNESS_INTENSITY_LEVEL_DURATION  (FPS / 2)
#define EXPLOSION_BRIGHTNESS_INTENSITY_MAX_DERIVATION  4 /* this max derivation -rarely- occurs, keep that in mind */

#define MAX_EXPLOSION_SHAKE_X  2
#define MAX_EXPLOSION_SHAKE_Y  2

#define EXPLOSION_FX_THRESHHOLD  (2 * FPS) /* an explosion will need to be displayed for this long (normal speed) at minimum to affect fx */

#define MAX_EXPLOSIONS_DRAWN  (HORIZONTAL_WIDTH * HORIZONTAL_HEIGHT)

#define MAX_TANKSHOTS  ((HORIZONTAL_WIDTH * HORIZONTAL_HEIGHT) / 2)


struct ExplosionInfo {
    int enabled;
    char name[MAX_EXPLOSION_NAME_LENGTH+1];
    int frames;
    int frame_duration;
    int repeat;
    int repeat_start; // frame
    int repeat_end; // frame
    int repeat_mirror;
    int max_brightness;
    int max_shift_x;
    int max_shift_y;
    int rumble_level;
    uint16 graphics_size;
    int graphics_offset;
    
    int original_frame_duration;
};

struct Explosion {
    int enabled;
    int info;
    int x; // be aware! pixel-precision here for x and y, not tile-precision.
    int y;
    int timer;
};

void initExplosions();
void initExplosionsWithScenario();
int addExplosion(int curX, int curY, int info, int delay);
int addTankShot(int curX, int curY);

void drawExplosions();
void loadExplosionsGraphicsBG(int baseBg, int *offsetBg);
void loadExplosionsGraphicsSprites(int *offsetSp);

void doExplosionsLogic();

extern struct ExplosionInfo explosionInfo[];
extern struct Explosion explosion[];

int getExplosionShiftX();
int getExplosionShiftY();

struct TankShot *getTankShots();
int getExplosionsSaveSize(void);
int getTankShotsSaveSize(void);
int getExplosionsSaveData(void *dest, int max_size);
int getTankShotsSaveData(void *dest, int max_size);

#endif
