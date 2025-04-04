// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "inputx.h"

#include "shared.h"
#include <nds/memory.h>

#define GH_GRIP_ID    0xF9FF
#define GH_GRIP_BUTTON_BLUE   0x08
#define GH_GRIP_BUTTON_YELLOW 0x10
#define GH_GRIP_BUTTON_RED    0x20
#define GH_GRIP_BUTTON_GREEN  0x40


int keysHeldTime[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // 0 through 17
int keysUpTime[]   = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // 0 through 17
uint32 keysOnUp       = 0;
uint32 keysUpToIgnore = 0;
touchPosition touchOrigin, touchLast;


uint32 getKeysDown() {
    uint32 result = keysDown() | keysHeld();
    uint8 gh_grip_buttons = ~SRAM[0];
    
    if (!isDSiMode()) {
        if (GBA_HEADER.is96h != 0x96 && GBAROM[0xFFFF] == GH_GRIP_ID) {
            if (gh_grip_buttons & GH_GRIP_BUTTON_BLUE)
                result |= KEY_BLUE;
            if (gh_grip_buttons & GH_GRIP_BUTTON_YELLOW)
                result |= KEY_YELLOW;
            if (gh_grip_buttons & GH_GRIP_BUTTON_RED)
                result |= KEY_RED;
            if (gh_grip_buttons & GH_GRIP_BUTTON_GREEN)
                result |= KEY_GREEN;
        }
    }
    
    return result;
}

int getKeysHeldTimeMin(uint32 keys) {
    int i;
    int result = -1;
    
    for (i=0; i<=17; i++) {
        if (keys & BIT(i)) {
            if (result == -1)
                result = keysHeldTime[i];
            else {
                if (keysHeldTime[i] < result)
                    result = keysHeldTime[i];
            }
        }
    }
    return (result == -1) ? 0 : result;
}

int getKeysHeldTimeMax(uint32 keys) {
    int i;
    int result = 0;
    
    for (i=0; i<=17; i++) {
        if (keys & BIT(i)) {
            if (keysHeldTime[i] > result)
                result = keysHeldTime[i];
        }
    }
    return result;
}

int getKeysUpTimeMin(uint32 keys) {
    int i;
    int result = -1;
    
    for (i=0; i<=17; i++) {
        if (keys & BIT(i)) {
            if (result == -1)
                result = abs(keysUpTime[i]);
            else {
                if (abs(keysUpTime[i]) < result)
                    result = abs(keysUpTime[i]);
            }
        }
    }
    return (result == -1) ? 0 : result;
}

int getKeysUpTimeMax(uint32 keys) {
    int i;
    int result = 0;
    
    for (i=0; i<=17; i++) {
        if (keys & BIT(i)) {
            if (abs(keysUpTime[i]) > result)
                result = abs(keysUpTime[i]);
        }
    }
    return result;
}

uint32 getKeysOnUp() {
    return keysOnUp;
}

touchPosition touchReadOrigin() {
    return touchOrigin;
}

touchPosition touchReadLast() {
    return touchLast;
}

void initKeysHeldTime(uint32 keys) {
    int i;
    
    for (i=0; i<=17; i++) {
        if (keys & BIT(i))
            keysHeldTime[i] = 0;
    }
}

void setKeysToIgnoreOnceOnUp(uint32 keys) {
    keysUpToIgnore = keys;
}

void updateKeys() {
    uint32 keys_down, keys_held, keys_up;
    int i;
    
    scanKeys();
    keys_down = keysDown();
    keys_held = keysHeld();
    
    for (i=0; i<=17; i++) {
        if (keysOnUp & BIT(i))
            keysHeldTime[i] = 0;
    }
    
    // GH Grip keys need to be assessed seperately
    for (i=14 /*KEY_BLUE*/; i<=17 /*KEY_GREEN*/; i++) {
        if (getKeysDown() & BIT(i)) {
            if (keysHeldTime[i] == 0)
                keys_down |= BIT(i);
            else
                keys_held |= BIT(i);
        }
    }
    
    keys_up = ~(keys_down | keys_held);   // libnds keysUp() means OnUp, really
    
    keysOnUp = 0;
    
    for (i=0; i<=17; i++) {
        if (keys_down & BIT(i)) {
            keysUpToIgnore &= ~(BIT(i));
            keysHeldTime[i] = 1;
            if (keysUpTime[i] > 0)
                keysUpTime[i] = -keysUpTime[i]; // store the last known value as a negative value
            if (BIT(i) == KEY_TOUCH) {
                touchRead(&touchOrigin);
                touchRead(&touchLast);
            }
        }
        else if (keys_held & BIT(i)) {
            if (keysHeldTime[i] > 0) {
                keysHeldTime[i]++;
                if (keysUpTime[i] > 0)
                    keysUpTime[i] = -keysUpTime[i]; // store the last known value as a negative value
                if (BIT(i) == KEY_TOUCH)
                    touchRead(&touchLast);
            }
        }
    
/*    for (i=0; i<14; i++) {
        if (keys_down & BIT(i)) {
            keysHeldTime[i]++;
            if (BIT(i) == KEY_TOUCH) {
                if (keys_down & KEY_TOUCH)
                    touchRead(&touchLast);
                if (keysHeldTime[i] == 1) { // special rules for touch ;)
                    if (keysDown() & KEY_TOUCH) 
                        touchRead(&touchOrigin);
                    else
                        keysHeldTime[i] = 0;
                }
            }
        }*/
        else if (keys_up & BIT(i)) {
            if (keysHeldTime[i] > 0) {
                keysUpTime[i] = 0;
                if (keysUpToIgnore & BIT(i)) {
                    keysHeldTime[i] = 0;
                    keysUpToIgnore &= ~(BIT(i));
                    continue;
                }
                keysOnUp |= BIT(i);
            }
            keysUpTime[i]++;
            if (keysUpTime[i] < 0) {
                keysUpTime[i]--;
            }
        }
    }
}
