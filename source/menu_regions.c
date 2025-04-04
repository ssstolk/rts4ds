// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_regions.h"

#include <stdlib.h>
#include <string.h>

#include "gif/gif.h"
#include "factions.h"
#include "subtitles.h"
#include "soundeffects.h"
#include "shared.h"
#include "cutscene_briefing.h"

#define MAX_NUMBER_OF_REGIONS       50
#define MAX_NUMBER_OF_NEXT_REGIONS  30

#define MRS_DISPLAYING_CONQUERED_DURATION   (SCREEN_REFRESH_RATE + SCREEN_REFRESH_RATE/2)
#define MRS_DISPLAYING_NEXT_DURATION        (SCREEN_REFRESH_RATE + SCREEN_REFRESH_RATE/2)

#define MENU_REGIONS_ARROW_ANIMATION_DURATION         (SCREEN_REFRESH_RATE/2)
#define MENU_REGIONS_ARROW_CHANGE_SELECTION_INTERVAL  (SCREEN_REFRESH_RATE/5)

struct Region {
    int current_faction;
    int next_faction;
};
struct Region menuRegionsRegion[MAX_NUMBER_OF_REGIONS];
int nextFaction[MAX_DIFFERENT_FACTIONS];
int amountOfNextFactionsToDraw;

struct NextRegion {
    int enabled;
    int region;
    enum Positioned arrow_positioned;
    int arrow_x;
    int arrow_y;
};
struct NextRegion menuRegionsNextRegion[MAX_NUMBER_OF_NEXT_REGIONS];

enum MenuRegionsState { MRS_DISPLAYING_CONQUERED, MRS_IDLE, MRS_DISPLAYING_NEXT };
enum MenuRegionsState menuRegionsState;
int menuRegionsTimer;
int menuRegionsArrowTimer;
int menuRegionsPictureX, menuRegionsPictureY;
int menuRegionsPictureTimer;
uint16 menuRegionsNextColour;
pGifHandler menuRegionsPictureAni;
char menuRegionsPictureAniFilename[256];
int menuRegionNextRegionSelected;

u8 *menuRegionsOriginal;
u8 *menuRegionsPieces;
u8 *menuRegionsBriefingOriginal;



inline uint16 averageColourOf(uint16 c1, uint16 c2) {
    return ((((c1 & 0x1F) + (c2 & 0x1F)) / 2) & 0x1F) |
            ((((c1 & 0x3E0) + (c2 & 0x3E0)) / 2) & 0x3E0) |
            ((((c1 & 0x7C00) + (c2 & 0x7C00)) / 2) & 0x7C00);
}


