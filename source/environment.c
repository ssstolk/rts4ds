// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "environment.h"

#include "fileio.h"

#include "game.h"
#include "radar.h"
#include "projectiles.h"
#include "settings.h"
#include "view.h"


#define CHASM_ANIMATION_FRAMES 4
#define CHASM_ANIMATION_FRAME_DURATION (FPS/4)


struct Environment environment;

static int base_environment;
static int base_shroud;

static uint16 sandchasmAni[CHASM_ANIMATION_FRAMES * 16*16/2];
static uint16 rockchasmAni[CHASM_ANIMATION_FRAMES * 16*16/2];
static uint16 *sandchasmAniBase;
static uint16 *rockchasmAniBase;
static int chasmAnimationTimer = 0;

static int customAniTimerInfo[5 * 16]; // SANDCUSTOM, ROCKCUSTOM, ROCKCUSTOM2, CHASMCUSTOM, CHASMCUSTOM2
static int customAniTimer[5 * 16];

static int paletteAniTimerInfo[1 + 16 + 1];
static int paletteAniDirection;
static int paletteAniNr;
static int paletteAniTimer;

unsigned int environment_widthmask;
unsigned int environment_widthshift;





inline void adjustShroudType(int x, int y) {
    int tile = TILE_FROM_XY(x,y);
    char statusMod = 0;
    
    if (x > 0) {
        if (environment.layout[tile-1].status) {
            environment.layout[tile-1].status |= CLEAR_RIGHT;
            statusMod |= CLEAR_LEFT;
        }
    } else
        statusMod |= CLEAR_LEFT;
    if (x+1 < environment.width) {
        if (environment.layout[tile+1].status) {
            environment.layout[tile+1].status |= CLEAR_LEFT;
            statusMod |= CLEAR_RIGHT;
        }
    } else
        statusMod |= CLEAR_RIGHT;
    if (y > 0) {
        if (environment.layout[tile - environment.width].status) {
            environment.layout[tile - environment.width].status |= CLEAR_DOWN;
            statusMod |= CLEAR_UP;
        }
    } else
        statusMod |= CLEAR_UP;
    if (y+1 < environment.height) {
        if (environment.layout[tile + environment.width].status) {
            environment.layout[tile + environment.width].status |= CLEAR_UP;
            statusMod |= CLEAR_DOWN;
        }
    } else
        statusMod |= CLEAR_DOWN;
    
    if (environment.layout[tile].status == UNDISCOVERED)
        setTileDirtyRadarDirtyBitmap(tile);
    environment.layout[tile].status |= statusMod;
}



