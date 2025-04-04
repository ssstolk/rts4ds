// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _INPUTX_H_
#define _INPUTX_H_

#include <nds/ndstypes.h>
#include <nds/arm9/input.h>

// GH grip mapped key values
#define KEY_BLUE     BIT(14)
#define KEY_YELLOW   BIT(15)
#define KEY_RED      BIT(16)
#define KEY_GREEN    BIT(17)

uint32 getKeysDown();                   // on KEY_DOWN and KEY_PRESSED
int    getKeysHeldTimeMin(uint32 keys); // amount of frames the given keys have been pressed (the smallest value of them all)
int    getKeysHeldTimeMax(uint32 keys); // amount of frames the given keys have been pressed (the highest value of them all)
int    getKeysUpTimeMin(uint32 keys);   // amount of frames the given keys have been up      (the smallest value of them all)
int    getKeysUpTimeMax(uint32 keys);   // amount of frames the given keys have been up      (the highest value of them all)
void   initKeysHeldTime(uint32 keys);   // sets the keys held time to 0 of the keys
uint32 getKeysOnUp();                     // on KEY_UP
void   setKeysToIgnoreOnceOnUp(uint32 keys); // will NOT mention these keys in getKeysOnUp() for the first time they're up.

touchPosition touchReadLast();          // touch position just before touch's KEY_UP
touchPosition touchReadOrigin();        // touch position of touch's KEY_DOWN

void updateKeys(); // performs scanKeys and update the extra data needed for inputx functionality

#endif
