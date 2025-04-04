// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "shared.h"

#include "structures.h"
#include "units.h"
#include "view.h"
#include "radar.h"
#include "settings.h"
#include "soundeffects.h"
#include "inputx.h"
#include "settings.h"


#define DOUBLE_TAP_HOTKEY_MAX_INTERVAL  (FPS / 3)
//#define TRIGGER_HOTKEY_ADD_MIN_INTERVAL  (2*FPS)
#define TRIGGER_HOTKEY_SAVE_MIN_INTERVAL (FPS + FPS/4)


int levelLoadedViaLevelSelect = 0;


int nrstrlen(int nr) {
    int len=1;
    while (nr >= 10) {
        nr /= 10;
        len++;
    }
    return len;
}


inline int abs(int value) {
    return (-(value < 0)) * value;
}

inline int min(int lh, int rh) {
    return (lh < rh) ? lh : rh;
}

inline int max(int lh, int rh) {
    return (lh > rh) ? lh : rh;
}

inline int square_root(int value) {
    int i;
    
    for (i=0; i<value/2; i++) {
        if (i*i >= value)
            return (i == 0) ? 0 : i-1;
    }
    return value/2;
}


int math_power(int base, int power) {
    int i;
    int result = 1;
    
    for (i=0; i<power; i++)
        result *= base;
    return result;
}


int withinRange(int curX, int curY, int range, int tgtX, int tgtY) {
    int diffX = abs(tgtX - curX);
    int diffY = abs(tgtY - curY);
    
    if (diffX == 0) {
        if (diffY == 0) // should never be able to shoot oneself ;)
            return 0;
        return (diffY <= range);
    }
    if (diffY == 0)
        return (diffX <= range);
    
    if (range == 1)
        return (diffX == 1 && diffY == 1);
    
    return (diffX * diffX + diffY * diffY <= range * range);
}

int withinShootRange(int curX, int curY, int range, int projectile_info, int tgtX, int tgtY) {
    if (!withinRange(curX, curY, range, tgtX, tgtY))
        return 0;
    
    if (!isClearPathProjectile(curX, curY, projectile_info, tgtX, tgtY))
        return 0;
    
    return 1;
}

int sideUnitWithinRangeStartingAt(enum Side side, int curX, int curY, int range, int unitnr) {
    int i = unitnr;
    
    for (; i<MAX_UNITS_ON_MAP; i++) {
        if (unit[i].enabled && !unit[i].side == !side) {
            if (withinRange(curX, curY, range, unit[i].x, unit[i].y))
                return i;
        }
    }
    return -1;

}

int sideUnitWithinRange(enum Side side, int curX, int curY, int range) {
    return sideUnitWithinRangeStartingAt(side, curX, curY, range, 0);
}

int randFixed() {
  #define RANDFIXEDARRAYLEN 31
  // RANDFIXEDARRAYLEN should be a PRIME for this to perform better!
  static int idx = 0;
  static u16 randStore[RANDFIXEDARRAYLEN] = {28379, 38924, 40223, 31191, 50039, 37195,
                                              51804, 65446, 59058,   407, 33931, 49799,
                                              25902, 65110, 39492, 65383,  8259, 29236,
                                              48217, 43098,  7779, 37572, 38634, 61276,
                                              37226, 49546,  3957, 57633, 60420, 52254,
                                              48024};

  if ((++idx)==RANDFIXEDARRAYLEN)
    idx=0;

  return randStore[idx];
}


// generates a pseudo-random value between -(delta-1) and +(delta-1)
int generateRandVariation(int delta) {
  // 50% increase, 50% decrease, to balance...
    if ((randFixed()%4)<2)
        return (randFixed()%delta);
    else
        return (-(randFixed()%delta));
}


int calculateDistance2(int curX, int curY, int tgtX, int tgtY) {
// actually it's distance^2
    int diffX = abs(tgtX - curX);
    int diffY = abs(tgtY - curY);
    
    if (diffX == 0) {
        if (diffY == 0)
            return 0;
        return (diffY*diffY);
    }
  
    if (diffY == 0)
        return (diffX*diffX);
    
    if (diffX == 1 && diffY == 1)
        return 1;
        // I know, this is a curious piece of code :)
    
    return (diffX*diffX + diffY*diffY);
}