void drawEnvironment() {
    int i, j, k;
    int graphics;
    int baseTile;
    int hflip, vflip;
    int envPalette;
    struct EnvironmentLayout *envLayout;
    
    int x = getViewCurrentX();
    int y = getViewCurrentY();
    
    envLayout = environment.layout + TILE_FROM_XY(x,y); // environment layout tile to start drawing
    if (paletteAniNr > 0) {
        envPalette = paletteAniNr - 1;
        if (envPalette > 15)
            envPalette = 15;
    } else
        envPalette = 0;
    
    for (i=0; i<HORIZONTAL_HEIGHT; i++) {
        for (j=0; j<HORIZONTAL_WIDTH; j++) {
            
            k = ACTION_BAR_SIZE * 64 + 64*i + 2*j;
            
            if (envLayout->status == UNDISCOVERED) {
                setSpritePlayScreen((i + ACTION_BAR_SIZE)*16, ATTR0_SQUARE,
                                    j*16, SPRITE_SIZE_16, 0, 0,
                                    0, PS_SPRITES_PAL_SHROUD, base_shroud);
            } else {
                if (envLayout->status != CLEAR_ALL) {
                    hflip = 0;
                    vflip = 0;
                    switch (envLayout->status) {
                        // three sides clear
                        case (CLEAR_UP | CLEAR_RIGHT | CLEAR_DOWN):
                            graphics = base_shroud + 4;
                            break;
                        case (CLEAR_RIGHT | CLEAR_DOWN | CLEAR_LEFT):
                            graphics = base_shroud + 5;
                            break;
                        case (CLEAR_DOWN | CLEAR_LEFT | CLEAR_UP):
                            graphics = base_shroud + 4;
                            hflip = 1;
                            break;
                        case (CLEAR_LEFT | CLEAR_UP | CLEAR_RIGHT):
                            graphics = base_shroud + 5;
                            vflip = 1;
                            break;
                        // two sides clear
                        case (CLEAR_UP | CLEAR_RIGHT):
                            graphics = base_shroud + 3;
                            break;
                        case (CLEAR_RIGHT | CLEAR_DOWN):
                            graphics = base_shroud + 3;
                            vflip = 1;
                            break;
                        case (CLEAR_DOWN | CLEAR_LEFT):
                            graphics = base_shroud + 3;
                            hflip = 1;
                            vflip = 1;
                            break;
                        case (CLEAR_LEFT | CLEAR_UP):
                            graphics = base_shroud + 3;
                            hflip = 1;
                            break;
                        // one side clear
                        case CLEAR_UP:
                            graphics = base_shroud + 1;
                            break;
                        case CLEAR_RIGHT:
                            graphics = base_shroud + 2;
                            break;
                        case CLEAR_DOWN:
                            graphics = base_shroud + 1;
                            vflip = 1;
                            break;
                        case CLEAR_LEFT:
                        default: // default should not happen, but the switch statement requires it
                            graphics = base_shroud + 2;
                            hflip = 1;
                            break;
                    }
                    
                    setSpritePlayScreen((i + ACTION_BAR_SIZE)*16, ATTR0_SQUARE,
                                        j*16, SPRITE_SIZE_16, hflip, vflip,
                                        0, PS_SPRITES_PAL_SHROUD, graphics);
                }
                
                graphics = envLayout->graphics;
                if (graphics >= SANDCUSTOM && graphics <= SANDCUSTOM16) { // did sandcustom animate?
                    if (customAniTimer[0 * 16 + graphics - SANDCUSTOM] < 0)
                        graphics++;
                } else if (graphics >= ROCKCUSTOM && graphics <= ROCKCUSTOM32) { // did rockcustom animate?
                    if (customAniTimer[1 * 16 + graphics - ROCKCUSTOM] < 0)
                        graphics++;
                } else if (graphics >= CHASMCUSTOM) { // did chasmcustom animate?
                    if (customAniTimer[3 * 16 + graphics - CHASMCUSTOM] < 0)
                        graphics++;
                }
                baseTile = (envPalette<<12) | (base_environment + graphics * 4);
                mapBG3[k]      = baseTile;
                mapBG3[k+1]    = baseTile+1;
                mapBG3[k+32]   = baseTile+2;
                mapBG3[k+1+32] = baseTile+3;
            }
            envLayout++;
        }
        envLayout += (environment.width - HORIZONTAL_WIDTH);
    }
}



