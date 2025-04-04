// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "radar.h"

#include "factions.h"
#include "environment.h"
#include "structures.h"
#include "units.h"
#include "playscreen.h"
#include "info.h"
#include "view.h"
#include "shared.h"
#include "inputx.h"
#include "fileio.h"


#define ANIMATION_RADAR_OVERLAY_FRAMES    8
#define ANIMATION_RADAR_OVERLAY_DURATION  ((FPS/30) * ANIMATION_RADAR_OVERLAY_FRAMES) /* one per game-frame */

#define FRIENDLY_HIT_COLOR0     (RGB15(30,20,7))
#define FRIENDLY_HIT_COLOR1     (RGB15(3,13,9))

#define MAX_NUMBER_OF_HITBOX    8

#define HITBOX_WIDTH            8
#define HITBOX_HEIGHT           5

#define HITBOX_TIMEOUT          (64*FPS/30)

struct HitBox {
    int min_x, min_y, max_x, max_y;
    int timeout;
};

struct HitBox hitBox[MAX_NUMBER_OF_HITBOX];


static uint16 radarEnvironmentColour[13];
static uint8  radarDirtyBitmap[MAX_TILES_ENVIRONMENT/8 + 1];

static int radarAlwaysEnabled;
static int radarStructureInfo;
static int radarFullPower;
static int radarOverlayTimer;

static int base_radar_overlay;

uint32 radarCX, radarCY; // only required for screenshotting purposes
uint16 radarScale;



void inputRadar(int x, int y) {
    static int forcedMoveUsed = 0;
    
    int mapWidth = environment.width;
    int mapHeight = environment.height;
    
    int properRadarInput = 0;
    
    if ((getKeysHeldTimeMin(KEY_TOUCH) > 1) || (getKeysOnUp() & KEY_TOUCH)) {
        
        if (mapWidth == mapHeight) {
            x -= (SCREEN_WIDTH - RADAR_MAX_SIZE) / 2;
            if (x >= 0 && x < RADAR_MAX_SIZE && y>=0 && y < RADAR_MAX_SIZE) { // proper input
                properRadarInput = 1;
                // x and y divide by scaling to get proper pixel on map
                x = (x<<8) / ((RADAR_MAX_SIZE<<8)/mapWidth);
                y = (y<<8) / ((RADAR_MAX_SIZE<<8)/mapWidth);
            }
        } else if (mapWidth > mapHeight) {
            x -= (SCREEN_WIDTH - RADAR_MAX_SIZE) / 2;
            y -= (RADAR_MAX_SIZE - ((mapHeight * RADAR_MAX_SIZE) / mapWidth)) / 2;
            if (x >= 0 && x < RADAR_MAX_SIZE && y>=0 && y < ((mapHeight * RADAR_MAX_SIZE) / mapWidth)) { // proper input
                properRadarInput = 1;
                // x and y divide by scaling to get proper pixel on map
                x = (x<<8) / ((RADAR_MAX_SIZE<<8)/mapWidth);
                y = (y<<8) / ((RADAR_MAX_SIZE<<8)/mapWidth);
            }
        } else { /* if (mapHeight > mapWidth) */
            x -= (SCREEN_WIDTH - ((mapWidth * RADAR_MAX_SIZE) / mapHeight)) / 2;
            if (x >= 0 && x < ((mapWidth * RADAR_MAX_SIZE) / mapHeight) && y>=0 && y < RADAR_MAX_SIZE) { // proper input
                properRadarInput = 1;
                // x and y divide by scaling to get proper pixel on map
                x = (x<<8) / ((RADAR_MAX_SIZE<<8)/mapHeight);
                y = (y<<8) / ((RADAR_MAX_SIZE<<8)/mapHeight);
            }
        }
        
        if (properRadarInput) {
            if (getModifier() == PSTM_ATTACK || getModifier() == PSTM_MOVE) {
                // give all units selected the forced command, and set the forced command back to NONE
                if (getModifier() == PSTM_ATTACK)
                    inputPlayScreenFriendlyForcedAttack(TILE_FROM_XY(x, y), 0);
                else
                    inputPlayScreenFriendlyForcedMove(TILE_FROM_XY(x, y));
                setModifierNone();
                forcedMoveUsed = 1;
            } else if (!forcedMoveUsed) {        
                // now to calculate the top-left pixel of the new viewing area for use with view adjustment
                x -= (HORIZONTAL_WIDTH/2);
                y -= (HORIZONTAL_HEIGHT/2);
                // set the new view
                setViewCurrentXY(x, y);
                setFocusOnUnitNr(-1); // scrolling took place, focus now lost
            }
        }
        
    }
    
    if (getKeysOnUp() & KEY_TOUCH)
        forcedMoveUsed = 0;
}


