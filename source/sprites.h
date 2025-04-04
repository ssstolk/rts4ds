// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _SPRITES_H_
#define _SPRITES_H_

#include <nds.h>


#define SPRITE_SIZE_8   0
#define SPRITE_SIZE_16  1
#define SPRITE_SIZE_32  2
#define SPRITE_SIZE_64  3



enum PlayScreenSpritesMode { PSSM_NORMAL, PSSM_EXPANDED };
void setPlayScreenSpritesMode(enum PlayScreenSpritesMode mode);

void initSpritesUsedInfoScreen();
//int getSpritesUsedInfoScreen();
void initSpritesUsedPlayScreen();
//int getSpritesUsedPlayScreen();

void setSpriteInfoScreen(int y, unsigned int shape,
                         int x, unsigned int size, unsigned int flipX, unsigned int flipY,
                         unsigned int priority, unsigned int palette, unsigned int graphics);
                         
void setSpritePlayScreen(int y, unsigned int shape,
                         int x, unsigned int size, unsigned int flipX, unsigned int flipY,
                         unsigned int priority, unsigned int palette, unsigned int graphics);

void setSpriteBorderPlayScreen(int x, int y, int x1, int y1);

void updateOAMafterVBlank();
//void updateOAMonHBlank();
//void handleVBlankInterruptSprites();
//void handleYTriggerInterrupt();

void init3DforExpandedSprites();

#endif
