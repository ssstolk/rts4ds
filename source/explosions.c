// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "explosions.h"

#include "environment.h"
#include "fileio.h"
#include "view.h"
#include "settings.h"
#include "sprites.h"
#include "shared.h"
#include "objectives.h"
#include "game.h"
#include "rumble.h"
#include "ingame_briefing.h"


struct ExplosionInfo explosionInfo[MAX_DIFFERENT_EXPLOSIONS];
struct Explosion explosion[MAX_EXPLOSIONS_ON_MAP];

static int explosionShiftX, explosionShiftY;


static int base_tankshot;


struct TankShot {
    int timer;
    int x;
    int y;
};
static struct TankShot tankShot[MAX_TANKSHOTS];


void initExplosions() {
    char oneline[256];
    int amountOfExplosions = 0;
    int i, j;
    FILE *fp;
    
    // get the names of all available explosions
    fp = openFile("explosions.ini", FS_PROJECT_FILE);
    readstr(fp, oneline);
    sscanf(oneline, "AMOUNT-OF-EXPLOSIONS %i", &amountOfExplosions);
    if (amountOfExplosions > MAX_DIFFERENT_EXPLOSIONS)
        errorSI("Too many types of explosions exist. Limit is:", MAX_DIFFERENT_EXPLOSIONS);
    for (i=0; i<amountOfExplosions; i++) {
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 255);
        strcpy(explosionInfo[i].name, oneline);
        explosionInfo[i].enabled = 1;
    }
    closeFile(fp);
    
    // get information on each and every available explosion
    for (i=0; i<amountOfExplosions; i++) {
        fp = openFile(explosionInfo[i].name, FS_EXPLOSIONS_INFO);
        
        // MEASUREMENTS section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[MEASUREMENTS]", strlen("[MEASUREMENTS]")));
        readstr(fp, oneline);
        sscanf(oneline, "Size=%i", &j);
        explosionInfo[i].graphics_size = (unsigned int) j;
        
        // ANIMATION section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[ANIMATION]", strlen("[ANIMATION]")));
        readstr(fp, oneline);
        sscanf(oneline, "Frames=%i", &explosionInfo[i].frames);
        readstr(fp, oneline);
        sscanf(oneline, "FrameDuration=%i", &explosionInfo[i].original_frame_duration);
        // optional Repeat information
        readstr(fp, oneline);
        if (oneline[0] == '[')
            explosionInfo[i].repeat = 0;
        else {
            sscanf(oneline, "Repeat=%i", &explosionInfo[i].repeat);
            readstr(fp, oneline);
            sscanf(oneline, "RepeatStartFrame=%i", &explosionInfo[i].repeat_start);
            explosionInfo[i].repeat_start--; // convert to start count from 0, not from 1
            readstr(fp, oneline);
            sscanf(oneline, "RepeatEndFrame=%i", &explosionInfo[i].repeat_end);
            // convert to start count from 0, not from 1... and then add 1 to say -at- which frame instead of -after-
            // so that's -1, +1... so no change at all here with code, really.
            readstr(fp, oneline);
            sscanf(oneline, "RepeatMirror=%i", &explosionInfo[i].repeat_mirror);
            
        // EFFECTS section
            do {
                readstr(fp, oneline);
            } while (strncmp(oneline, "[EFFECTS]", strlen("[EFFECTS]")));
        }
        readstr(fp, oneline);
        sscanf(oneline, "Brightness=%i", &explosionInfo[i].max_brightness);
        readstr(fp, oneline);
        sscanf(oneline, "ShiftX=%i", &explosionInfo[i].max_shift_x);
        readstr(fp, oneline);
        sscanf(oneline, "ShiftY=%i", &explosionInfo[i].max_shift_y);
        readstr(fp, oneline);
        sscanf(oneline, "Rumble=%i", &explosionInfo[i].rumble_level);
        
        closeFile(fp);
    }
    for (i=amountOfExplosions; i<MAX_DIFFERENT_EXPLOSIONS; i++) // setting all unused ones to disabled
        explosionInfo[i].enabled = 0;
}

