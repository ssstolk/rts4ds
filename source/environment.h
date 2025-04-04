// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_

#include <nds.h>

//#define DISABLE_SHROUD

#define MAX_TILES_ENVIRONMENT   (64*64)

#define MAX_ENVIRONMENT_ORE_LEVEL   400

extern unsigned int environment_widthmask;
extern unsigned int environment_widthshift;
#define X_FROM_TILE(tile)  ((tile)&environment_widthmask)
#define Y_FROM_TILE(tile)  ((tile)>>environment_widthshift)
#define TILE_FROM_XY(x,y)  (((y)<<environment_widthshift)+(x))


/*EnvironmentTileStatus*/
#define UNDISCOVERED 0
#define CLEAR_UP     BIT(0)
#define CLEAR_RIGHT  BIT(1)
#define CLEAR_DOWN   BIT(2)
#define CLEAR_LEFT   BIT(3)
#define CLEAR_ALL    (CLEAR_UP | CLEAR_RIGHT | CLEAR_DOWN | CLEAR_LEFT)

enum Traversability { UNTRAVERSABLE              = 'X',  /* i.e. buildings and impassable terrain types */
                      TRAVERSABLE                = ' ',
                      TRAVERSABLE_BY_NON_TRACKED = 'M' };

enum EnvironmentTileGraphics { SAND,
                                SANDCUSTOM,    SANDCUSTOM16  = SANDCUSTOM    + 15,
                                SANDHILL,      SANDHILL16    = SANDHILL      + 15,
                                SANDCHASM,     SANDCHASM16   = SANDCHASM     + 15,
                                ROCK,          ROCK16        = ROCK          + 15,
                                ROCKCUSTOM,    ROCKCUSTOM16  = ROCKCUSTOM    + 15,
                                ROCKCUSTOM17,  ROCKCUSTOM32  = ROCKCUSTOM17  + 15,
                                MOUNTAIN,      MOUNTAIN16    = MOUNTAIN      + 15,
                                ROCKCHASM,     ROCKCHASM16   = ROCKCHASM     + 15,
                                ORE,           ORE16         = ORE           + 15,
                                OREHILL,       OREHILL16     = OREHILL       + 15,
                                CHASMCUSTOM,   CHASMCUSTOM16 = CHASMCUSTOM   + 15,
                                CHASMCUSTOM17, CHASMCUSTOM32 = CHASMCUSTOM17 + 15,
                               NUMBER_OF_ENVIRONMENT_TILE_GRAPHICS
                              };

struct EnvironmentLayout {
    unsigned char  status;          /*      EnvironmentTileStatus   */
    unsigned char  traversability;  /* enum Traversability          */
    unsigned short graphics;        /* enum EnvironmentTileGraphics */
    int ore_level;
    int contains_overlay;   // -1 means none, larger is an overlay
    int contains_structure; // -1 means none, larger is a structure, less is a reference to another tile
    int contains_unit;      // -1 means none, larger is a unit,      less is a reference to a unit moving there
};


struct Environment {
    int width;
    int height;
    int ore_multiplier;
    struct EnvironmentLayout layout[MAX_TILES_ENVIRONMENT];
};



void adjustShroudType(int x, int y);

extern struct Environment environment;

int isEnvironmentTileBetween(int curTile, enum EnvironmentTileGraphics graphicsMin, enum EnvironmentTileGraphics graphicsMax);
void reduceEnvironmentOre(int curTile, int amount);

void initTileTraversability(struct EnvironmentLayout *envLayout);

void initEnvironment();
void initEnvironmentWithScenario();

void drawEnvironment();
void doEnvironmentLogic();
void loadEnvironmentGraphicsBG(int baseBg, int *offsetBg);
void loadEnvironmentGraphicsSprites(int *offsetSp);

int getEnvironmentSaveSize(void);
int getEnvironmentSaveData(void *dest, int max_size);

#endif