int sideUnitWithinShootRange(enum Side side, int curX, int curY, int range, int projectile_info) {
    //  linear complexity (testing units just once)
    //  doesn't call sideUnitWithinRangeStartingAt() function )

    int i, bestUnitNum = -1;
    int dist, bestDist = range * range + 1;
  
    for (i=0; i<MAX_UNITS_ON_MAP; i++) {
        if (unit[i].enabled && !unit[i].side == !side) {
            dist = calculateDistance2(curX, curY, unit[i].x, unit[i].y); 
            if ((dist<bestDist) && isClearPathProjectile(curX, curY, projectile_info, unit[i].x, unit[i].y)) {
                // found a closer hittable unit
                bestDist=dist;
                bestUnitNum=i;
            }
        }
    }
  
    return bestUnitNum;
}

int damagedFootWithinRange(enum Side side, int curX, int curY, int range) {
    int i;
    
    for (i=0; i<MAX_UNITS_ON_MAP; i++) {
        if (unit[i].enabled && unit[i].side == side && unitInfo[unit[i].info].type == UT_FOOT && unit[i].armour < unitInfo[unit[i].info].max_armour) {
            if (withinRange(curX, curY, range, unit[i].x, unit[i].y))
                return i;
        }
    }
    return -1;
}

int rotateBaseToFace(enum Positioned positioned, enum Positioned positioned_new) {
    int diff;
    
    if (positioned % 4 == positioned_new % 4)
        return 0;
    
    diff = (positioned % 4) - (positioned_new % 4);
    if ((diff > 0 && diff < 2) || (diff < -2))
        return 1;
    return 2;
}

int rotateToFace(enum Positioned positioned, enum Positioned positioned_new) {
    int diff;
    
    if (positioned == positioned_new)
        return 0;
    
    diff = positioned - positioned_new;
    if ((diff > 0 && diff < 4) || (diff < -4))
        return 1;
    return 2;
}

enum Positioned positionedToFaceXY(int curX, int curY, int tgtX, int tgtY) {
    enum Positioned facing;
    int diffX = tgtX - curX;
    int diffY = tgtY - curY;
    
    if (diffY < 0) { // upper half
        if (diffX <= 0) { // upper-left part
            if (diffX >= (diffY+1)/2)
                facing = UP;
            else if (diffY >= (diffX+1)/2)
                facing = LEFT;
            else
                facing = LEFT_UP;
        } else { // upper-right part
            if (diffX <= -((diffY+1)/2))
                facing = UP;
            else if (diffY >= -((diffX-1)/2))
                facing = RIGHT;
            else
                facing = RIGHT_UP;
        }
    } else { // lower half
        if (diffX <= 0) { // lower-left part
            if (diffX >= ((-diffY)+1)/2)
                facing = DOWN;
            else if ((-diffY) >= (diffX+1)/2)
                facing = LEFT;
            else
                facing = LEFT_DOWN;
        } else { // lower-right part
            if (diffX <= -(((-diffY)+1)/2))
                facing = DOWN;
            else if ((-diffY) >= -((diffX-1)/2))
                facing = RIGHT;
            else
                facing = RIGHT_DOWN;
        }
    }
    return facing;
}

int rotateToFaceXY(int curX, int curY, enum Positioned positioned, int tgtX, int tgtY) {
    enum Positioned facing;
    int diff;
    
    facing = positionedToFaceXY(curX, curY, tgtX, tgtY);
    
    // facing now holds what position should be obtained long-term
    // in short-term, rotation can only be done by one step though
    if (facing == positioned) return 0; // no rotation is needed
    diff = (int) facing - (int) positioned;
    if (diff > 0) {
        if (diff <= 4) return 2; // rotate to the right
        else return 1; // rotate to the left
    }
    if (diff >= -3) return 1; // rotate to the left
    else return 2; // rotate to the right
}