void drawMenuRegions() {
    int nextFactionToDraw = 0;
    int newColour;
    int regionToDraw = getRegion();
    int i, j, k;
//    int ownerFaction;
    uint16 l;
    
    initSpritesUsedInfoScreen();
    initSpritesUsedPlayScreen();
    
    setSpritePlayScreen(SCREEN_HEIGHT - 32, ATTR0_SQUARE, // the "back" sprite
                        0, SPRITE_SIZE_32, 0, 0,
                        0, 0, 0);
    
    if (menuRegionsState == MRS_DISPLAYING_CONQUERED) {
        
        i = menuRegionsTimer / MRS_DISPLAYING_CONQUERED_DURATION;
        if (i < amountOfNextFactionsToDraw) { // New: Faction
            nextFactionToDraw = nextFaction[i];
            if ((menuRegionsTimer % MRS_DISPLAYING_CONQUERED_DURATION) % (MRS_DISPLAYING_CONQUERED_DURATION / 5) == 0) {
                newColour = factionInfo[nextFactionToDraw].colour;
                for (i= (menuRegionsTimer % MRS_DISPLAYING_CONQUERED_DURATION) / (MRS_DISPLAYING_CONQUERED_DURATION / 5); i<256*192; i+=5) {
                    if (menuRegionsPieces[i] > 0 && menuRegionsRegion[menuRegionsPieces[i] - 1].next_faction == nextFactionToDraw)
                        BG_GFX[i] = BIT(15) | averageColourOf(BG_PALETTE[menuRegionsOriginal[i]], newColour);
                }
            }
        } else { // Next regions
            if ((menuRegionsTimer % MRS_DISPLAYING_CONQUERED_DURATION) % (MRS_DISPLAYING_CONQUERED_DURATION / 5) == 0) {
                for (i= (menuRegionsTimer % MRS_DISPLAYING_CONQUERED_DURATION) / (MRS_DISPLAYING_CONQUERED_DURATION / 5); i<256*192; i+=5) {
                    if (menuRegionsPieces[i] > 0) {
                        for (j=0; j<MAX_NUMBER_OF_NEXT_REGIONS && menuRegionsNextRegion[j].enabled; j++) {
                            if (menuRegionsNextRegion[j].region == menuRegionsPieces[i]) {
//                                ownerFaction = menuRegionsRegion[menuRegionsPieces[i] - 1].next_faction;
//                                if (ownerFaction == -1)
//                                    ownerFaction = menuRegionsRegion[menuRegionsPieces[i] - 1].current_faction;
//                                if (ownerFaction >= 0)
//                                    BG_GFX[i] = BIT(15) | averageColourOf(averageColourOf(BG_PALETTE[menuRegionsOriginal[i]], factionInfo[ownerFaction].colour), menuRegionsNextColour);
//                                else
                                    BG_GFX[i] = BIT(15) | averageColourOf(BG_PALETTE[menuRegionsOriginal[i]], menuRegionsNextColour);
                                break;
                            }
                        }
                    }
                }
            }
        }
        
    } else {
        // draw arrows
        j = 0;
        for (i=0; i<MAX_NUMBER_OF_NEXT_REGIONS && menuRegionsNextRegion[i].enabled; i++) {
            setSpritePlayScreen(menuRegionsNextRegion[i].arrow_y, ATTR0_SQUARE,
                                menuRegionsNextRegion[i].arrow_x, SPRITE_SIZE_16, 0, 0,
                                0, 0, 32 + (menuRegionsNextRegion[i].arrow_positioned*2 + menuRegionsArrowTimer >= MENU_REGIONS_ARROW_ANIMATION_DURATION/2) * 4);
            if (j == menuRegionNextRegionSelected)
                setSpritePlayScreen(menuRegionsNextRegion[i].arrow_y - 8, ATTR0_SQUARE,
                                    menuRegionsNextRegion[i].arrow_x - 8, SPRITE_SIZE_32, 0, 0,
                                    0, 0, 16);
            j++;
        }
        
        if (menuRegionsState == MRS_DISPLAYING_NEXT) {
            if (menuRegionsTimer == 0 || menuRegionsTimer == MRS_DISPLAYING_NEXT_DURATION/2) {
                // colour the selected region
                for (i=0; i<256*192; i++) {
                    newColour = factionInfo[getFaction(FRIENDLY)].colour;
                    if (menuRegionsPieces[i] == regionToDraw)
                        BG_GFX[i] = BIT(15) | averageColourOf(BG_PALETTE[menuRegionsOriginal[i]], newColour);
                }
            } else if (menuRegionsTimer == MRS_DISPLAYING_NEXT_DURATION/4 || menuRegionsTimer == (MRS_DISPLAYING_NEXT_DURATION/4)*3) {
                // uncolour the selected region to Next colour
                for (i=0; i<256*192; i++) {
                    if (menuRegionsPieces[i] == regionToDraw)
                        BG_GFX[i] = BIT(15) | averageColourOf(BG_PALETTE[menuRegionsOriginal[i]], menuRegionsNextColour);
                }
                /*// uncolour the selected region to who owned it
                newColour = -1;
                if (menuRegionsRegion[getRegion() - 1].next_faction != -1)
                    newColour = factionInfo[menuRegionsRegion[getRegion() - 1].next_faction].colour;
                else if (menuRegionsRegion[getRegion() - 1].current_faction != -1)
                    newColour = factionInfo[menuRegionsRegion[getRegion() - 1].current_faction].colour;
                
                if (newColour == -1) {
                    for (i=0; i<256*192; i++) {
                        if (menuRegionsPieces[i] == regionToDraw)
                            BG_GFX[i] = BIT(15) | BG_PALETTE[menuRegionsOriginal[i]];
                    }
                } else {
                    for (i=0; i<256*192; i++) {
                        if (menuRegionsPieces[i] == regionToDraw)
                            BG_GFX[i] = BIT(15) | averageColourOf(BG_PALETTE[menuRegionsOriginal[i]], newColour);
                    }
                }*/
            }
        }
    }
    if (menuRegionsPictureAni != NULL && menuRegionsPictureTimer == 0) {
        for (i=0; i<menuRegionsPictureAni->dimY; i++) {
            for (j=0; j<menuRegionsPictureAni->dimX; j++) {
                k = (menuRegionsPictureY+i)*256 + menuRegionsPictureX+j;
                if (menuRegionsBriefingOriginal[k] == 0) {
                    l = menuRegionsPictureAni->gifPalette[menuRegionsPictureAni->gifFrame[i*menuRegionsPictureAni->dimX + j]];
//                    if (l) // make sure transparency isn't drawn
                        BG_GFX_SUB[k] = BIT(15) | l;
                }
            }
        }
    }
    
    drawSubtitles();
}