void doEnvironmentLogic() {
    int i;
    uint16 *baseSandAni, *baseRockAni;
//    int gameSpeed = getGameSpeed();
    
    startProfilingFunction("doEnvironmentLogic");
    
    chasmAnimationTimer++;
    if (chasmAnimationTimer >= CHASM_ANIMATION_FRAMES * CHASM_ANIMATION_FRAME_DURATION)
        chasmAnimationTimer = 0;
    if (!(chasmAnimationTimer % CHASM_ANIMATION_FRAME_DURATION)) { // need to load in a new chasm animation frame!
        i = (chasmAnimationTimer / CHASM_ANIMATION_FRAME_DURATION) * 16*16/2;
        baseSandAni = sandchasmAni + i;
        baseRockAni = rockchasmAni + i;
        for (i=0; i<16*16/2; i++) {
            sandchasmAniBase[i] = baseSandAni[i];
            rockchasmAniBase[i] = baseRockAni[i];
        }
    }
    
    for (i=0; i<5*16; i++) {
        if (customAniTimerInfo[i] > 0) {
            customAniTimer[i]--; //-= gameSpeed;
            if (customAniTimer[i] < -customAniTimerInfo[i])
                customAniTimer[i] = customAniTimerInfo[i];
        }
    }
    
    if (paletteAniTimerInfo[paletteAniNr]) {
        if (--paletteAniTimer <= 0) {
            if (paletteAniNr == 0) {
                paletteAniDirection = 1;
                paletteAniNr = 2;
            } else if (paletteAniNr == 17) {
                paletteAniDirection = -1;
                paletteAniNr = 15;
            } else
                paletteAniNr += paletteAniDirection;
            paletteAniTimer = paletteAniTimerInfo[paletteAniNr];
        }
    } else if (paletteAniTimerInfo[1]) {
        if (paletteAniNr == 0) {
            paletteAniDirection = 1;
            paletteAniNr = 2;
            paletteAniTimer = paletteAniTimerInfo[paletteAniNr];
        } else if (paletteAniNr == 17) {
            paletteAniDirection = -1;
            paletteAniNr = 15;
            paletteAniTimer = paletteAniTimerInfo[paletteAniNr];
        }
    }
    
    stopProfilingFunction();
}




inline int isEnvironmentTileBetween(int curTile, enum EnvironmentTileGraphics graphicsMin, enum EnvironmentTileGraphics graphicsMax) {
    return (environment.layout[curTile].graphics >= graphicsMin && environment.layout[curTile].graphics <= graphicsMax);
}



enum EnvironmentTileGraphics minTileGraphics(enum EnvironmentTileGraphics graphics) {
    if (graphics == SAND)
        return SAND;
    return (((graphics - 1) / 16) * 16) + 1;
}


enum EnvironmentTileGraphics maxTileGraphics(enum EnvironmentTileGraphics graphics) {
    enum EnvironmentTileGraphics result;
    
    if (graphics == SAND)
        return SAND;
    
    result = (((graphics - 1) / 16) * 16) + 16;
    
    if (result == ROCK16 /*|| result == MOUNTAIN16*/)
        return ROCKCHASM16;
    if (result == ORE16)
        return OREHILL16;
    
    return result;
}



enum EnvironmentTileGraphics mapOreTileGraphicsBySurroundings(int x, int y) {
    int curTile = TILE_FROM_XY(x,y);
    struct EnvironmentLayout *envLayout = environment.layout + curTile;
    enum EnvironmentTileGraphics graphics, own_low, own_high;
    int gfx[16];
    int k;
    
    if (envLayout->ore_level <= 0)
        return SAND;
    
    graphics = envLayout->graphics;
    own_low = minTileGraphics(graphics);
    own_high = maxTileGraphics(graphics);
    
    for (k=0; k<16; k++)
        gfx[k] = 1;
        