void activateEntityView(int x, int y, int width, int height, int viewRange) {
    #ifndef DISABLE_SHROUD
    struct EnvironmentLayout *envLayout;
    int viewRangeM  = viewRange - 1;
    int viewRangeM2 = viewRangeM * viewRangeM;
    
    int amountX, amountY;
    int i, j;
    
    // first take care of the tiles occupied by the entity itself
    envLayout = environment.layout + TILE_FROM_XY(x,y);
    for (i=0; i<height; i++) {
        for (j=0; j<width; j++) {
            if (envLayout->status == UNDISCOVERED)
                setTileDirtyRadarDirtyBitmap(TILE_FROM_XY(x+j, y+i));
            envLayout->status = CLEAR_ALL;
            envLayout++;
        }
        envLayout += environment.width - width;
    }
    
    // the ones above the structure
    amountY = min(viewRangeM, y);
    for (i=0; i<width; i++) {
        envLayout = environment.layout + TILE_FROM_XY(x+i, y-1);
        for (j=0; j<amountY; j++) {
            if (envLayout->status == UNDISCOVERED)
                setTileDirtyRadarDirtyBitmap(TILE_FROM_XY(x+i, (y-1)-j));
            envLayout->status = CLEAR_ALL;
            envLayout -= environment.width;
        }
        if (j<viewRange && ((y-1)-j >= 0))
            adjustShroudType(x+i, (y-1)-j);
    }
    
    // the ones on the right side of the structure
    amountX = min(viewRangeM, environment.width - (x+width));
    for (i=0; i<height; i++) {
        envLayout = environment.layout + TILE_FROM_XY(x+width, y+i);
        for (j=0; j<amountX; j++) {
            if (envLayout->status == UNDISCOVERED)
                setTileDirtyRadarDirtyBitmap(TILE_FROM_XY(x+width+j, y+i));
            envLayout->status = CLEAR_ALL;
            envLayout++;
        }
        if (j<viewRange && ((x+width)+j < environment.width))
            adjustShroudType((x+width)+j, y+i);
    }
    
    // the ones below the structure
    amountY = min(viewRangeM, environment.height - (y+height));
    for (i=0; i<width; i++) {
        envLayout = environment.layout + TILE_FROM_XY(x+i, y+height);
        for (j=0; j<amountY; j++) {
            if (envLayout->status == UNDISCOVERED)
                setTileDirtyRadarDirtyBitmap(TILE_FROM_XY(x+i, (y+height)+j));
            envLayout->status = CLEAR_ALL;
            envLayout += environment.width;
        }
        if (j<viewRange && ((y+height)+j < environment.height))
            adjustShroudType(x+i, (y+height)+j);
    }
    
    // the ones on the left side of the structure
    amountX = min(viewRangeM, x);
    for (i=0; i<height; i++) {
        envLayout = environment.layout + TILE_FROM_XY(x-1, y+i);
        for (j=0; j<amountX; j++) {
            if (envLayout->status == UNDISCOVERED)
                setTileDirtyRadarDirtyBitmap(TILE_FROM_XY((x-1)-j, y+i));
            envLayout->status = CLEAR_ALL;
            envLayout--;
        }
        if (j<viewRange && ((x-1)-j >= 0))
            adjustShroudType((x-1)-j, y+i);
    }
    
    // the top-right ones
    amountY = min(viewRange, y);
    amountX = min(viewRange, environment.width - (x+width));
    for (i=0; i<amountY; i++) {
        envLayout = environment.layout + TILE_FROM_XY(x+width, (y-1) - i);
        for (j=0; j<amountX && i*i + j*j <= viewRangeM2; j++) {
            if (envLayout->status == UNDISCOVERED)
                setTileDirtyRadarDirtyBitmap(TILE_FROM_XY(x+width+j, (y-1) - i));
            envLayout->status |= CLEAR_DOWN | CLEAR_LEFT;
            adjustShroudType(x + width + j, (y-1) - i);
            envLayout++;
        }
    }
    
    // the bottom-right ones
    amountY = min(viewRange, environment.height - (y+height));
    for (i=0; i<amountY; i++) {
        envLayout = environment.layout + TILE_FROM_XY(x+width, y + height + i);
        for (j=0; j<amountX && i*i + j*j <= viewRangeM2; j++) {
            if (envLayout->status == UNDISCOVERED)
                setTileDirtyRadarDirtyBitmap(TILE_FROM_XY(x+width+j, y + height + i));
            envLayout->status |= CLEAR_UP | CLEAR_LEFT;
            adjustShroudType(x + width + j, y + height + i);
            envLayout++;
        }
    }
    
    // the bottom-left ones
    amountX = min(viewRange, x);
    for (i=0; i<amountY; i++) {
        envLayout = environment.layout + TILE_FROM_XY(x-1, y + height + i);
        for (j=0; j<amountX && i*i + j*j <= viewRangeM2; j++) {
            if (envLayout->status == UNDISCOVERED)
                setTileDirtyRadarDirtyBitmap(TILE_FROM_XY((x-1)-j, y + height + i));
            envLayout->status |= CLEAR_UP | CLEAR_RIGHT;
            adjustShroudType((x-1) - j, y + height + i);
            envLayout--;
        }
    }
    
    // the top-left ones
    amountY = min(viewRange, y);
    for (i=0; i<amountY; i++) {
        envLayout = environment.layout + TILE_FROM_XY(x-1, (y-1) - i);
        for (j=0; j<amountX && i*i + j*j <= viewRangeM2; j++) {
            if (envLayout->status == UNDISCOVERED)
                setTileDirtyRadarDirtyBitmap(TILE_FROM_XY((x-1)-j, (y-1) - i));
            envLayout->status |= CLEAR_RIGHT | CLEAR_DOWN;
            adjustShroudType((x-1) - j, (y-1) - i);
            envLayout--;
        }
    }
    #endif
}


