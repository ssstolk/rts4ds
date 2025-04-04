// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "overlay.h"

#include "environment.h"
#include "view.h"
#include "settings.h"
#include "soundeffects.h"
#include "fileio.h"

#define OVERLAY_TRACKS_SHORT_DURATION  ((2*(2*FPS)) / getGameSpeed())
#define OVERLAY_TRACKS_LONG_DURATION   ((2*(4*FPS)) / getGameSpeed())
#define OVERLAY_SHOT_DURATION          ((2*(10*FPS)) / getGameSpeed())
#define OVERLAY_DEAD_FOOT_DURATION     ((2*(5*FPS)) / getGameSpeed())
#define OVERLAY_BLOOD_DURATION         ((2*(5*FPS)) / getGameSpeed())

struct Overlay overlay[MAX_OVERLAY_ON_MAP];

static int base_overlay[NUMBER_OF_OVERLAY_TYPES];




void addStructureDestroyedOverlay(int x, int y, int width, int height) { // keep in mind that the Destroyed gfx is 4x3
    int i, j;
    int curOverlayNr;
    struct EnvironmentLayout *envLayout = environment.layout;
    int base = MAX_OVERLAY_ON_MAP + MAX_PERMANENT_OVERLAY_TYPES + (rand() % ((3+1) - height)) * 4 + (rand() % ((4+1) - width));
    
    for (i=0; i<height; i++) {
        for (j=0; j<width; j++) {
            curOverlayNr = envLayout[TILE_FROM_XY(x+j, y+i)].contains_overlay;
            if (curOverlayNr != -1) {
                if (curOverlayNr < MAX_OVERLAY_ON_MAP)
                   overlay[curOverlayNr].enabled = 0;
                else if (curOverlayNr - MAX_OVERLAY_ON_MAP < MAX_PERMANENT_OVERLAY_TYPES)
                    continue;
            }
            envLayout[TILE_FROM_XY(x+j, y+i)].contains_overlay = base + 4*i + j;
        }
    }
}


void clearStructureDestroyedOverlay(int x, int y, int width, int height) {
    int i, j;
    struct EnvironmentLayout *envLayout = environment.layout;
    
    for (i=0; i<height; i++) {
        for (j=0; j<width; j++) {
            if (envLayout[TILE_FROM_XY(j, i)].contains_overlay >= MAX_OVERLAY_ON_MAP + MAX_PERMANENT_OVERLAY_TYPES)
                envLayout[TILE_FROM_XY(j, i)].contains_overlay = -1;
        }
    }
}


int addOverlay(int x, int y, enum OverlayType type, int frame) {
    static struct Overlay prevOverlay;
    int i;
    int curOverlayNr;
    struct Overlay *curOverlay;
    
    curOverlayNr = environment.layout[TILE_FROM_XY(x,y)].contains_overlay;
    
    if (curOverlayNr >= MAX_OVERLAY_ON_MAP + MAX_PERMANENT_OVERLAY_TYPES) {
        if (type < OT_DESTROYED)
            return -1;
    } else if (curOverlayNr >= MAX_OVERLAY_ON_MAP) {
        return -1;
    } else if (curOverlayNr != -1) {
        if (type < overlay[curOverlayNr].type)
            return -1;
        prevOverlay = overlay[curOverlayNr];
        overlay[curOverlayNr].enabled = 0;
    }
    
    for (i=0, curOverlay=overlay; i<MAX_OVERLAY_ON_MAP; i++, curOverlay++) {
        if (!curOverlay->enabled) {
            curOverlay->enabled = 1;
            curOverlay->x = x;
            curOverlay->y = y;
            curOverlay->type = type;
            if (curOverlayNr != -1 && (type == OT_ROCK_SHOT || type == OT_SAND_SHOT) && type == prevOverlay.type) {
                curOverlay->frame = prevOverlay.frame;
/*              if (type == OT_ROCK_SHOT && base_overlay[OT_ROCK_SHOT] != base_overlay[OT_SAND_SHOT]) {
                    if ((base_overlay[OT_ROCK_SHOT + 1] - base_overlay[OT_ROCK_SHOT]) / 4 > (prevOverlay.frame + 2))
                        curOverlay->frame = prevOverlay.frame + 2;
                } else {
                    if ((base_overlay[OT_SAND_SHOT + 1] - base_overlay[OT_SAND_SHOT]) / 4 > (prevOverlay.frame + 2))
                        curOverlay->frame = prevOverlay.frame + 2;
                }
*/              if (((base_overlay[(type == OT_ROCK_SHOT && base_overlay[OT_ROCK_SHOT] != base_overlay[OT_SAND_SHOT]) ? (OT_SAND_SHOT) : (OT_SAND_SHOT+1)] -
                      base_overlay[type]) / 4) > (curOverlay->frame + 2))
                    curOverlay->frame += 2;
//if (type == OT_SAND_SHOT && curOverlay->frame > 2) errorI4(prevOverlay.frame, type, base_overlay[OT_SAND_SHOT + 1], base_overlay[OT_SAND_SHOT]);
            } else if (type == OT_ROCK_SHOT || type == OT_SAND_SHOT || type == OT_BLOOD)
                curOverlay->frame = (x + y) & 1;
            else
                curOverlay->frame = frame;
            curOverlay->timer = 0;
            environment.layout[TILE_FROM_XY(x,y)].contains_overlay = i;
            if (type == OT_SAND_SHOT)
                playSoundeffectControlled(SE_IMPACT_SAND, volumePercentageOfLocation(x, y, DISTANCE_TO_HALVE_SE_IMPACT_SAND), soundeffectPanningOfLocation(x, y));
            return i;
        }
    }
    return -1;
}