static inline uint16 getRadarEnvironmentColour(enum EnvironmentTileGraphics type) {
    if (type == SAND)
        return radarEnvironmentColour[0];
    return radarEnvironmentColour[((type-1) / 16) + 1];
}

void setAllDirtyRadarDirtyBitmap() {
    int i, j;
    int amountOfTiles = environment.width * environment.height;
    
    // make radar bitmap dirty
    for (i=0; i<amountOfTiles/8; i++)
        radarDirtyBitmap[i] = 0xFF;
    if (amountOfTiles < MAX_TILES_ENVIRONMENT) {
        radarDirtyBitmap[i] = 0;
        for (j=0; j<amountOfTiles%8; j++)
            radarDirtyBitmap[i] |= BIT(j);
        i++;
    }
    for (; i<MAX_TILES_ENVIRONMENT/8; i++)
        radarDirtyBitmap[i] = 0;
}

void setTileDirtyRadarDirtyBitmap(int tile) {
    radarDirtyBitmap[tile/8] |= BIT(tile%8);
}

void drawHitBox (int n) {

    int mapWidth = environment.width;
    int mapHeight = environment.height;
    int tx,ty;
    int i,j;
    int color = ((hitBox[n].timeout / 4) & 0x1) ? (FRIENDLY_HIT_COLOR0|BIT(15)) : (FRIENDLY_HIT_COLOR1|BIT(15));
    
    tx = hitBox[n].min_x - (HITBOX_WIDTH-(hitBox[n].max_x-hitBox[n].min_x))/2;
    if (tx<0) tx=0;
    else if (tx>(mapWidth-HITBOX_WIDTH)) tx=mapWidth-HITBOX_WIDTH;
    
    ty = hitBox[n].min_y - (HITBOX_HEIGHT-(hitBox[n].max_y-hitBox[n].min_y))/2;
    if (ty<0) ty=0;
    else if (ty>(mapHeight-HITBOX_HEIGHT)) ty=mapHeight-HITBOX_HEIGHT;
    
    // top line
    j = ty*128 + tx;
    for (i=0; i<HITBOX_WIDTH; i++)
        BG_GFX_SUB[j+i] = color;
    
    // bottom line
    j = (ty + HITBOX_HEIGHT-1)*128 + tx;
    for (i=0; i<HITBOX_WIDTH; i++)
        BG_GFX_SUB[j+i] = color;

    // left line
    j = ty*128 + tx;
    for (i=0; i<HITBOX_HEIGHT; i++)
        BG_GFX_SUB[j+i*128] = color;

    // right line
    j = ty*128 + tx + HITBOX_WIDTH-1;
    for (i=0; i<HITBOX_HEIGHT; i++)
        BG_GFX_SUB[j+i*128] = color;
}