void initExplosionsSpeed() {
    int i;
    
    for (i=0; i<MAX_DIFFERENT_EXPLOSIONS && explosionInfo[i].enabled; i++) {
        explosionInfo[i].frame_duration = (2*((explosionInfo[i].original_frame_duration * FPS) / SCREEN_REFRESH_RATE)) / getGameSpeed();
        if (explosionInfo[i].frame_duration == 0)
            explosionInfo[i].frame_duration = 1;
    }
}

void initExplosionsWithScenario() {
    int i;
    
    for (i=0; i<MAX_EXPLOSIONS_ON_MAP; i++)
        explosion[i].enabled = 0;
    
    for (i=0; i<MAX_TANKSHOTS; i++)
        tankShot[i].timer = 0;
    
    explosionShiftX = 0;
    explosionShiftY = 0;
    
    initExplosionsSpeed();
}


int addTankShot(int curX, int curY) {
    int i;
    struct TankShot *curTankShot;
    
    if (curX < getViewCurrentX() || curX >= getViewCurrentX() + HORIZONTAL_WIDTH ||
        curY < getViewCurrentY() || curY >= getViewCurrentY() + HORIZONTAL_HEIGHT)
        return -1;
    
    for (i=0, curTankShot=tankShot; i<MAX_TANKSHOTS; i++, curTankShot++) {
        if (curTankShot->timer == 0) {
            curTankShot->timer = 1;
            curTankShot->x = curX * 16;
            curTankShot->y = curY * 16;
            return i;
        }
    }
    return -1;
}


int addExplosion(int curX, int curY, int info, int delay) {
    int i;
    int rumbleDuration;
    struct ExplosionInfo *curExplosionInfo;
    
    if (info < 0) // a unit or structure which can't explode tried to create a new explosion
        return -1;
    
    for (i=0; i<MAX_EXPLOSIONS_ON_MAP; i++) {
        if (!explosion[i].enabled) {
            explosion[i].enabled = 1;
            explosion[i].info = info;
            explosion[i].x = curX;
            explosion[i].y = curY;
            explosion[i].timer = -delay;
            curExplosionInfo = explosionInfo + info;
            rumbleDuration = (curExplosionInfo->frames + curExplosionInfo->repeat * (curExplosionInfo->repeat_end - curExplosionInfo->repeat_start)) * curExplosionInfo->frame_duration;
            if (explosionInfo[info].rumble_level == 10) // can only be Nuke, let it rumble no matter what
                addRumble(10, rumbleDuration);
            else if (curX/16 >= getViewCurrentX() && curX/16 < getViewCurrentX() + HORIZONTAL_WIDTH &&
                     curY/16 >= getViewCurrentY() && curY/16 < getViewCurrentY() + HORIZONTAL_HEIGHT)
                addRumble(explosionInfo[info].rumble_level, rumbleDuration);
            return i;
        }
    }
    return -1;
}