void drawMenuRegionsBG() {
    int ownerFaction;
    int i;
    
    for (i=0; i<256*192; i++) {
        if (menuRegionsPieces[i] == 0)
            BG_GFX[i] = BIT(15) | BG_PALETTE[menuRegionsOriginal[i]];
        else {
            ownerFaction = menuRegionsRegion[menuRegionsPieces[i] - 1].current_faction;
            if (ownerFaction == -1)
                BG_GFX[i] = BIT(15) | BG_PALETTE[menuRegionsOriginal[i]];
            else
                BG_GFX[i] = BIT(15) | averageColourOf(BG_PALETTE[menuRegionsOriginal[i]], factionInfo[ownerFaction].colour);
        }
        
        BG_GFX_SUB[i] = BIT(15) | BG_PALETTE_SUB[menuRegionsBriefingOriginal[i]];
    }

    REG_DISPCNT |= DISPLAY_BG3_ACTIVE;
    REG_DISPCNT_SUB |= DISPLAY_BG3_ACTIVE;
}

void resetSubBG() {
    int i;
    for (i=0; i<256*192; i++) {
        BG_GFX_SUB[i]=BIT(15)|0;
    }
}

void doMenuRegionsLogic() {
    u8 paletteEntry;
    u8 curPaletteEntry;
    int ownerFaction;
    int i, j;
    
    switch (menuRegionsState) {
        case MRS_DISPLAYING_CONQUERED:
            if ((keysDown() & KEY_A) || (getKeysOnUp() & KEY_TOUCH)) {
                // just display everything as it will be
                for (i=0; i<256*192; i++) {
                    if (menuRegionsPieces[i] != 0) {
                        for (j=0; j<MAX_NUMBER_OF_NEXT_REGIONS && menuRegionsNextRegion[j].enabled; j++) {
                            if (menuRegionsNextRegion[j].region == menuRegionsPieces[i]) {
//                                ownerFaction = menuRegionsRegion[menuRegionsPieces[i] - 1].next_faction;
//                                if (ownerFaction == -1)
//                                    ownerFaction = menuRegionsRegion[menuRegionsPieces[i] - 1].current_faction;
//                                if (ownerFaction >= 0)
//                                    BG_GFX[i] = BIT(15) | averageColourOf(averageColourOf(BG_PALETTE[menuRegionsOriginal[i]], factionInfo[ownerFaction].colour), menuRegionsNextColour);
//                                else
                                    BG_GFX[i] = BIT(15) | averageColourOf(BG_PALETTE[menuRegionsOriginal[i]], menuRegionsNextColour);
                                break;
                            }
                        }
                        if (j >= MAX_NUMBER_OF_NEXT_REGIONS || !menuRegionsNextRegion[j].enabled) {
                            ownerFaction = menuRegionsRegion[menuRegionsPieces[i] - 1].next_faction;
                            if (ownerFaction != -1)
                                BG_GFX[i] = BIT(15) | averageColourOf(BG_PALETTE[menuRegionsOriginal[i]], factionInfo[ownerFaction].colour);
                        }
                    }
                }
                menuRegionsTimer = 0;
                menuRegionsState = MRS_IDLE;
                playSoundeffect(SE_MENU_OK);
            } else {
                menuRegionsTimer++;
                if (menuRegionsTimer >= (amountOfNextFactionsToDraw + 1) * MRS_DISPLAYING_CONQUERED_DURATION) {
                    menuRegionsTimer = 0;
                    menuRegionsState = MRS_IDLE;
                }
            }
            break;
        case MRS_IDLE:
            curPaletteEntry = 0;
            j = 0;
            for (i=0; i<MAX_NUMBER_OF_NEXT_REGIONS && menuRegionsNextRegion[i].enabled; i++) {
                if (j == menuRegionNextRegionSelected)
                    curPaletteEntry = menuRegionsPieces[(menuRegionsNextRegion[i].arrow_y + 8) * 256 + (menuRegionsNextRegion[i].arrow_x + 8)];
                j++;
            }
            menuRegionsArrowTimer++;
            if (menuRegionsArrowTimer >= MENU_REGIONS_ARROW_ANIMATION_DURATION)
                menuRegionsArrowTimer = 0;
            if (getKeysOnUp() & KEY_TOUCH) {
                paletteEntry = menuRegionsPieces[touchReadLast().py * 256 + touchReadLast().px];
                if (paletteEntry != 0) { // a region was selected
                    j = 0;
                    for (i=0; i<MAX_NUMBER_OF_NEXT_REGIONS && menuRegionsNextRegion[i].enabled; i++) {
                        if (menuRegionsNextRegion[i].region == paletteEntry) {
                            if (j == menuRegionNextRegionSelected) {
                                setRegion(paletteEntry);
                                menuRegionsTimer = 0;
                                menuRegionsState = MRS_DISPLAYING_NEXT;
                                playSoundeffect(SE_MENU_REGION);
                            } else {
                                menuRegionNextRegionSelected = j;
                                playSoundeffect(SE_MENU_SELECT);
                            }
                            break;
                        }
                        j++;
                    }
                }
            } else if (keysDown() & KEY_A) {
                setRegion(curPaletteEntry);
                menuRegionsTimer = 0;
                menuRegionsState = MRS_DISPLAYING_NEXT;
                playSoundeffect(SE_MENU_REGION);
            } else if ((keysDown() & (KEY_UP | KEY_LEFT)) || ((keysHeld() & (KEY_UP | KEY_LEFT)) && (getKeysHeldTimeMax(KEY_UP | KEY_LEFT) % MENU_REGIONS_ARROW_CHANGE_SELECTION_INTERVAL == 0))) {
                if (menuRegionNextRegionSelected > 0) {
                    menuRegionNextRegionSelected--;
                    playSoundeffect(SE_MENU_SELECT);
                }
            } else if ((keysDown() & (KEY_DOWN | KEY_RIGHT)) || ((keysHeld() & (KEY_DOWN | KEY_RIGHT)) && (getKeysHeldTimeMax(KEY_DOWN | KEY_RIGHT) % MENU_REGIONS_ARROW_CHANGE_SELECTION_INTERVAL == 0))) {
                if (menuRegionNextRegionSelected < (j-1)) {
                    menuRegionNextRegionSelected++;
                    playSoundeffect(SE_MENU_SELECT);
                }
            }
            break;
        case MRS_DISPLAYING_NEXT:
            menuRegionsArrowTimer++;
            if (menuRegionsArrowTimer >= MENU_REGIONS_ARROW_ANIMATION_DURATION)
                menuRegionsArrowTimer = 0;
            menuRegionsTimer++;
            if (menuRegionsTimer > MRS_DISPLAYING_NEXT_DURATION) {
                free(menuRegionsOriginal);
                free(menuRegionsPieces);
                free(menuRegionsBriefingOriginal);
                if (menuRegionsPictureAni != NULL) {
                    gifHandlerDestroy(menuRegionsPictureAni);
                    menuRegionsPictureAni = NULL;
                }
                initCutsceneBriefingFilename();
                resetSubBG();
                setGameState(CUTSCENE_BRIEFING);
            }
            break;
    }
    if (menuRegionsPictureAni != NULL) {
        menuRegionsPictureTimer++;
        if (menuRegionsPictureTimer >= menuRegionsPictureAni->gifTiming / 15) {
            i = menuRegionsPictureAni->frmCount;
            if (i < gifHandlerLoadNextFrame(menuRegionsPictureAni))
                menuRegionsPictureTimer = 0;
            else {
                if (i <= 1) {
                    // it was a still frame, no need to reload. that would only crash RTS4DS.
                    gifHandlerDestroy(menuRegionsPictureAni);
                    menuRegionsPictureAni = NULL;
                } else
                    menuRegionsPictureAni = gifHandlerLoad(openFile(menuRegionsPictureAniFilename, FS_BRIEFINGPICTURES_GRAPHICS), menuRegionsPictureAni);
            }
        }
    }
    if ((keysDown() & KEY_B) ||
        ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px < 32 && touchReadLast().py >= SCREEN_HEIGHT - 32)) {
        free(menuRegionsOriginal);
        free(menuRegionsPieces);
        free(menuRegionsBriefingOriginal);
        if (menuRegionsPictureAni != NULL) {
            gifHandlerDestroy(menuRegionsPictureAni);
            menuRegionsPictureAni = NULL;
        }
        setGameState(isLevelLoadedViaLevelSelect() ? MENU_LEVELSELECT : MENU_MAIN);
        playSoundeffect(SE_MENU_BACK);
    }