void setDirtyHitBox (int n) {

    int mapWidth = environment.width;
    int mapHeight = environment.height;
    int tx,ty;
    int i,bit;
    
    tx = hitBox[n].min_x - (HITBOX_WIDTH-(hitBox[n].max_x-hitBox[n].min_x))/2;
    if (tx<0) tx=0;
    else if (tx>(mapWidth-HITBOX_WIDTH)) tx=mapWidth-HITBOX_WIDTH;
    
    ty = hitBox[n].min_y - (HITBOX_HEIGHT-(hitBox[n].max_y-hitBox[n].min_y))/2;
    if (ty<0) ty=0;
    else if (ty>(mapHeight-HITBOX_HEIGHT)) ty=mapHeight-HITBOX_HEIGHT;
    
    // top line
    bit = TILE_FROM_XY(tx, ty);
    for (i=0; i<HITBOX_WIDTH; i++)
        radarDirtyBitmap[(bit+i)/8] |= BIT((bit+i)%8);
    
    // bottom line
    bit = TILE_FROM_XY(tx, ty + HITBOX_HEIGHT-1);
    for (i=0; i<HITBOX_WIDTH; i++)
        radarDirtyBitmap[(bit+i)/8] |= BIT((bit+i)%8);

    // left line
    bit = TILE_FROM_XY(tx, ty);
    for (i=0; i<HITBOX_HEIGHT; i++)
        radarDirtyBitmap[(bit+i*mapWidth)/8] |= BIT((bit+i*mapWidth)%8);

    // right line
    bit = TILE_FROM_XY(tx + HITBOX_WIDTH-1, ty);
    for (i=0; i<HITBOX_HEIGHT; i++)
        radarDirtyBitmap[(bit+i*mapWidth)/8] |= BIT((bit+i*mapWidth)%8);
}

void displayHitRadar(int x, int y) {
    int i, min, chosen=-1;
    
    // check if x,y isn't yet undiscovered, otherwise skip that
    if (environment.layout[TILE_FROM_XY(x,y)].status!=UNDISCOVERED) {
     
        // cicle the hitBox searching a best fit
        for (i=0; i<MAX_NUMBER_OF_HITBOX; i++) {
            // an already in-use hitBox is the best place, if it fits
            if (hitBox[i].timeout>0) {
                // we check if this (x,y) in already IN the box
                // OR if the box can be moved to include this (x,y)
                if ( ((x>=hitBox[i].min_x) && (x<=hitBox[i].max_x)) ||
                     ((x<hitBox[i].min_x) && ((hitBox[i].min_x-x) < (HITBOX_WIDTH-(hitBox[i].max_x-hitBox[i].min_x)))) || 
                     ((x>hitBox[i].max_x) && ((x-hitBox[i].max_x) < (HITBOX_WIDTH-(hitBox[i].max_x-hitBox[i].min_x)))) ) {
                     
                    if ( ((y>=hitBox[i].min_y) && (y<=hitBox[i].max_y)) ||
                         ((y<hitBox[i].min_y) && ((hitBox[i].min_y-y) < (HITBOX_WIDTH-(hitBox[i].max_y-hitBox[i].min_y)))) || 
                         ((y>hitBox[i].max_y) && ((y-hitBox[i].max_y) < (HITBOX_WIDTH-(hitBox[i].max_y-hitBox[i].min_y)))) ) {
                        
                        // WOW, it fits! Leave this cycle immediatly...
                        chosen = i;
                        break;
                    }
                }
            }
         
            // a free hitBox could be a good fit
            if ((hitBox[i].timeout==0) && (chosen==-1))
                chosen=i;
        }
        
        // a hitbox SHOULD be assigned so if a best fit can't be found,
        // take the oldest one found in the array
        // if this happens TOO OFTEN you should consider increasing MAX_NUMBER_OF_HITBOX
        if (chosen<0) {
            min=HITBOX_TIMEOUT;
            for (i=0; i<MAX_NUMBER_OF_HITBOX; i++) {
                if (hitBox[i].timeout<=min) {
                    chosen=i;
                    min=hitBox[i].timeout;
                }
            }
            setDirtyHitBox(chosen);   // remove hitbox
            hitBox[chosen].timeout=0; // reset
        }
        
        // update the chosen hitbox
        if (hitBox[chosen].timeout==0) {
            hitBox[chosen].min_x=x;
            hitBox[chosen].max_x=x;
            hitBox[chosen].min_y=y;
            hitBox[chosen].max_y=y;
            hitBox[chosen].timeout=HITBOX_TIMEOUT;
        } else {
            // replace old hitbox
            setDirtyHitBox(chosen);
            if (x<hitBox[chosen].min_x) hitBox[chosen].min_x=x;
            if (x>hitBox[chosen].max_x) hitBox[chosen].max_x=x;
            if (y<hitBox[chosen].min_y) hitBox[chosen].min_y=y;
            if (y>hitBox[chosen].max_y) hitBox[chosen].max_y=y;
            // avoid blinking restart
            hitBox[chosen].timeout= (hitBox[chosen].timeout & 0x7) + HITBOX_TIMEOUT;
        }
        
    }
}