    if (y!=0) {
        graphics = environment.layout[TILE_FROM_XY(x,y-1)].graphics;
        if (graphics < own_low || graphics > own_high) {
            gfx[1]=0; gfx[3]=0; gfx[5]=0; gfx[7]=0; gfx[9]=0; gfx[11]=0; gfx[13]=0; gfx[15]=0;
        } else {
            gfx[0]=0; gfx[2]=0; gfx[4]=0; gfx[6]=0; gfx[8]=0; gfx[10]=0; gfx[12]=0; gfx[14]=0;
        }
    }
    if (y!=environment.height-1) {
        graphics = environment.layout[TILE_FROM_XY(x,y+1)].graphics;
        if (graphics < own_low || graphics > own_high) {
            gfx[4]=0; gfx[5]=0; gfx[6]=0; gfx[7]=0; gfx[12]=0; gfx[13]=0; gfx[14]=0; gfx[15]=0;
        } else {
            gfx[0]=0; gfx[1]=0; gfx[2]=0; gfx[3]=0; gfx[8]=0; gfx[9]=0; gfx[10]=0; gfx[11]=0;
        }
    }
    if (x!=0) {
        graphics = environment.layout[TILE_FROM_XY(x-1,y)].graphics;
        if (graphics < own_low || graphics > own_high) {
            gfx[8]=0; gfx[9]=0; gfx[10]=0; gfx[11]=0; gfx[12]=0; gfx[13]=0; gfx[14]=0; gfx[15]=0;
        } else {
            gfx[0]=0; gfx[1]=0; gfx[2]=0; gfx[3]=0; gfx[4]=0; gfx[5]=0; gfx[6]=0; gfx[7]=0;
        }
    }
    if (x!=environment.width-1) {
        graphics = environment.layout[TILE_FROM_XY(x+1,y)].graphics;
        if (graphics < own_low || graphics > own_high) {
            gfx[2]=0; gfx[3]=0; gfx[6]=0; gfx[7]=0; gfx[10]=0; gfx[11]=0; gfx[14]=0; gfx[15]=0;
        } else {
            gfx[0]=0; gfx[1]=0; gfx[4]=0; gfx[5]=0; gfx[8]=0; gfx[9]=0; gfx[12]=0; gfx[13]=0;
        }
    }
    for (k=0; k<16; k++)
        if (gfx[15-k]) return (enum EnvironmentTileGraphics) (((int)own_low) + 15-k);
    return environment.layout[TILE_FROM_XY(x,y)].graphics;
}



void reduceEnvironmentOre(int curTile, int amount) {
    struct EnvironmentLayout *envLayout = environment.layout + curTile;
    int x = X_FROM_TILE(curTile);
    int y = Y_FROM_TILE(curTile);
    int oldOreLevel;
    
    if (envLayout->ore_level <= 0)
        return;
    
    oldOreLevel = envLayout->ore_level;
    envLayout->ore_level -= amount;
    if ((envLayout->ore_level <= 0 && amount > 0) ||
        (envLayout->ore_level <= ((MAX_ENVIRONMENT_ORE_LEVEL*environment.ore_multiplier)/2) && 
                    oldOreLevel > ((MAX_ENVIRONMENT_ORE_LEVEL*environment.ore_multiplier)/2) )) {
        envLayout->graphics = mapOreTileGraphicsBySurroundings(x, y);
        setTileDirtyRadarDirtyBitmap(curTile);
        
        if (x > 0) {
            if (environment.layout[curTile - 1].ore_level > 0) {
                environment.layout[curTile - 1].graphics = mapOreTileGraphicsBySurroundings(x-1, y);
                setTileDirtyRadarDirtyBitmap(curTile - 1);
            }
        }
        if (x < environment.width - 1) {
            if (environment.layout[curTile + 1].ore_level > 0) {
                environment.layout[curTile + 1].graphics = mapOreTileGraphicsBySurroundings(x+1, y);
                setTileDirtyRadarDirtyBitmap(curTile + 1);
            }
        }
        if (y > 0) {
            if (environment.layout[curTile - environment.width].ore_level > 0) {
                environment.layout[curTile - environment.width].graphics = mapOreTileGraphicsBySurroundings(x, y-1);
                setTileDirtyRadarDirtyBitmap(curTile - environment.width);
            }
        }
        if (y < environment.height - 1) {
            if (environment.layout[curTile + environment.width].ore_level > 0) {
                environment.layout[curTile + environment.width].graphics = mapOreTileGraphicsBySurroundings(x, y+1);
                setTileDirtyRadarDirtyBitmap(curTile + environment.width);
            }
        }
    }
}