void alterXYAccordingToUnitMovement(unsigned int move, int move_aid, int speed, int *x, int *y) {
    int aid;
    
    if (move >= UM_MOVE_UP && move <= UM_MOVE_LEFT_UP) {
        if (move_aid <= speed/2) { // in 2nd tile of movement
            aid = (move_aid * 8) / (speed/2);
            if (move >= UM_MOVE_RIGHT_UP && move <= UM_MOVE_RIGHT_DOWN) // moving right
                (*x) -= aid;
            else if (move != UM_MOVE_UP && move != UM_MOVE_DOWN) // moving left
                (*x) += aid;
            if (move >= UM_MOVE_RIGHT_DOWN && move <= UM_MOVE_LEFT_DOWN) // moving down
                (*y) -= aid;
            else if (move != UM_MOVE_LEFT && move != UM_MOVE_RIGHT) // moving up
                (*y) += aid;
        } else { // in 1st tile of movement
            aid = 8 - ((((move_aid - (speed/2))) * 8) / (speed/2));
            if (move >= UM_MOVE_RIGHT_UP && move <= UM_MOVE_RIGHT_DOWN) // moving right
                (*x) += aid;
            else if (move != UM_MOVE_UP && move != UM_MOVE_DOWN) // moving left
                (*x) -= aid;
            if (move >= UM_MOVE_RIGHT_DOWN && move <= UM_MOVE_LEFT_DOWN) // moving down
                (*y) += aid;
            else if (move != UM_MOVE_LEFT && move != UM_MOVE_RIGHT) // moving up
                (*y) -= aid;
        }
    }
}


unsigned int volumePercentageOfLocation(int x, int y, int distanceToHalveVolume) {
    unsigned int distance;
    unsigned int aid;
    unsigned int diffX = 0;
    unsigned int diffY = 0;
    int viewX = getViewCurrentX();
    int viewY = getViewCurrentY();
    
    if (x < viewX)
        diffX = viewX - x;
    else if (x >= viewX + HORIZONTAL_WIDTH)
        diffX = (x - (viewX + HORIZONTAL_WIDTH)) + 1;
    
    if (y < viewY)
        diffY = viewY - y;
    else if (y >= viewY + HORIZONTAL_HEIGHT)
        diffY = (y - (viewY + HORIZONTAL_HEIGHT)) + 1;
    
    // we're going to use Manhattan distance here, for shame.
    distance = diffX + diffY;
    
    aid = (distance * 128) / distanceToHalveVolume;
    return (100 * 100) /
            (math_power(2, aid / 128) *
            (((100 * (aid % 128)) / 128) + 100));
}


unsigned int soundeffectPanningOfLocation(int x, int y) {
    #ifdef _MONAURALSWITCH_
    if (getMonauralEnabled())
        return 64;
    #endif

    int viewX = getViewCurrentX();
    if (x < viewX)
        return 20; //0;
    if (x >= viewX + HORIZONTAL_WIDTH)
        return 107; //127;
    return 64;
}