void doHitBoxLogic() {
    int i;
    for (i=0; i<MAX_NUMBER_OF_HITBOX; i++)
        if (hitBox[i].timeout>0)
            if (--hitBox[i].timeout==0)
                setDirtyHitBox(i);
}


void drawRadar() {
    int i, j, k=0, l=0, bit=0, toX, toY;
    int graphics;
    int offset;
    int mapWidth = environment.width;
/*    int mapHeight = environment.height;*/
    uint16 colour;
    struct EnvironmentLayout *environmentLayout = environment.layout;
    struct EnvironmentLayout *curLayout;
    uint16 sideColour[MAX_DIFFERENT_FACTIONS];
    
    startProfilingFunction("drawRadar");
    
    graphics = base_radar_overlay + ((radarOverlayTimer * ANIMATION_RADAR_OVERLAY_FRAMES) / ANIMATION_RADAR_OVERLAY_DURATION) * 4;
    
    
    offset = ((SCREEN_WIDTH - RADAR_MAX_SIZE) / 2) / 8;
/*    if (mapWidth < mapHeight) { // 32x128
        i = 0;
        offset += (((RADAR_MAX_SIZE - (RADAR_MAX_SIZE / 4)) / 2) / 8);
        toX = (RADAR_MAX_SIZE/4) / 16;
        toY = RADAR_MAX_SIZE/16;
    } else if (mapWidth > mapHeight) { // 128x32
        i = ((RADAR_MAX_SIZE - (RADAR_MAX_SIZE / 4)) / 2) / 16;
        offset += i * 64;
        toX = RADAR_MAX_SIZE/16;
        toY = (RADAR_MAX_SIZE/16) - i;
    } else { // 32x32 or 64x64*/
        i = 0;
        toX = RADAR_MAX_SIZE/16;
        toY = RADAR_MAX_SIZE/16;
/*    }*/
    
    for (; i<toY; i++, offset+=64) {
        for (j=0; j<toX; j++) {
            mapSubBG0[offset + j*2         ] = graphics;
            mapSubBG0[offset + j*2 + 1     ] = graphics + 1;
            mapSubBG0[offset + j*2     + 32] = graphics + 2;
            mapSubBG0[offset + j*2 + 1 + 32] = graphics + 3;
        }
    }
    
    
    if (!radarAlwaysEnabled) {
        if (getPowerConsumation(FRIENDLY) > getPowerGeneration(FRIENDLY)) {
            if (radarFullPower) {
                radarFullPower = 0;
                setAllDirtyRadarDirtyBitmap();
            }
        } else {
            if (radarStructureInfo != -1) {
                for (i=0; i<MAX_STRUCTURES_ON_MAP; i++) {
                    if (structure[i].info == radarStructureInfo && structure[i].enabled && structure[i].side == FRIENDLY)
                        break;
                }
                if (radarFullPower != (i < MAX_STRUCTURES_ON_MAP))
                    setAllDirtyRadarDirtyBitmap();
                radarFullPower = (i < MAX_STRUCTURES_ON_MAP);
            }
        }
    }
    
    startProfilingFunction("drawRadar (fill)");
    
    for (i=0; i<getAmountOfSides(); i++)
        sideColour[i] = factionInfo[getFaction(i)].colour;
    
    
    for (i=0; i<MAX_TILES_ENVIRONMENT/8 + 1; i++) {
        if (radarDirtyBitmap[i]) {
            for (j=0; j<8; j++) {
                if (radarDirtyBitmap[i] & BIT(j)) {
                    colour = BIT(15) | RGB15(5, 5, 5); // black for radar default colour
                    bit = 8*i + j;
                    curLayout = &environmentLayout[bit];
                    if (curLayout->status != UNDISCOVERED) {
                        l = curLayout->contains_unit;
                        if (radarFullPower && l >= 0)
                            colour = sideColour[unit[l].side];
                        else {
                            l = curLayout->contains_structure;
                            if (l < -1)
                                l = (curLayout+(l+1))->contains_structure;
                            if (l >= 0)
                                colour = sideColour[structure[l].side];
                            else if (radarFullPower)
                                colour = getRadarEnvironmentColour(curLayout->graphics);
                        }
                    }
                    // colour has been determined, now to draw it
                    BG_GFX_SUB[Y_FROM_TILE(bit)*128 + X_FROM_TILE(bit)] = BIT(15) | colour;
                }
            }
            radarDirtyBitmap[i] = 0;  // make this clean
        }
    }
    
    stopProfilingFunction();
    
    // draw hitboxes (processing is done outside)
    for (i=0; i<MAX_NUMBER_OF_HITBOX; i++) {
        if (hitBox[i].timeout>0) {
            // the hitbox is active: draw it - only if radar is ON!
            if (radarFullPower)
                drawHitBox (i);
        }
    }
    
    startProfilingFunction("drawRadar (white box)"); // takes little time
    
    // create the white box
    k = HORIZONTAL_WIDTH;
    l = HORIZONTAL_HEIGHT;
    
    // top line
    j = getViewCurrentY()*128 + getViewCurrentX();
    bit = TILE_FROM_XY(getViewCurrentX(), getViewCurrentY());
    for (i=0; i<k; i++) {
        BG_GFX_SUB[j+i] = RGB15(31,31,31) | BIT(15);
        radarDirtyBitmap[(bit+i)/8] |= BIT((bit+i)%8);
    }
    // bottom line
    j = (getViewCurrentY() + l-1)*128 + getViewCurrentX();
    bit = TILE_FROM_XY(getViewCurrentX(), getViewCurrentY() + l-1);
    for (i=0; i<k; i++) {
        BG_GFX_SUB[j+i] = RGB15(31,31,31) | BIT(15);
        radarDirtyBitmap[(bit+i)/8] |= BIT((bit+i)%8);
    }
    // left line
    j = getViewCurrentY()*128 + getViewCurrentX();
    bit = TILE_FROM_XY(getViewCurrentX(), getViewCurrentY());
    for (i=0; i<l; i++) {
        BG_GFX_SUB[j+i*128] = RGB15(31,31,31) | BIT(15);
        radarDirtyBitmap[(bit+i*mapWidth)/8] |= BIT((bit+i*mapWidth)%8);
    }
    // right line
    j = getViewCurrentY()*128 + getViewCurrentX() + k-1;
    bit = TILE_FROM_XY(getViewCurrentX() + k-1, getViewCurrentY());
    for (i=0; i<l; i++) {
        BG_GFX_SUB[j+i*128] = RGB15(31,31,31) | BIT(15);
        radarDirtyBitmap[(bit+i*mapWidth)/8] |= BIT((bit+i*mapWidth)%8);
    }
    
    stopProfilingFunction();
    stopProfilingFunction();
}