void drawOverlay() {
    int i, j;
    int mapTile, baseTile;

    mapTile = TILE_FROM_XY(getViewCurrentX(), getViewCurrentY()); // tile to start drawing
    
    for (i=0; i<HORIZONTAL_HEIGHT; i++) {
        for (j=0; j<HORIZONTAL_WIDTH; j++) {
            baseTile = environment.layout[mapTile].contains_overlay;
            if (baseTile >= 0) {
                if (baseTile >= MAX_OVERLAY_ON_MAP + MAX_PERMANENT_OVERLAY_TYPES)
                    baseTile = base_overlay[OT_DESTROYED] + ((baseTile - (MAX_OVERLAY_ON_MAP + MAX_PERMANENT_OVERLAY_TYPES)) * 4);
                else if (baseTile >= MAX_OVERLAY_ON_MAP)
                    baseTile = base_overlay[OT_PERMANENT] + ((baseTile - (MAX_OVERLAY_ON_MAP)) * 4);
                else if (overlay[baseTile].type <= OT_GRUBS2)
                    baseTile = (PS_BG_PAL_OVERLAY<<12) | (base_overlay[overlay[baseTile].type] + (overlay[baseTile].frame % 4) * 4);
                else
                    baseTile = (PS_BG_PAL_OVERLAY<<12) | (base_overlay[overlay[baseTile].type] + overlay[baseTile].frame * 4);
                mapBG2[ACTION_BAR_SIZE * 64 + 64*i + 2*j]      = baseTile; // 128 because I'm leaving the top two rows blank (== 32*2*2)
                mapBG2[ACTION_BAR_SIZE * 64 + 64*i + 2*j+1]    = baseTile+1;
                mapBG2[ACTION_BAR_SIZE * 64 + 64*i + 2*j+32]   = baseTile+2;
                mapBG2[ACTION_BAR_SIZE * 64 + 64*i + 2*j+1+32] = baseTile+3;
            }
            mapTile++;
        }
        mapTile += (environment.width - HORIZONTAL_WIDTH);
    }
}

void doOverlayLogic() {
    int i;
    struct Overlay *curOverlay;
    
    startProfilingFunction("doOverlayLogic");
    
    for (i=0, curOverlay=overlay; i<MAX_OVERLAY_ON_MAP; i++, curOverlay++) {
        if (curOverlay->enabled) {
            curOverlay->timer++;
            switch (curOverlay->type) {
                case OT_TRACKS:
                case OT_TRACKS2:
                case OT_GRUBS:
                    if (curOverlay->timer >= OVERLAY_TRACKS_SHORT_DURATION)
                        curOverlay->enabled = 0;
                    break;
                case OT_SMOKE:
                case OT_GRUBS2:
                    if (curOverlay->timer >= OVERLAY_TRACKS_LONG_DURATION)
                        curOverlay->enabled = 0;
                    break;
                case OT_DESTROYED:
                    break;
                case OT_ROCK_SHOT:
                case OT_SAND_SHOT:
                    if (curOverlay->timer >= OVERLAY_SHOT_DURATION) {
                        if (curOverlay->frame > 1) {
                            curOverlay->frame -= 2;
                            curOverlay->timer = 0;
                        } else
                            curOverlay->enabled = 0;
                    }
                    break;
                case OT_DEAD_FOOT:
                    if (curOverlay->timer >= OVERLAY_DEAD_FOOT_DURATION) {
                        if (curOverlay->frame == 0 && (environment.layout[TILE_FROM_XY(curOverlay->x, curOverlay->y)].graphics <= SANDHILL16 || environment.layout[TILE_FROM_XY(curOverlay->x, curOverlay->y)].graphics >= ORE)) {
                            curOverlay->frame++;
                            curOverlay->timer = 0;
                        } else
                            curOverlay->enabled = 0;
                    }
                    break;
                case OT_BLOOD:
                    if (curOverlay->timer >= OVERLAY_BLOOD_DURATION)
                        curOverlay->enabled = 0;
                    break;
                case OT_PERMANENT:
                    break;
            }
            if (!curOverlay->enabled)
                environment.layout[TILE_FROM_XY(curOverlay->x, curOverlay->y)].contains_overlay = -1;
        }
    }
    
    stopProfilingFunction();
}