void inputButtonScrolling() {
    static int scrolling = 0;
    int i;
    
    if (getKeysHeldTimeMin(KEY_TOUCH)) {
        scrolling = 0; // stop scrolling timer
        return;
    }
    
    for (i=0; i<SCREEN_REFRESH_RATE/FPS; i++) {
        
        if (getPrimaryHand() == RIGHT_HANDED) {
            if (keysHeld() & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN)) {
                if (scrolling==0 || scrolling==10 || scrolling==20 || scrolling==25 || (scrolling>29 && scrolling<60 && scrolling&1) || scrolling>60) {
                    if (keysHeld() & KEY_LEFT && keysHeld() & KEY_UP) { setViewCurrentXY(getViewCurrentX() - 1, getViewCurrentY() - 1); }
                    else if (keysHeld() & KEY_RIGHT && keysHeld() & KEY_UP) { setViewCurrentXY(getViewCurrentX() + 1, getViewCurrentY() - 1); }
                    else if (keysHeld() & KEY_LEFT && keysHeld() & KEY_DOWN) { setViewCurrentXY(getViewCurrentX() - 1, getViewCurrentY() + 1); }
                    else if (keysHeld() & KEY_RIGHT && keysHeld() & KEY_DOWN) { setViewCurrentXY(getViewCurrentX() + 1, getViewCurrentY() + 1); }
                    else if (keysHeld() & KEY_UP) { setViewCurrentXY(getViewCurrentX(), getViewCurrentY() - 1); }
                    else if (keysHeld() & KEY_DOWN) { setViewCurrentXY(getViewCurrentX(), getViewCurrentY() + 1); }
                    else if (keysHeld() & KEY_LEFT) { setViewCurrentXY(getViewCurrentX() - 1, getViewCurrentY()); }
                    else if (keysHeld() & KEY_RIGHT) { setViewCurrentXY(getViewCurrentX() + 1, getViewCurrentY()); }
                }
                scrolling++;
                if (scrolling >= 240) scrolling = 120;
            } else scrolling = 0;
        } else {
            if (keysHeld() & (KEY_A | KEY_B | KEY_X | KEY_Y)) {
                if (scrolling==0 || scrolling==10 || scrolling==20 || scrolling==25 || (scrolling>29 && scrolling<60 && scrolling&1) || scrolling>60) {
                    if (keysHeld() & KEY_Y && keysHeld() & KEY_X) { setViewCurrentXY(getViewCurrentX() - 1, getViewCurrentY() - 1); }
                    else if (keysHeld() & KEY_A && keysHeld() & KEY_X) { setViewCurrentXY(getViewCurrentX() + 1, getViewCurrentY() - 1); }
                    else if (keysHeld() & KEY_Y && keysHeld() & KEY_B) { setViewCurrentXY(getViewCurrentX() - 1, getViewCurrentY() + 1); }
                    else if (keysHeld() & KEY_A && keysHeld() & KEY_B) { setViewCurrentXY(getViewCurrentX() + 1, getViewCurrentY() + 1); }
                    else if (keysHeld() & KEY_X) { setViewCurrentXY(getViewCurrentX(), getViewCurrentY() - 1); }
                    else if (keysHeld() & KEY_B) { setViewCurrentXY(getViewCurrentX(), getViewCurrentY() + 1); }
                    else if (keysHeld() & KEY_Y) { setViewCurrentXY(getViewCurrentX() - 1, getViewCurrentY()); }
                    else if (keysHeld() & KEY_A) { setViewCurrentXY(getViewCurrentX() + 1, getViewCurrentY()); }
                }
                scrolling++;
                if (scrolling >= 240) scrolling = 120;
            } else scrolling = 0;
        }
        
    }
}


void clearHotKey(int key) {
    int i;
    struct Structure *curStructure = structure;
    struct Unit *curUnit = unit;
    
    for (i=0; i<MAX_STRUCTURES_ON_MAP; i++, curStructure++) {
        if (curStructure->enabled && curStructure->group == key)
            curStructure->group = KEY_LID; // i.e. not a button usable for group
    }
    for (i=0; i<MAX_UNITS_ON_MAP; i++, curUnit++) {
        if (curUnit->enabled && (curUnit->group & key) && curUnit->side == FRIENDLY)
            curUnit->group = KEY_LID | (curUnit->group & UGAI_FRIENDLY_MASK); // i.e. not a button usable for group
    }
}


int addHotKey(int key) {
    int i;
    struct Structure *curStructure = structure;
    struct Unit *curUnit = unit;
    
    // ensure no enemy units or any structures are selected
    for (i=0; i<MAX_UNITS_ON_MAP; i++, curUnit++) {
        if (curUnit->enabled && curUnit->selected && curUnit->side != FRIENDLY)
            return 0;
    }
    for (i=0; i<MAX_STRUCTURES_ON_MAP; i++, curStructure++) {
        if (curStructure->enabled && (curStructure->selected || curStructure->group == key))
            return 0;
    }
    
    // add currently selected units to specified key; then selects all units grouped under that hotkey
    for (i=0, curUnit=unit; i<MAX_UNITS_ON_MAP; i++, curUnit++) {
        if (curUnit->enabled && curUnit->selected) // known to be FRIENDLY due to above check for enemy unit
            curUnit->group = key | (curUnit->group & UGAI_FRIENDLY_MASK);
        if (curUnit->enabled && !curUnit->selected && curUnit->group == (key | (curUnit->group & UGAI_FRIENDLY_MASK)))
            curUnit->selected = 1;
    }
    return 1;
}

