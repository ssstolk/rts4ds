// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_rts4ds.h"

#ifdef DISABLE_MENU_RTS4DS

#include <string.h>
#include <stdio.h>

#include "factions.h"
#include "environment.h"
#include "overlay.h"
#include "explosions.h"
#include "projectiles.h"
#include "structures.h"
#include "units.h"
#include "ai.h"
#include "soundeffects.h"
#include "subtitles.h"
#include "settings.h"
#include "playscreen.h"
#include "pathfinding.h"

#define MAX_PROJECT_LENGTH 24
#define SPLASH_FADE_DURATION (SCREEN_REFRESH_RATE / 2)
#define SPLASH_FADE_IN_TYPE     1 /* 2 is black, 1 is white  */
#define SPLASH_FADE_OUT_TYPE    2 /* 2 is black, 1 is white  */

char project[MAX_PROJECT_LENGTH];
int splashTimer;


void drawSplash() {
    if (splashTimer > 0) {
        REG_MASTER_BRIGHT_SUB = (SPLASH_FADE_IN_TYPE<<14) | ((splashTimer*16) / SPLASH_FADE_DURATION);
        REG_MASTER_BRIGHT     = (SPLASH_FADE_IN_TYPE<<14) | ((splashTimer*16) / SPLASH_FADE_DURATION);
    } else if (splashTimer < 0) {
        REG_MASTER_BRIGHT_SUB = (SPLASH_FADE_OUT_TYPE<<14) | (((-splashTimer)*16) / SPLASH_FADE_DURATION);
        REG_MASTER_BRIGHT     = (SPLASH_FADE_OUT_TYPE<<14) | (((-splashTimer)*16) / SPLASH_FADE_DURATION);
    }
}

void drawSplashBG() {
    REG_DISPCNT_SUB |= DISPLAY_BG3_ACTIVE;
    REG_DISPCNT     |= DISPLAY_BG3_ACTIVE;
}

void doSplashLogic() {
    if (--splashTimer < -SPLASH_FADE_DURATION) {
        setGameState(INTRO);
        return;
    }
    
    if (splashTimer == 0) {
        REG_MASTER_BRIGHT_SUB = 0;
        REG_MASTER_BRIGHT     = 0;
        
        initFactions();
        initEnvironment();
        initOverlay();
        initExplosions();
        initProjectiles();
        initStructures();
        initUnits();
        initPriorityStructureAI();
        initSoundeffects();
        initSubtitles();
        #ifndef REMOVE_ASTAR_PATHFINDING
        initPathfinding();
        #endif
        readSaveFile();
        preloadPlayScreen3DGraphics();
    }
}

void loadSplashGraphics(enum GameState oldState) {
    int i;
    copyFileVRAM(BG_GFX_SUB, "splash_top.bin",    FS_RTS4DS_FILE);
    copyFileVRAM(BG_GFX,     "splash_bottom.bin", FS_RTS4DS_FILE);
    for (i=0; i<256*192; i++) {
        BG_GFX_SUB[i] |= BIT(15);
        BG_GFX[i]     |= BIT(15);
    }
}

int initSplash() {
    FILE *fp;
    char oneline[256];
    
    REG_MASTER_BRIGHT_SUB = (SPLASH_FADE_IN_TYPE<<14) | 16;
    REG_MASTER_BRIGHT     = (SPLASH_FADE_IN_TYPE<<14) | 16;
    
    fp = openFile("games.txt", FS_RTS4DS_FILE);
    readstr(fp, oneline); // AMOUNT-OF-GAMES line
    readstr(fp, oneline);
    closeFile(fp);
    replaceEOLwithEOF(oneline, 256);
    strncpy(project, oneline, MAX_PROJECT_LENGTH);
    project[MAX_PROJECT_LENGTH-1] = 0;
    setCurrentProjectDirname(project);
    
    splashTimer = SPLASH_FADE_DURATION;
    
    BG_PALETTE_SUB[0] = BIT(15) | RGB15(0, 31, 0);
    
    REG_DISPCNT_SUB = ( MODE_3_2D );
    REG_DISPCNT     = ( MODE_3_2D );
    
    REG_BG3CNT_SUB = BG_BMP16_256x256;
    REG_BG3CNT     = BG_BMP16_256x256;
    
    REG_BG3PA_SUB = 1<<8; // scaling of 1
    REG_BG3PB_SUB = 0;
    REG_BG3PC_SUB = 0;
    REG_BG3PD_SUB = 1<<8; // scaling of 1
    REG_BG3X_SUB = 0;
    REG_BG3Y_SUB = 0;
    
    REG_BG3PA = 1<<8; // scaling of 1
    REG_BG3PB = 0;
    REG_BG3PC = 0;
    REG_BG3PD = 1<<8; // scaling of 1
    REG_BG3X = 0;
    REG_BG3Y = 0;
    
    return 0;
}

#endif
