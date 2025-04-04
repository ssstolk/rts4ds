// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_rts4ds.h"

#ifndef DISABLE_MENU_RTS4DS

#include <string.h>
#include <stdio.h>

#include "soundeffects.h"

#include "rts4ds_logo_bin.h"
#include "rts4ds_creator_bin.h"
#include "rts4ds_font_bin.h"
#include "rts4ds_bar_bin.h"


#define MAX_PROJECTS 5
#define MAX_PROJECT_LENGTH 24

char project[MAX_PROJECTS][MAX_PROJECT_LENGTH];
int numberOfProjects;


void drawMenuRts4ds() {
    int i;
    
    if (numberOfProjects<=1)
        return;
    
    initSpritesUsedInfoScreen();
    for (i=0; i<3; i++)
        setSpriteInfoScreen(62, ATTR0_SQUARE,
                            80 + i*32, SPRITE_SIZE_32, 0, 0,
                            0, 0, i*16);
    for (i=0; i<12; i++)
        setSpriteInfoScreen(178, ATTR0_SQUARE,
                            156 + i*8, SPRITE_SIZE_8, 0, 0,
                            0, 0, 48 + i);
    
    initSpritesUsedPlayScreen();
    for (i=0; i<numberOfProjects; i++)
        drawMenuOption(i, project[i], 1);
}

void drawMenuRts4dsBG() {
}

void doMenuRts4dsLogic() {
    if (updateMenuOption(numberOfProjects) || numberOfProjects==1) {
        setCurrentProjectDirname(project[getMenuOption()]);
        setGameState(INTRO);
        playSoundeffect(SE_MENU_OK);
    }
}

void loadMenuRts4dsGraphics(enum GameState oldState) {
    int i, j;
    int offset;
    
    // palette exists out of only 3 entries, so no need to go all fancy and load this in
    BG_PALETTE[0] = RGB15(0, 0, 0);        // black
    SPRITE_PALETTE[0] = RGB15(0, 0, 0);    // black
    SPRITE_PALETTE[1] = RGB15(31, 31, 31); // white
    SPRITE_PALETTE[2] = RGB15(25, 0, 5);   // red

    // load in bar graphics and font graphics, which are #include'ed for this menu only
    for (i=0; i<rts4ds_font_bin_size>>1; i++)
        SPRITE_GFX[i] = ((uint16*)rts4ds_font_bin)[i];
    offset = rts4ds_font_bin_size>>1;
    for (i=0; i<8; i++) {
        for (j=0; j<rts4ds_bar_bin_size>>1; j++) {
            SPRITE_GFX[offset + j] = ((uint16*)rts4ds_bar_bin)[j];
        }
        offset += rts4ds_bar_bin_size>>1;
    }
    
    // palette exists out of only 3 entries, so no need to go all fancy and load this in
    BG_PALETTE_SUB[0]     = RGB15(0, 0, 0);    // black
    SPRITE_PALETTE_SUB[0] = RGB15(0, 0, 0);    // black
    SPRITE_PALETTE_SUB[1] = RGB15(31, 31, 31); // white
    SPRITE_PALETTE_SUB[2] = RGB15(25, 0, 5);   // red
    
    // load in rts4ds menu graphics, which are #include'ed for this menu only
    for (i=0; i<rts4ds_logo_bin_size>>1; i++)
        SPRITE_GFX_SUB[i] = ((uint16*)rts4ds_logo_bin)[i];
    offset = (rts4ds_logo_bin_size>>1);
    for (i=0; i<rts4ds_creator_bin_size>>1; i++)
        SPRITE_GFX_SUB[offset + i] = ((uint16*)rts4ds_creator_bin)[i];
}

int initMenuRts4ds() {
    FILE *fp;
    char oneline[256];
    int i;
    
    fp = openFile("games.txt", FS_RTS4DS_FILE);
    readstr(fp, oneline);
    sscanf(oneline, "AMOUNT-OF-GAMES %i", &numberOfProjects);
    
    if (numberOfProjects <= 0)
        error("One to five RTS projects required.", "Found none specified.");
    if (numberOfProjects > 5)
        numberOfProjects = 5;
    
    for (i=0; i<numberOfProjects; i++) {
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 255);
        strcpy(project[i], oneline);
    }
    closeFile(fp);
    
    REG_DISPCNT = ( MODE_0_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_DISPCNT_SUB = ( MODE_0_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    
    return 0;
}

#endif