int saveHotKey(int key) {
    int i;
    struct Structure *curStructure = structure;
    struct Unit *curUnit = unit;
    
    for (i=0; i<MAX_UNITS_ON_MAP; i++, curUnit++) {
        if (curUnit->enabled && curUnit->selected && curUnit->side != FRIENDLY)
            return 0;
    }
    for (i=0; i<MAX_STRUCTURES_ON_MAP; i++, curStructure++) {
        if (curStructure->enabled) {
            if (curStructure->selected)
                curStructure->group = key;
            else if (curStructure->group == key)
                curStructure->group = KEY_LID; // i.e. not a button usable for group
        }
    }
    for (i=0, curUnit=unit; i<MAX_UNITS_ON_MAP; i++, curUnit++) {
        if (curUnit->enabled && curUnit->side == FRIENDLY) {
            if (curUnit->selected)
                curUnit->group = key | (curUnit->group & UGAI_FRIENDLY_MASK);
            else if (curUnit->group & key)
                curUnit->group = KEY_LID | (curUnit->group & UGAI_FRIENDLY_MASK); // i.e. not a button usable for group
        }
    }
    return 1;
}


unsigned getHotKeyWithin(uint32 keys) {
    if (getPrimaryHand() == RIGHT_HANDED) {
        if (keys & KEY_X) return KEY_X;
        if (keys & KEY_B) return KEY_B;
        if (keys & KEY_Y) return KEY_Y;
        if (keys & KEY_A) return KEY_A;
    } else {
        if (keys & KEY_UP)    return KEY_UP;
        if (keys & KEY_DOWN)  return KEY_DOWN;
        if (keys & KEY_LEFT)  return KEY_LEFT;
        if (keys & KEY_RIGHT) return KEY_RIGHT;
    }
    
    if (keys & KEY_BLUE)   return KEY_BLUE;
    if (keys & KEY_YELLOW) return KEY_YELLOW;
    if (keys & KEY_RED)    return KEY_RED;
    if (keys & KEY_GREEN)  return KEY_GREEN;
    
    return 0;
}