void drawExplosions() {
    int i, j, k;
    int x, y, graphics, mirror;
    int lowX, highX, lowY, highY;
    struct Explosion *curExplosion;
    struct ExplosionInfo *curExplosionInfo;
    int tile;
    struct Explosion *brightnessExplosion = 0;
    int maxBrightness = 0;
    struct Explosion *shiftExplosion = 0;
    int maxShiftTotal = 0;
    short int shiftX, shiftY;
    int explosionsDrawn = 0;
    struct TankShot *curTankShot;
    
    lowX  = getViewCurrentX() * 16;
    highX = (getViewCurrentX() + HORIZONTAL_WIDTH) * 16;
    lowY  = getViewCurrentY() * 16;
    highY = (getViewCurrentY() + HORIZONTAL_HEIGHT) * 16;
    
    for (i=0, curExplosion=explosion; i<MAX_EXPLOSIONS_ON_MAP; i++, curExplosion++) {
        if (curExplosion->enabled && curExplosion->timer >= 0 &&
            curExplosion->x >= lowX && curExplosion->x < highX &&
            curExplosion->y >= lowY && curExplosion->y < highY) {
            tile = TILE_FROM_XY(curExplosion->x/16, curExplosion->y/16);
            if (environment.layout[tile].status != UNDISCOVERED) {
                // explosion needs to be displayed
                curExplosionInfo = explosionInfo + curExplosion->info;
                mirror = 0;
                graphics = curExplosion->timer / curExplosionInfo->frame_duration; // which frame if repeat wouldn't be present
                if (curExplosionInfo->repeat && graphics >= curExplosionInfo->repeat_end) {
                    k = curExplosionInfo->repeat_end - curExplosionInfo->repeat_start;
                    for (j=curExplosionInfo->repeat; j && graphics >= curExplosionInfo->repeat_end; j--) {
                        graphics -= k;
                        mirror = !mirror && curExplosionInfo->repeat_mirror;
                    }
                }
                graphics = curExplosionInfo->graphics_offset + graphics * math_power(4, (int) curExplosionInfo->graphics_size - 1);
                x =  (curExplosion->x - lowX) - (8 << curExplosionInfo->graphics_size) / 2;
                y = ((curExplosion->y - lowY) - (8 << curExplosionInfo->graphics_size) / 2) + ACTION_BAR_SIZE * 16;
                setSpritePlayScreen(y, ATTR0_SQUARE,
                                    x, curExplosionInfo->graphics_size, mirror, 0,
                                    1, PS_SPRITES_PAL_EXPLOSIONS, graphics);
                explosionsDrawn++;
                if (!brightnessExplosion || curExplosionInfo->max_brightness > maxBrightness) {
                    brightnessExplosion = curExplosion;
                    maxBrightness = curExplosionInfo->max_brightness;
                }
                if (!shiftExplosion || (curExplosionInfo->max_shift_x + curExplosionInfo->max_shift_y > maxShiftTotal)) {
                    shiftExplosion = curExplosion;
                    maxShiftTotal = curExplosionInfo->max_shift_x + curExplosionInfo->max_shift_y;
                }
                
            }
        }
    }
    
    for (i=0, curTankShot=tankShot; i<MAX_TANKSHOTS && explosionsDrawn < MAX_EXPLOSIONS_DRAWN; i++, curTankShot++) {
        if (curTankShot->timer > 0 &&
            curTankShot->x >= lowX && curTankShot->x < highX &&
            curTankShot->y >= lowY && curTankShot->y < highY)
        {
//errorI4(curTankShot->x, curTankShot->y, lowX, lowY);
            setSpritePlayScreen((curTankShot->y - lowY) + (ACTION_BAR_SIZE * 16 - 8), ATTR0_SQUARE,
                                (curTankShot->x - lowX) - 8, SPRITE_SIZE_32, 0, 0,
                                1, PS_SPRITES_PAL_EXPLOSIONS, base_tankshot);
            explosionsDrawn++;
        }
    }
    
    if (getGameState() == INGAME && getObjectivesState() == OBJECTIVES_INCOMPLETE && getIngameBriefingState() != IBS_ACTIVE) {
        if (brightnessExplosion && maxBrightness > 0) {
            curExplosionInfo = explosionInfo + brightnessExplosion->info;
            i = (curExplosionInfo->frames + curExplosionInfo->repeat * (curExplosionInfo->repeat_end - curExplosionInfo->repeat_start)) * curExplosionInfo->frame_duration; // amount of game-frames the explosion lasts
            if (i > 0) {
                j = maxBrightness - min(maxBrightness, i / ((int) EXPLOSION_BRIGHTNESS_INTENSITY_LEVEL_DURATION)); // brightness reduced by hardcoded max length for a level
                k = ((maxBrightness * brightnessExplosion->timer) + (i-1)) / i; // brightness reduced by regular length
                maxBrightness = min(j, k); // minimum brightness
                if (maxBrightness <= 0)
                    maxBrightness = 0;
                else
                    maxBrightness = rand() % ((int) maxBrightness);
                REG_MASTER_BRIGHT = maxBrightness | (1<<14); // white
            }
        } else // back to normal
            REG_MASTER_BRIGHT = 0;
    }
    
    if (getGameState() == INGAME && getIngameBriefingState() != IBS_ACTIVE && shiftExplosion) {
        curExplosionInfo = explosionInfo + shiftExplosion->info;
        shiftX = curExplosionInfo->max_shift_x;
        if (shiftX > 0)
            shiftX = (rand() % (shiftX * 2 + 1)) - shiftX;
        shiftY = curExplosionInfo->max_shift_y;
        if (shiftY > 0)
            shiftY = (rand() % (shiftY * 2 + 1)) - shiftY;
        REG_BG0HOFS = (-(shiftX/2));
        REG_BG0VOFS = (-(shiftY/2)) - getScreenScrollingPlayScreenShort(); // doesn't actually work for 3D layer
        REG_BG1HOFS = shiftX;
        REG_BG1VOFS = shiftY - getScreenScrollingPlayScreenShort();
        REG_BG2HOFS = shiftX;
        REG_BG2VOFS = shiftY - getScreenScrollingPlayScreenShort();
        REG_BG3HOFS = shiftX;
        REG_BG3VOFS = shiftY - getScreenScrollingPlayScreenShort();
        explosionShiftX = -shiftX;
        explosionShiftY = -shiftY;
    } else { // back to normal
        REG_BG0HOFS = 0;
        REG_BG0VOFS = 0 - getScreenScrollingPlayScreenShort(); // doesn't actually work for 3D layer
        REG_BG1HOFS = 0;
        REG_BG1VOFS = 0 - getScreenScrollingPlayScreenShort();
        REG_BG2HOFS = 0;
        REG_BG2VOFS = 0 - getScreenScrollingPlayScreenShort();
        REG_BG3HOFS = 0;
        REG_BG3VOFS = 0 - getScreenScrollingPlayScreenShort();
        explosionShiftX = 0;
        explosionShiftY = 0;
    }
}