#ifdef DEBUG_BUILD
// TEMPORARY FOR LEVEL HACK
if (keysDown() & KEY_R) {
    free(menuRegionsOriginal);
    free(menuRegionsPieces);
    free(menuRegionsBriefingOriginal);
    if (menuRegionsPictureAni != NULL) {
        gifHandlerDestroy(menuRegionsPictureAni);
        menuRegionsPictureAni = NULL;
    }
    setLevel(getLevel() + 1);
    setGameState(MENU_REGIONS);
    playSoundeffect(SE_MENU_OK);
}
#endif
}


void loadMenuRegionsGraphics(enum GameState oldState) {
    char filepath[256];
    
    strcpy(filepath, "briefing_");
    strcat(filepath, factionInfo[getFaction(FRIENDLY)].name);

    copyFileVRAM(BG_PALETTE_SUB, filepath, FS_PALETTES);
    menuRegionsBriefingOriginal = (u8*) malloc(256 * 192);
    copyFile(menuRegionsBriefingOriginal, filepath, FS_MENU_GRAPHICS);
    
    copyFileVRAM(BG_PALETTE, "regions", FS_PALETTES);
    copyFileVRAM(SPRITE_PALETTE, "regions", FS_PALETTES);
    
    menuRegionsOriginal = (u8*) malloc(256 * 192);
    copyFile(menuRegionsOriginal, "regions", FS_MENU_GRAPHICS);
    
    menuRegionsPieces = (u8*) malloc(256 * 192);
    copyFile(menuRegionsPieces, "regions_pieces", FS_MENU_GRAPHICS);
    
    copyFileVRAM(SPRITE_GFX, "regions_back", FS_MENU_GRAPHICS);
    copyFileVRAM(SPRITE_GFX + ((32*32) >> 1), "regions_bit_select", FS_MENU_GRAPHICS);
    copyFileVRAM(SPRITE_GFX + ((32*32+32*32) >> 1), "arrows", FS_MENU_GRAPHICS);
    
    loadSubtitlesGraphics();
}