void initPaletteEnvironmentAnimation(char *filename) {
    FILE *fp;
    char oneline[256];
    int i;
    
    fp = openFile(filename, FS_ENVIRONMENT_PALCYCLE_INFO);
    if (!fp) {
//error("could not open environment palcycle", filename);
        paletteAniTimerInfo[0] = 0;
        paletteAniTimerInfo[1] = 0;
        return;
    }
    for (i=0; i<1+16+1; i++) {
        readstr(fp, oneline);
        sscanf(oneline, "%i", paletteAniTimerInfo + i);
        paletteAniTimerInfo[i] *= FPS / 10; // was specified in 1/10th of a second, and now it's amount of frames at regular speed
    }
    closeFile(fp);
}


void initCustomEnvironmentAnimation(int *timerInfo, char *filename) {
    FILE *fp;
    char oneline[256];
    int i;
    
    fp = openFile(filename, FS_ENVIRONMENT_INFO);
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[ANIMATION]", strlen("[ANIMATION]")));
    for (i=0; i<16; i++) {
        readstr(fp, oneline);
        sscanf(oneline, "%i", timerInfo + i);
        timerInfo[i] *= FPS / 10; // was specified in 1/10th of a second, and now it's amount of frames at regular speed
    }
    closeFile(fp);
}


void initProjectilesCollission(enum EnvironmentTileGraphics graphics, char *filename) {
    FILE *fp;
    char oneline[256];
    char *curPosition;
    int i;
    
    fp = openFile(filename, FS_ENVIRONMENT_INFO);
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[GENERAL]", strlen("[GENERAL]")));
    readstr(fp, oneline); // skip Description
    readstr(fp, oneline); // skip Colour
    readstr(fp, oneline);
    curPosition = oneline + strlen("Impassable=");
    if (graphics == SAND)
        setProjectileImpassableOverEnvironment(SAND, curPosition[0] != '0');
    else {
        for (i=0; i<16; i++)
            setProjectileImpassableOverEnvironment(graphics + i, curPosition[i] != '0');
    }
    closeFile(fp);
}



void initEnvironment() {
    // nothing done here
}


void initTileTraversability(struct EnvironmentLayout *envLayout) {
    if (envLayout->graphics >= SANDCHASM && envLayout->graphics <= SANDCHASM16) {
        envLayout->traversability = UNTRAVERSABLE;
        return;
    }
    if (envLayout->graphics >= ROCKCHASM && envLayout->graphics <= ROCKCHASM16) {
        envLayout->traversability = UNTRAVERSABLE;
        return;
    }
    if (envLayout->graphics >= CHASMCUSTOM) {
        envLayout->traversability = UNTRAVERSABLE;
        return;
    }
    if (envLayout->graphics >= MOUNTAIN && envLayout->graphics <= MOUNTAIN16) {
        envLayout->traversability = TRAVERSABLE_BY_NON_TRACKED;
        return;
    }
    
    envLayout->traversability = TRAVERSABLE;
}