int getRadarCY() {
    return radarCY;
}

#define FIX_DIV(a,b)    ((((a) << 9) / (b) + 1) >> 1)

void positionRadar() {
    int mapWidth = environment.width;
    int mapHeight = environment.height;
    int xo, yo;
    
    if (mapWidth == mapHeight) {
        xo = -((SCREEN_WIDTH - RADAR_MAX_SIZE) << 8) / 2;
        yo = 0;
    } else if (mapWidth > mapHeight) {
        xo = -((SCREEN_WIDTH - RADAR_MAX_SIZE) << 8) / 2;
        yo = -((RADAR_MAX_SIZE - ((mapHeight * RADAR_MAX_SIZE) / mapWidth)) << (8 + (mapWidth == 128)));
    } else /*if (mapHeight > mapWidth)*/ {
        xo = -((SCREEN_WIDTH - ((mapWidth * RADAR_MAX_SIZE) / mapHeight)) << (8 + (mapHeight == 128)));
        yo = 0;
    }
    
    yo -= (getScreenScrollingInfoScreen() * 256);
    
    radarCX = (xo * FIX_DIV(mapWidth,  RADAR_MAX_SIZE)) >> 8;
    REG_BG2X_SUB = radarCX;
    radarCY = (yo * FIX_DIV(mapHeight, RADAR_MAX_SIZE)) >> 8;
    REG_BG2Y_SUB = radarCY;
}