void inputHotKeys() {
    int i;
    int aSelectedUnit = -1;
    unsigned actualKey;
    struct Structure *curStructure = structure;
    struct Unit *curUnit = unit;
    enum UnitType unitType = 255;
    int setViewToSelectedUnits = -1;
    
    
    for (i=0; i<20; i++) {
        actualKey = BIT(i);
        if (getKeysHeldTimeMax(actualKey) == TRIGGER_HOTKEY_SAVE_MIN_INTERVAL) {
            if ((getPrimaryHand() == RIGHT_HANDED && (actualKey & (KEY_X | KEY_B | KEY_Y | KEY_A | KEY_BLUE | KEY_YELLOW | KEY_RED | KEY_GREEN))) ||
                (getPrimaryHand() == LEFT_HANDED && (actualKey & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_BLUE | KEY_YELLOW | KEY_RED | KEY_GREEN)))) {
                saveHotKey(actualKey) ? playSoundeffect(SE_BUTTONCLICK) : playSoundeffect(SE_CANNOT);
                return;
            }
        }
    }
    
    actualKey = getHotKeyWithin(getKeysOnUp());
    
    if (!actualKey) 
        return;
    
    /*
    // select held? add to existing group under hotkey.
    if (getPrimaryHand() == RIGHT_HANDED && (getKeysDown() & KEY_SELECT) && (actualKey & (KEY_X | KEY_B | KEY_Y | KEY_A | KEY_BLUE | KEY_YELLOW | KEY_RED | KEY_GREEN))) {
        addHotKey(actualKey) ? playSoundeffect(SE_BUTTONCLICK) : playSoundeffect(SE_CANNOT);
    }
    else
    if (getPrimaryHand() == LEFT_HANDED && (getKeysDown() & KEY_SELECT) && (actualKey & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_BLUE | KEY_YELLOW | KEY_RED | KEY_GREEN))) {
        addHotKey(actualKey) ? playSoundeffect(SE_BUTTONCLICK) : playSoundeffect(SE_CANNOT);
    }
    */
    
    #ifndef DEBUG_BUILD
    if (getKeysDown() & KEY_SELECT) {
        doUnitsSelectAll(); // select all FRIENDLY units, other than those that collect ore
        playSoundeffect(SE_BUTTONCLICK);
        return;
    }
    #endif
    
    // key long enough held? it has already been save time. return.
    if (getKeysHeldTimeMax(actualKey) > TRIGGER_HOTKEY_SAVE_MIN_INTERVAL)
        return;
    
    // select time.
    for (i=0; i<MAX_STRUCTURES_ON_MAP; i++, curStructure++) {
        if (curStructure->enabled) {
            if (curStructure->group == actualKey) {
                curStructure->selected = 1;
                setViewCurrentXY(curStructure->x + structureInfo[curStructure->info].width/2 - HORIZONTAL_WIDTH/2, curStructure->y + structureInfo[curStructure->info].height/2 - HORIZONTAL_HEIGHT/2);
                if (curStructure->side == FRIENDLY)
                    playSoundeffect(SE_STRUCTURE_SELECT);
            } else
                curStructure->selected = 0;
        }
    }
    if (i == MAX_STRUCTURES_ON_MAP) {
        for (i=0; i<MAX_UNITS_ON_MAP; i++, curUnit++) {
            if (curUnit->enabled) {
                if ((curUnit->group & actualKey) && curUnit->side == FRIENDLY) {
                    if (curUnit->selected && getKeysUpTimeMin(actualKey) < DOUBLE_TAP_HOTKEY_MAX_INTERVAL && setViewToSelectedUnits != 0)
                        setViewToSelectedUnits = 1;
                    else
                        setViewToSelectedUnits = 0;
                    aSelectedUnit = i;
                    curUnit->selected = 1;
                    if (unitType == 255 || unitInfo[curUnit->info].type > unitType)
                        unitType = unitInfo[curUnit->info].type;
                } else
                    curUnit->selected = 0;
            }
        }
    }
    setFocusOnUnitNr(-1);
    if (setViewToSelectedUnits == 1)
        setViewCurrentXY(unit[aSelectedUnit].x - HORIZONTAL_WIDTH/2, unit[aSelectedUnit].y - HORIZONTAL_HEIGHT/2);
    
    if (unitType == UT_FOOT)
        playSoundeffect(SE_FOOT_SELECT);
        //playSoundeffectControlled(SE_FOOT_SELECT, volumePercentageOfLocation(unit[aSelectedUnit].x, unit[aSelectedUnit].y, DISTANCE_TO_HALVE_SE_FOOT_SELECT), soundeffectPanningOfLocation(unit[aSelectedUnit].x, unit[aSelectedUnit].y));
    else if (unitType == UT_WHEELED)
        playSoundeffect(SE_WHEELED_SELECT);
        //playSoundeffectControlled(SE_UNIT_SELECT, volumePercentageOfLocation(unit[aSelectedUnit].x, unit[aSelectedUnit].y, DISTANCE_TO_HALVE_SE_UNIT_SELECT), soundeffectPanningOfLocation(unit[aSelectedUnit].x, unit[aSelectedUnit].y));
    else if (unitType == UT_TRACKED)
        playSoundeffect(SE_TRACKED_SELECT);
        //playSoundeffectControlled(SE_UNIT_SELECT, volumePercentageOfLocation(unit[aSelectedUnit].x, unit[aSelectedUnit].y, DISTANCE_TO_HALVE_SE_UNIT_SELECT), soundeffectPanningOfLocation(unit[aSelectedUnit].x, unit[aSelectedUnit].y));
    else if (unitType == UT_AERIAL)
        playSoundeffect(SE_TRACKED_SELECT);
        //playSoundeffectControlled(SE_UNIT_SELECT, volumePercentageOfLocation(unit[aSelectedUnit].x, unit[aSelectedUnit].y, DISTANCE_TO_HALVE_SE_UNIT_SELECT), soundeffectPanningOfLocation(unit[aSelectedUnit].x, unit[aSelectedUnit].y));
}


void setLevelLoadedViaLevelSelect(int value) {
    levelLoadedViaLevelSelect = value;
}

int isLevelLoadedViaLevelSelect() {
    return levelLoadedViaLevelSelect;
}
