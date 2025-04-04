// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _SHARED_H_
#define _SHARED_H_

#include <nds.h>

//#define DEBUG_BUILD   // now taken care of by Makefile (see target "debug")

#define SCREEN_REFRESH_RATE 60
#define FPS (isDSiMode() ? 60 : 30)

#define MAX_DESCRIPTION_LENGTH 200

#define SPRITE_EXPANDED_GFX  VRAM_B
#define SPRITE_EXPANDED_PAL  VRAM_F
#define BG_EXPANDED_PAL      VRAM_E

#define mapBG0  ((uint16*)SCREEN_BASE_BLOCK(28))
#define mapBG1  ((uint16*)SCREEN_BASE_BLOCK(29))
#define mapBG2  ((uint16*)SCREEN_BASE_BLOCK(30))
#define mapBG3  ((uint16*)SCREEN_BASE_BLOCK(31))

#define mapSubBG0  ((uint16*)SCREEN_BASE_BLOCK_SUB(31))
#define mapSubBG1  ((uint16*)SCREEN_BASE_BLOCK_SUB(30))

#define INFINITE_ARMOUR  9999999


enum PlayScreenBGPaletteSlot  { PS_BG_PAL_SLOT_3D /* entry unused */, PS_BG_PAL_SLOT_GUI, PS_BG_PAL_SLOT_OVERLAY_AND_STRUCTURES, PS_BG_PAL_SLOT_ENVIRONMENT };
enum PlayScreenBGPalette      { PS_BG_PAL_STRUCTURES, PS_BG_PAL_OVERLAY }; // subdivision is only needed for OVERLAY_AND_STRUCTURES
enum PlayScreenSpritesPalette { PS_SPRITES_PAL_PROJECTILES, PS_SPRITES_PAL_EXPLOSIONS, PS_SPRITES_PAL_SMOKE, PS_SPRITES_PAL_SHROUD, PS_SPRITES_PAL_GUI, PS_SPRITES_PAL_FACTIONS };

enum Side { FRIENDLY, ENEMY1, ENEMY2, ENEMY3, ENEMY4, MAX_SIDES };
enum Positioned { UP, RIGHT_UP, RIGHT, RIGHT_DOWN, DOWN, LEFT_DOWN, LEFT, LEFT_UP };

enum GroupAI { AWAITING_REPLACEMENT = BIT(24), TECHTREE_INSUFFICIENT = BIT(25), QUEUED_FOR_REPLACEMENT = BIT(26) }; // careful! needs to be higher than any UnitGroupAI

int nrstrlen(int nr);

int abs(int value);
int min(int lh, int rh);
int max(int lh, int rh);
int square_root(int value);

int math_power(int base, int power);
int randFixed();
int generateRandVariation(int delta);
int calculateDistance2(int curX, int curY, int tgtX, int tgtY);
int withinRange(int curX, int curY, int range, int tgtX, int tgtY);
int withinShootRange(int curX, int curY, int range, int projectile_info, int tgtX, int tgtY);
int sideUnitWithinRange(enum Side side, int curX, int curY, int range);
int sideUnitWithinShootRange(enum Side side, int curX, int curY, int range, int projectile_info);
int damagedFootWithinRange(enum Side side, int curX, int curY, int range);
enum Positioned positionedToFaceXY(int curX, int curY, int tgtX, int tgtY);
int rotateBaseToFace(enum Positioned positioned, enum Positioned positioned_new);
int rotateToFace(enum Positioned positioned, enum Positioned positioned_new);
int rotateToFaceXY(int curX, int curY, enum Positioned positioned, int tgtX, int tgtY);

void activateEntityView(int x, int y, int width, int height, int viewRange);

void alterXYAccordingToUnitMovement(unsigned int move, int move_aid, int speed, int *x, int *y);

unsigned int volumePercentageOfLocation(int x, int y, int distanceToHalveVolume);
unsigned int soundeffectPanningOfLocation(int x, int y);

void inputButtonScrolling();
void inputHotKeys();

void clearHotKey(int key);
int addHotKey(int key);
int saveHotKey(int key);

void setLevelLoadedViaLevelSelect(int value);
int  isLevelLoadedViaLevelSelect();

#endif