void initEnvironmentWithScenario() {
    int i, j;
    struct EnvironmentLayout *envLayout;
    char filename[256];
    char oneline[256];
    char *filePosition;
    FILE *fp;
    
    // initializing some values
    REG_BG3HOFS = 0;
    REG_BG3VOFS = 0;
    chasmAnimationTimer = 0;

    // load in the file data
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[MAP]", strlen("[MAP]")));
    readstr(fp, filename);
    readstr(fp, oneline);
    sscanf(oneline, "Width=%i", &environment.width);
    readstr(fp, oneline);
    sscanf(oneline, "Height=%i", &environment.height);
    
    environment_widthshift = 0;
    for (i=environment.width; i>1; i/=2) {
        environment_widthshift++; 
    }
    environment_widthmask = 0;
    for (i=0; i<environment_widthshift; i++) {
        environment_widthmask |= BIT(i); 
    }
    
    readstr(fp, oneline);
    if (sscanf(oneline, "OreMultiplier=%i", &environment.ore_multiplier) != 1)
        environment.ore_multiplier=1;  // default
    
    closeFile(fp);

    replaceEOLwithEOF(filename, 255);
    fp = openFile(filename+strlen("Map="), FS_SCENARIO_MAP);
    envLayout = environment.layout;
    for (i=0; i<environment.height; i++) {
        readstr(fp, oneline);
        for (j=0; j<environment.width; j++, envLayout++) {
            #ifdef DISABLE_SHROUD
            envLayout->status = ~UNDISCOVERED;
            #else
            envLayout->status = UNDISCOVERED;
            #endif
            envLayout->graphics = ((unsigned char*) oneline)[j] - 33;
            envLayout->ore_level = 0;
            if (envLayout->graphics >= ORE) {
                if (envLayout->graphics <= ORE16)
                    envLayout->ore_level = (MAX_ENVIRONMENT_ORE_LEVEL*environment.ore_multiplier)/2;
                else if (envLayout->graphics <= OREHILL16)
                    envLayout->ore_level = (MAX_ENVIRONMENT_ORE_LEVEL*environment.ore_multiplier);
            }
            initTileTraversability(envLayout);
        }
    }
    closeFile(fp);
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    // GRAPHICS section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[GRAPHICS]", strlen("[GRAPHICS]")));
    readstr(fp, oneline);
    replaceEOLwithEOF(oneline, 255);
    closeFile(fp);
    
    strcpy(filename, oneline + strlen("Environment="));
    if (filename[0] != 0)
        strcat(filename, "/");
    
    filePosition = filename + strlen(filename);

    strcpy(filePosition, "Sand");
    initProjectilesCollission(SAND, filename);
    strcpy(filePosition, "Sandhill");
    initProjectilesCollission(SANDHILL, filename);
    strcpy(filePosition, "Sandchasm");
    initProjectilesCollission(SANDCHASM, filename);
    strcpy(filePosition, "Rock");
    initProjectilesCollission(ROCK, filename);
    strcpy(filePosition, "Mountain");
    initProjectilesCollission(MOUNTAIN, filename);
    strcpy(filePosition, "Rockchasm");
    initProjectilesCollission(ROCKCHASM, filename);
    strcpy(filePosition, "Ore");
    initProjectilesCollission(ORE, filename);
    strcpy(filePosition, "Orehill");
    initProjectilesCollission(OREHILL, filename);
    
    strcpy(filePosition, "Sandcustom");
    initProjectilesCollission(SANDCUSTOM, filename);
    initCustomEnvironmentAnimation(customAniTimerInfo + 0 * 16, filename);
    strcpy(filePosition, "Rockcustom");
    initProjectilesCollission(ROCKCUSTOM, filename);
    initCustomEnvironmentAnimation(customAniTimerInfo + 1 * 16, filename);
    strcpy(filePosition, "Rockcustom2");
    initProjectilesCollission(ROCKCUSTOM17, filename);
    initCustomEnvironmentAnimation(customAniTimerInfo + 2 * 16, filename);
    strcpy(filePosition, "Chasmcustom");
    initProjectilesCollission(CHASMCUSTOM, filename);
    initCustomEnvironmentAnimation(customAniTimerInfo + 3 * 16, filename);
    strcpy(filePosition, "Chasmcustom2");
    initProjectilesCollission(CHASMCUSTOM17, filename);
    initCustomEnvironmentAnimation(customAniTimerInfo + 4 * 16, filename);
    
    for (i=0; i<5*16; i++)
        customAniTimer[i] = customAniTimerInfo[i];
    
    strcpy(filePosition, "Palette_cycling");
    initPaletteEnvironmentAnimation(filename);
    paletteAniNr        = 0;
    paletteAniDirection = 1;
    paletteAniTimer     = paletteAniTimerInfo[0];
}