void loadOverlayGraphicsBG(int baseBg, int *offsetBg) {
    int i;
    char oneline[256];
    char filename[256];
    char *filePosition;
    FILE *fp;
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    // GRAPHICS section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[GRAPHICS]", strlen("[GRAPHICS]")));
    for (i=0; i<2; i++)
        readstr(fp, oneline);
    replaceEOLwithEOF(oneline, 255);
    closeFile(fp);
    
    strcpy(filename, oneline + strlen("Overlay="));
    if (filename[0] != 0)
        strcat(filename, "/");
    filePosition = filename + strlen(filename);
    
    base_overlay[OT_TRACKS] = (*offsetBg)/(8*8);
    strcpy(filePosition, "Tracks");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);
    base_overlay[OT_TRACKS2] = (*offsetBg)/(8*8);
    strcpy(filePosition, "Tracks2");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);
    base_overlay[OT_SMOKE] = (*offsetBg)/(8*8);
    strcpy(filePosition, "Smoke");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);
    base_overlay[OT_GRUBS] = (*offsetBg)/(8*8);
    strcpy(filePosition, "Grubs");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);
    base_overlay[OT_GRUBS2] = (*offsetBg)/(8*8);
    strcpy(filePosition, "Grubs2");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);
    base_overlay[OT_DESTROYED] = (*offsetBg)/(8*8);
    strcpy(filePosition, "Destroyed_Structure");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);
    base_overlay[OT_ROCK_SHOT] = (*offsetBg)/(8*8);
    strcpy(filePosition, "Shot_Rock1");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);
    strcpy(filePosition, "Shot_Rock2");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);
    strcpy(filePosition, "Shot_Rock3");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);
    base_overlay[OT_SAND_SHOT] = (*offsetBg)/(8*8);
    strcpy(filePosition, "Shot_Sand1");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);
    strcpy(filePosition, "Shot_Sand2");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);
    strcpy(filePosition, "Shot_Sand3");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);
    base_overlay[OT_DEAD_FOOT] = (*offsetBg)/(8*8);
    strcpy(filePosition, "Dead_Foot");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);
    base_overlay[OT_BLOOD] = (*offsetBg)/(8*8);
    strcpy(filePosition, "Blood");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);
    base_overlay[OT_PERMANENT] = (*offsetBg)/(8*8);
    for (i=0; i<MAX_PERMANENT_OVERLAY_TYPES; i++) {
        sprintf(filePosition, "Permanent%i", i+1);
        *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS); 
    }
    strcpy(filePosition, "Blood");
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_OVERLAY_GRAPHICS);

}

void loadOverlayGraphicsSprites(int *offsetSp) {
}

void initOverlayWithScenario() {
    FILE *fp;
    char oneline[256];
    int type, x, y;
    int i;
    
    for (i=0; i<environment.width*environment.height; i++)
        environment.layout[i].contains_overlay = -1;
    for (i=0; i<MAX_OVERLAY_ON_MAP; i++)
        overlay[i].enabled = 0;
    
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    // OVERLAY section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[OVERLAY]", strlen("[OVERLAY]")) && oneline[0] != '-');
    if (oneline[0] != '-') {
        while (1) {
            readstr(fp, oneline);
            if (sscanf(oneline, "Permanent%i,%i,%i", &type, &x, &y) != 3)
                break;
            if (type > MAX_PERMANENT_OVERLAY_TYPES)
                error("Too high permanent overlay type found in line", oneline);
            environment.layout[TILE_FROM_XY(x, y)].contains_overlay = MAX_OVERLAY_ON_MAP + (type-1);
        }
    }
    closeFile(fp);
}

void initOverlay() {
}

inline struct Overlay *getOverlay() {
    return overlay;
}

int getOverlaySaveSize(void) {
    return sizeof(overlay);
}

int getOverlaySaveData(void *dest, int max_size) {
    int size=sizeof(overlay);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&overlay,size);
    return (size);
}