inline void scaleRadar() {
    int mapWidth = environment.width;
    int mapHeight = environment.height;
    
    if (mapWidth >= mapHeight)
        radarScale = FIX_DIV(mapWidth, RADAR_MAX_SIZE);
    else
        radarScale = FIX_DIV(mapHeight, RADAR_MAX_SIZE);
    REG_BG2PA_SUB = radarScale;
    REG_BG2PD_SUB = radarScale;
}

void drawRadarBG() {
    int i;
    
    for (i=0; i<(32*32); i++)
        mapSubBG0[i] = 0;
    
    for (i=0; i<(128*128); i++) // (128*128) being the size of this BG
        BG_GFX_SUB[i] = 0; // does not show up, not having set BIT(15)
    
    // scale radar
    scaleRadar();
    
    // position radar
    positionRadar();
    
    setAllDirtyRadarDirtyBitmap();
}

void doRadarLogic() {
    radarOverlayTimer++;
    if (radarOverlayTimer >= ANIMATION_RADAR_OVERLAY_DURATION)
        radarOverlayTimer = 0;
    doHitBoxLogic();
}

void loadRadarGraphics(int baseBg, int *offsetBg, int *offsetSp) {
    char filename[256];
    strcpy(filename, "radar_overlay_");
    strcat(filename, factionInfo[getFaction(FRIENDLY)].name);
    base_radar_overlay = (*offsetBg)/(8*8);
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_GUI_GRAPHICS);
}

void initEnvironmentColour(int nr, char *name) {
    FILE *fp;
    char filename[256];
    char oneline[256];
    int i, j, k;
    
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
    
    strcat(filename, name);
    fp = openFile(filename, FS_ENVIRONMENT_INFO);
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[GENERAL]", strlen("[GENERAL]")));
    readstr(fp, oneline);
    readstr(fp, oneline);
    sscanf(oneline, "Colour=%i,%i,%i", &i, &j, &k);
    closeFile(fp);
    radarEnvironmentColour[nr] = RGB15(i, j, k);
}

void initRadar() {
    FILE *fp;
    char oneline[256];
    int i;
    int j, k;
    
    initEnvironmentColour(0,  "Sand");
    initEnvironmentColour(1,  "Sandcustom");
    initEnvironmentColour(2,  "Sandhill");
    initEnvironmentColour(3,  "Sandchasm");
    initEnvironmentColour(4,  "Rock");
    initEnvironmentColour(5,  "Rockcustom");
    initEnvironmentColour(6,  "Rockcustom2");
    initEnvironmentColour(7,  "Mountain");
    initEnvironmentColour(8,  "Rockchasm");
    initEnvironmentColour(9,  "Ore");
    initEnvironmentColour(10, "Orehill");
    initEnvironmentColour(11, "Chasmcustom");
    initEnvironmentColour(12, "Chasmcustom2");
    
    radarStructureInfo = -1;
    for (i=0; i<MAX_DIFFERENT_STRUCTURES; i++) {
        if (!strncmp("Radar", structureInfo[i].name, strlen(structureInfo[i].name))) {
            radarStructureInfo = i;
            break;
        }
    }
    
    radarFullPower = 0;
    radarOverlayTimer = 0;
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    // RADAR section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[RADAR]", strlen("[RADAR]")) && strncmp(oneline, "[GRAPHICS]", strlen("[GRAPHICS]")));
    if (oneline[1] == 'R') {
        readstr(fp, oneline);
        sscanf(oneline, "Enabled=%i", &radarAlwaysEnabled);
        if (radarAlwaysEnabled)
            radarFullPower = 1;
        readstr(fp, oneline);
        while (oneline[0] != '[') {
            sscanf(oneline, "Cleared=%i,%i,%i", &i, &j, &k);
            activateEntityView(i, j, 1, 1, k);
            readstr(fp, oneline);
        }
    } else
        radarAlwaysEnabled = 0;
    closeFile(fp);
    
    // reset hitBox
    for (i=0; i<MAX_NUMBER_OF_HITBOX; i++)
      hitBox[i].timeout=0;
}