void doExplosionsLogic() {
    int i;
    struct Explosion *curExplosion;
    struct ExplosionInfo *curExplosionInfo;
    struct TankShot *curTankShot;
    
    startProfilingFunction("doExplosionsLogic");
    
    for (i=0, curExplosion=explosion; i<MAX_EXPLOSIONS_ON_MAP; i++, curExplosion++) {
        if (curExplosion->enabled) {
            curExplosionInfo = explosionInfo + curExplosion->info;
            if (++curExplosion->timer >= (curExplosionInfo->frames + curExplosionInfo->repeat * (curExplosionInfo->repeat_end - curExplosionInfo->repeat_start)) * curExplosionInfo->frame_duration)
                curExplosion->enabled = 0;
        }
    }
    
    for (i=0, curTankShot=tankShot; i<MAX_TANKSHOTS; i++, curTankShot++) {
        if (curTankShot->timer > 0)
            curTankShot->timer--;
    }
    
    stopProfilingFunction();
}

void loadExplosionsGraphicsBG(int baseBg, int *offsetBg) {
}

void loadExplosionsGraphicsSprites(int *offsetSp) {
    int i;

    for (i=0; i<MAX_DIFFERENT_EXPLOSIONS && explosionInfo[i].enabled; i++) {
        explosionInfo[i].graphics_offset = (*offsetSp)/(16*16);
        *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), explosionInfo[i].name, FS_EXPLOSIONS_GRAPHICS);
    }
    
    base_tankshot = (*offsetSp)/(16*16);
    *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), "Tank_Shot", FS_EXPLOSIONS_GRAPHICS);
}


inline int getExplosionShiftX() {
    return explosionShiftX;
}

inline int getExplosionShiftY() {
    return explosionShiftY;
}

inline struct TankShot *getTankShots() {
    return tankShot;
}

int getExplosionsSaveSize(void) {
    return sizeof(explosion);
}

int getTankShotsSaveSize(void) {
    return sizeof(tankShot);
}

int getExplosionsSaveData(void *dest, int max_size) {
    int size=sizeof(explosion);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&explosion,size);
    return (size);
}

int getTankShotsSaveData(void *dest, int max_size) {
    int size=sizeof(tankShot);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&tankShot,size);
    return (size);
}