int initMenuRegions() {
    char oneline[256];
    char *charPosition;
    int i, j, k, l;
    int amountOfRegions;
    int amountOfLevels;
    FILE *fp;
    
    // check if all levels were played already; if so, switch to OUTTRO
    strcpy(oneline, "levels_");
    strcat(oneline, factionInfo[getFaction(FRIENDLY)].name);
    fp = openFile(oneline, FS_SCENARIO_FILE);

    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[GENERAL]", strlen("[GENERAL]")));
    readstr(fp, oneline);
    sscanf(oneline, "Levels = %i", &amountOfLevels);
    
    closeFile(fp);
    if (getLevel() > amountOfLevels) {
        setGameState(OUTTRO);
        return 1;
    }

    // there's a next level, so let's continue
    menuRegionsState = MRS_DISPLAYING_CONQUERED;
    menuRegionsTimer = 0;
    menuRegionsArrowTimer = 0;
    menuRegionsPictureAni = NULL;
    menuRegionsPictureTimer = 0;
    menuRegionNextRegionSelected = 0;
    
    for (i=0; i<MAX_NUMBER_OF_REGIONS; i++) {
        menuRegionsRegion[i].current_faction = -1;
        menuRegionsRegion[i].next_faction = -1;
    }
    for (i=0; i<MAX_NUMBER_OF_NEXT_REGIONS; i++)
        menuRegionsNextRegion[i].enabled = 0;
    
    strcpy(oneline, "levels_");
    strcat(oneline, factionInfo[getFaction(FRIENDLY)].name);
    fp = openFile(oneline, FS_SCENARIO_FILE);
    
    // GENERAL section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[GENERAL]", strlen("[GENERAL]")));
    readstr(fp, oneline);
    readstr(fp, oneline);
    // PictureXY here
    sscanf(oneline, "PictureXY = %i, %i", &menuRegionsPictureX, &menuRegionsPictureY);
    
    // COLOURS section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[COLOURS]", strlen("[COLOURS]")));
    readstr(fp, oneline);
    // Next here
    sscanf(oneline, "Next = %i,%i,%i", &i, &j, &k);
    menuRegionsNextColour = RGB15(i, j, k);
    
    
    for (i=0; i<=getLevel()-1; i++) {
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[LEVEL", strlen("[LEVEL")));
    }
    
    readstr(fp, oneline);
    while (!strncmp(oneline, "Old: ", strlen("Old: "))) {
        // this line should have a name of a faction followed by which regions it just conquered
        for (j=0; j<MAX_DIFFERENT_FACTIONS && factionInfo[j].enabled; j++) {
            if (!strncmp(oneline + strlen("Old: "), factionInfo[j].name, strlen(factionInfo[j].name))) {
                charPosition = strchr(oneline, '=');
                if (charPosition != 0) {
                    sscanf(charPosition, "= %i", &amountOfRegions);
                    if (amountOfRegions > 0) {
                        charPosition = strchr(charPosition, ':');
                        if (charPosition != 0) {
                            for (k=0; k<amountOfRegions; k++) {
                                charPosition = strchr(charPosition, ' ');
                                sscanf(charPosition, " %i", &l);
                                menuRegionsRegion[l - 1].current_faction = j; // set current faction
                                charPosition++; // make sure we move past the space
                            }
                        }
                    }
                }
                break;
            }
        }
        readstr(fp, oneline);
    }
    
    amountOfNextFactionsToDraw = 0;
    
    while (!strncmp(oneline, "New: ", strlen("New: "))) {
        // this line should have a name of a faction followed by which regions it just conquered
        for (j=0; j<MAX_DIFFERENT_FACTIONS && factionInfo[j].enabled; j++) {
            if (!strncmp(oneline + strlen("New: "), factionInfo[j].name, strlen(factionInfo[j].name))) {
                charPosition = strchr(oneline, '=');
                if (charPosition != 0) {
                    sscanf(charPosition, "= %i", &amountOfRegions);
                    if (amountOfRegions > 0) {
                        charPosition = strchr(charPosition, ':');
                        if (charPosition != 0) {
                            for (k=0; k<amountOfRegions; k++) {
                                charPosition = strchr(charPosition, ' ');
                                sscanf(charPosition, " %i", &l);
                                menuRegionsRegion[l - 1].next_faction = j; // set next faction
                                charPosition++; // make sure we move past the space
                            }
                        }
                        nextFaction[amountOfNextFactionsToDraw] = j;
                        amountOfNextFactionsToDraw++;
                    }
                }
                break;
            }
        }
        readstr(fp, oneline);
    }

    // "Next" here.
    charPosition = strchr(oneline, '=');
    if (charPosition != 0) {
        sscanf(charPosition, "= %i", &amountOfRegions);
        charPosition = strchr(charPosition, ':');
        if (charPosition != 0) {
            for (j=0; j<amountOfRegions; j++) {
                charPosition = strchr(charPosition, ' ');
                sscanf(charPosition, " %i", &k);
                menuRegionsNextRegion[j].region = k;
                charPosition++; // make sure we move past the space
            }
            readstr(fp, oneline);
            for (j=0; j<amountOfRegions; j++) {
                charPosition = strchr(oneline, '=');
                if (charPosition != 0) {
                    sscanf(charPosition, "= %i, %i, %i", &k, &menuRegionsNextRegion[j].arrow_x, &menuRegionsNextRegion[j].arrow_y);
                    menuRegionsNextRegion[j].arrow_positioned = k;
                    menuRegionsNextRegion[j].enabled = 1;
                }
                readstr(fp, oneline);
            }
        }
    }
    // "Description" here
    if (i == getLevel()) {
        charPosition = strchr(oneline, '=');
        if (charPosition != 0) {
            replaceEOLwithEOF(oneline, 255);
            setSubtitles(charPosition + 2);
        }
    }
    readstr(fp, oneline);
    // Picture name here
    replaceEOLwithEOF(oneline, 255);
    charPosition = strchr(oneline, '=');
    if (strncmp(charPosition + 2, "None", strlen("None")) && strncmp(charPosition + 2, "none", strlen("none"))) {
        strcpy(menuRegionsPictureAniFilename, charPosition + 2);
        menuRegionsPictureAni = gifHandlerLoad(openFile(menuRegionsPictureAniFilename, FS_BRIEFINGPICTURES_GRAPHICS), NULL);
    }
    
    closeFile(fp);
    
    REG_DISPCNT = ( MODE_5_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT = BG_BMP16_256x256;
/*  REG_BG3PA = 1<<8; // scaling of 1
    REG_BG3PB = 0;
    REG_BG3PC = 0;
    REG_BG3PD = 1<<8; // scaling of 1
    REG_BG3X  = 0;
    REG_BG3Y  = 0;*/
    
    REG_DISPCNT_SUB = ( MODE_5_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT_SUB = BG_BMP16_256x256;
/*  REG_BG3PA_SUB = 1<<8; // scaling of 1
    REG_BG3PB_SUB = 0;
    REG_BG3PC_SUB = 0;
    REG_BG3PD_SUB = 1<<8; // scaling of 1
    REG_BG3X_SUB  = 0;
    REG_BG3Y_SUB  = 0;*/   
    
    playMiscSoundeffect("menu_regions"); // startup sounds for non-GIF screens
    
    return 0;
}