void loadEnvironmentGraphicsBG(int baseBg, int *offsetBg) {
    char filename[256];
    char oneline[256];
    char *filePosition;
    int i;
    FILE *fp;
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    // GRAPHICS section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[GRAPHICS]", strlen("[GRAPHICS]")));
    readstr(fp, oneline);
    replaceEOLwithEOF(oneline, 255);
    closeFile(fp);
    
    strcpy(filename, oneline + strlen("Environment="));
    if (filename[0] != 0)
        strcat(filename, "/");
    filePosition = filename + strlen(filename);
    
    base_environment = (*offsetBg)/(8*8);
    strcpy(filePosition, "Sand");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_ENVIRONMENT_GRAPHICS);
    strcpy(filePosition, "Sandcustom");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_ENVIRONMENT_GRAPHICS);
    strcpy(filePosition, "Sandhill");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_ENVIRONMENT_GRAPHICS);
    strcpy(filePosition, "Sandchasm");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_ENVIRONMENT_GRAPHICS);
    strcpy(filePosition, "Rock");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_ENVIRONMENT_GRAPHICS);
    strcpy(filePosition, "Rockcustom");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_ENVIRONMENT_GRAPHICS);
    strcpy(filePosition, "Rockcustom2");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_ENVIRONMENT_GRAPHICS);
    strcpy(filePosition, "Mountain");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_ENVIRONMENT_GRAPHICS);
    strcpy(filePosition, "Rockchasm");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_ENVIRONMENT_GRAPHICS);
    strcpy(filePosition, "Ore");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_ENVIRONMENT_GRAPHICS);
    strcpy(filePosition, "Orehill");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_ENVIRONMENT_GRAPHICS);
    strcpy(filePosition, "Chasmcustom");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_ENVIRONMENT_GRAPHICS);
    strcpy(filePosition, "Chasmcustom2");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_ENVIRONMENT_GRAPHICS);
    
    sandchasmAniBase = (uint16*)(baseBg + base_environment*(8*8) + SANDCHASM16 * 8*8*4);
    for (i=0; i<16*16/2; i++)
        sandchasmAni[i] = sandchasmAniBase[i];
    strcpy(filePosition, "Sandchasm_ani");
    copyFileVRAM(sandchasmAni + 16*16/2, filename, FS_ENVIRONMENT_GRAPHICS);
    
    rockchasmAniBase = (uint16*)(baseBg + base_environment*(8*8) + ROCKCHASM16 * 8*8*4);
    for (i=0; i<16*16/2; i++)
        rockchasmAni[i] = rockchasmAniBase[i];
    strcpy(filePosition, "Rockchasm_ani");
    copyFileVRAM(rockchasmAni + 16*16/2, filename, FS_ENVIRONMENT_GRAPHICS);
}



void loadEnvironmentGraphicsSprites(int *offsetSp) {
    FILE *fp;
    char oneline[256];
    char filename[256];
    int i;

    if (offsetSp) {
        base_shroud = (*offsetSp)/(16*16);
        *offsetSp += (6 * 16*16);
        return;
    }
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    // GRAPHICS section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[GRAPHICS]", strlen("[GRAPHICS]")));
    for (i=0; i<4; i++)
        readstr(fp, oneline);
    replaceEOLwithEOF(oneline, 255);
    closeFile(fp);
    
    if (oneline[0] != 'S')
        filename[0] = 0;
    else {
        strcpy(filename, oneline + strlen("Shroud="));
        if (filename[0] != 0)
            strcat(filename, "/");
    }
    strcat(filename, "Shroud");
    
    copyFileVRAM(SPRITE_EXPANDED_GFX + ((base_shroud*(16*16))>>1), filename, FS_SHROUD_GRAPHICS);
}

int getEnvironmentSaveSize(void) {
    return sizeof(environment);
}

int getEnvironmentSaveData(void *dest, int max_size) {
    int size=sizeof(environment);
    
    if (size>max_size)
        return 0;

    memcpy (dest,&environment,size);
    return size;
}