void dataSaveGameRadar(unsigned short *data) {
    //  this generates a 64x64 15bpp bitmap into memory location pointed by data
    //  works for 32x32, 128xHHH, WWWx128, WWxHH (64x64 and smaller) maps
    int i, j, l;
    struct EnvironmentLayout *environmentLayout = environment.layout;
    struct EnvironmentLayout *curLayout;
    unsigned short sideColour[MAX_DIFFERENT_FACTIONS];
    unsigned short colour=0; // =0 to suppress warning
    unsigned hofs, vofs;
    
    // (radarFullPower overrides the same-named global var, which may not be up to date...)
    int radarFullPower=0;   // set to OFF
    if (radarAlwaysEnabled) {
        radarFullPower=1;   //  radar ON anyway
    } else {
        if (getPowerConsumation(FRIENDLY) <= getPowerGeneration(FRIENDLY)) {  // enough power?
            // check if we've got a radar!
            for (i=0; i<MAX_STRUCTURES_ON_MAP; i++) {
                if (structure[i].info == radarStructureInfo && structure[i].enabled && structure[i].side == FRIENDLY) {
                    radarFullPower = 1;
                    break;
                }
            }
        }
    }
    
    for (i=0; i<getAmountOfSides(); i++)
        sideColour[i] = factionInfo[getFaction(i)].colour;
    

    //  now we have 3 different ways of generating the map, according to the size:
    if ((environment.width==32) && (environment.height==32)) {
        // map: 32x32. 'zoom' radar image 2x
        for (j=0; j<environment.height; j++) {
            for (i=0; i<environment.width; i++) {
                colour = RGB15(5, 5, 5);        // radar default colour
                curLayout = &environmentLayout[TILE_FROM_XY(i,j)];
                if (curLayout->status != UNDISCOVERED) {
                    l = curLayout->contains_unit;
                    if (radarFullPower && l >= 0)
                        colour = sideColour[unit[l].side];
                    else {
                        l = curLayout->contains_structure;
                        if (l < -1)
                            l = (curLayout+(l+1))->contains_structure;
                        if (l >= 0)
                            colour = sideColour[structure[l].side];
                        else if (radarFullPower)
                            colour = getRadarEnvironmentColour(curLayout->graphics);
                    }
                }
                // colour has been determined, now save it
                data[j*2*64+i*2]  = BIT(15) | colour;
                data[j*2*64+i*2+1]  = BIT(15) | colour;
                data[(j*2+1)*64+i*2]  = BIT(15) | colour;
                data[(j*2+1)*64+i*2+1] = BIT(15) | colour;
            }
        }
    } else if ((environment.width==128) || (environment.height==128)) {
        // map: 128x32 or 32x128. We should 'shrink' it to half size and fill around
        
        if (environment.height==128) {
            hofs=(64-(environment.width/2))/2;
            // fill left and right areas of the radar image
            for (j=0;j<environment.height;j++) {
                for (i=0;i<hofs;i++)
                    data[j*64+i]=BIT(15); // 'BLACK'
                for (i=(environment.width/2)+hofs;i<64;i++)
                    data[j*64+i]=BIT(15); // 'BLACK'
            }
        } else
            hofs=0;
        
        if (environment.width==128) {
            vofs=(64-(environment.height/2))/2;
            // fill top and bottom areas of the radar image
            for (i=0;i<environment.width;i++) {
                for (j=0;j<vofs;j++)
                    data[j*64+i]=BIT(15); // 'BLACK'
                for (j=(environment.height/2)+vofs;j<64;j++)
                    data[j*64+i]=BIT(15); // 'BLACK'
            }
        } else
            vofs=0;
     
        //  fill radar image, 1 pixel for 2x2 tiles
        for (j=0; j<environment.height; j++) {
            for (i=0; i<environment.width; i++) {
                if (((j%2)==0) && ((i%2)==0)) {
                    // 1st pixel of four
                    colour = RGB15(5, 5, 5);        // radar default colour
                    curLayout = &environmentLayout[TILE_FROM_XY(i,j)];
                    if (curLayout->status != UNDISCOVERED) {
                        l = curLayout->contains_unit;
                        if (radarFullPower && l >= 0)
                            colour = sideColour[unit[l].side];
                        else {
                            l = curLayout->contains_structure;
                            if (l < -1)
                                l = (curLayout+(l+1))->contains_structure;
                            if (l >= 0)
                                colour = sideColour[structure[l].side];
                            else if (radarFullPower)
                                colour = getRadarEnvironmentColour(curLayout->graphics);
                        }
                    }
                } else {
                    curLayout = &environmentLayout[TILE_FROM_XY(i,j)];
                    if (curLayout->status != UNDISCOVERED) {
                        l = curLayout->contains_unit;
                        if (radarFullPower && l >= 0)
                            colour = sideColour[unit[l].side]; // BETA: check if enemy
                        else {
                            l = curLayout->contains_structure;
                            if (l < -1)
                                l = (curLayout+(l+1))->contains_structure;
                            if (l >= 0)
                                colour = sideColour[structure[l].side]; // BETA: check if enemy
                        }
                    }
                    
                    // if colour has been determined, save it
                    if (((j%2)!=0) && ((i%2)!=0))
                        data[(j/2+vofs)*64+(i/2+hofs)] = BIT(15) | colour;
                }
            }
        }
        
    } else {
        // maps: 64x64 and everything that fits it but bigger than 32x32
        
        if (environment.width<64) {
            hofs=(64-environment.width)/2;
            // fill left and right areas of the radar image
            for (j=0;j<environment.height;j++) {
                for (i=0;i<hofs;i++)
                    data[j*64+i]=BIT(15); // 'BLACK'
                for (i=environment.width+hofs;i<64;i++)
                    data[j*64+i]=BIT(15); // 'BLACK'
            }
        } else
            hofs=0;
        
        if (environment.height<64) {
            vofs=(64-environment.height)/2;
            // fill top and bottom areas of the radar image
            for (i=0;i<environment.width;i++) {
                for (j=0;j<vofs;j++)
                    data[j*64+i]=BIT(15); // 'BLACK'
                for (j=environment.height+vofs;j<64;j++)
                    data[j*64+i]=BIT(15); // 'BLACK'
            }
        } else
            vofs=0;
        
        for (j=0; j<environment.height; j++) {
            for (i=0; i<environment.width; i++) {
                colour = RGB15(5, 5, 5);        // radar default colour
                curLayout = &environmentLayout[TILE_FROM_XY(i,j)];
                if (curLayout->status != UNDISCOVERED) {
                    l = curLayout->contains_unit;
                    if (radarFullPower && l >= 0)
                        colour = sideColour[unit[l].side];
                    else {
                        l = curLayout->contains_structure;
                        if (l < -1)
                            l = (curLayout+(l+1))->contains_structure;
                        if (l >= 0)
                            colour = sideColour[structure[l].side];
                        else if (radarFullPower)
                            colour = getRadarEnvironmentColour(curLayout->graphics);
                    }
                }
                // colour has been determined, now save it
                data[(j+vofs)*64+(i+hofs)] = BIT(15) | colour;
            }
        }
    }
}
