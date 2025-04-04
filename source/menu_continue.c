// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_continue.h"

#include <string.h>

#include "factions.h"
#include "settings.h"
#include "animation.h"
#include "soundeffects.h"
#include "cutscene_introduction.h"


void drawMenuContinue() {
    char optionName[256];
    int amountOfOptions=0;
    int i;
    
    initSpritesUsedInfoScreen();
    initSpritesUsedPlayScreen();
    
    setSpritePlayScreen(SCREEN_HEIGHT - 32, ATTR0_SQUARE, // the "back" sprite
                        0, SPRITE_SIZE_32, 0, 0,
                        0, 0, (8704+4096)/(8*8));
    
    if (getLevelCurrentFactionInfo() != -1 && getLevelCurrentFactionInfo() < 3) {
        if (getLevelCurrent() <= 9) {
            sprintf(optionName, "%s level %i", factionInfo[getLevelCurrentFactionInfo()].name, getLevelCurrent());
            drawMenuOption(amountOfOptions, optionName, 1);
            amountOfOptions++;
        } else if (getLevelCurrentFactionInfo()+1 < 3) {
            sprintf(optionName, "new game with %s", factionInfo[getLevelCurrentFactionInfo() + 1].name);
            drawMenuOption(amountOfOptions, optionName, 1);
            amountOfOptions++;
        }
    }
    for (i=0; i<3 /*MAX_DIFFERENT_FACTIONS*/ && factionInfo[i].enabled; i++) {
        if (getLevelReached(i) > 1 && getLevelReached(i) <= 9 && (i != getLevelCurrentFactionInfo() || getLevelReached(i) > getLevelCurrent())) {
            strcpy(optionName, factionInfo[i].name);
            sprintf(optionName + strlen(factionInfo[i].name), " level %i", getLevelReached(i));
            drawMenuOption(amountOfOptions, optionName, 1);
            amountOfOptions++;
        }
    }
    
    drawAnimation();
}

void drawMenuContinueBG() {
    REG_DISPCNT |= DISPLAY_BG3_ACTIVE;
/*    REG_DISPCNT_SUB |= DISPLAY_BG3_ACTIVE;*/
}

void doMenuContinueLogic() {
    int amountOfOptions=0;
    int i;
    
    if (doAnimationLogic()) // loop the animation please.
        initAnimation("menu", 1, 1, 0);
    
    if (getLevelCurrentFactionInfo() != -1 && getLevelCurrentFactionInfo() < 3) {
        if (getLevelCurrent() <= 9 || getLevelCurrentFactionInfo()+1 < 3)
            amountOfOptions++;
    }
    for (i=0; i<3 /*MAX_DIFFERENT_FACTIONS*/ && factionInfo[i].enabled; i++) {
        if (getLevelReached(i) > 1 && getLevelReached(i) <= 9 && (i != getLevelCurrentFactionInfo() || getLevelReached(i) > getLevelCurrent()))
            amountOfOptions++;
    }
    
    if ((keysDown() & KEY_B) ||
        ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px < 32 && touchReadLast().py >= SCREEN_HEIGHT - 32)) {
        setGameState(MENU_MAIN);
        playSoundeffect(SE_MENU_BACK);
    } else if (updateMenuOption(amountOfOptions)) {
        amountOfOptions = 0;
        if (getMenuOption() == 0 && getLevelCurrentFactionInfo() != -1 && getLevelCurrentFactionInfo() < 3) {
            if (getLevelCurrent() <= 9) {
                setLevelLoadedViaLevelSelect(0);
                setFaction(FRIENDLY, getLevelCurrentFactionInfo());
                setLevel(getLevelCurrent());
                setGameState(MENU_REGIONS);
                playSoundeffect(SE_MENU_OK);
                return;
            } else if (getLevelCurrentFactionInfo()+1 < 3) {
                setLevelLoadedViaLevelSelect(0);
                setFaction(FRIENDLY, getLevelCurrentFactionInfo() + 1);
                setLevel(1);
                initCutsceneIntroductionFilename();
                setGameState(CUTSCENE_INTRODUCTION);
                return;
            }
        } else {
            if (getLevelCurrentFactionInfo() != -1 && getLevelCurrentFactionInfo() < 3 && (getLevelCurrent() <= 9 || getLevelCurrentFactionInfo()+1 < 3))
                amountOfOptions++;
            for (i=0; i<3 /*MAX_DIFFERENT_FACTIONS*/ && factionInfo[i].enabled; i++) {
                if (getLevelReached(i) > 1 && getLevelReached(i) <= 9 && (i != getLevelCurrentFactionInfo() || getLevelReached(i) > getLevelCurrent())) {
                    if (getMenuOption() == amountOfOptions) {
                        setLevelLoadedViaLevelSelect(0);
                        setFaction(FRIENDLY, i);
                        setLevel(getLevelReached(i));
                        setGameState(MENU_REGIONS);
                        playSoundeffect(SE_MENU_OK);
                        return;
                    }
                    amountOfOptions++;
                }
            }
        }
    }
}

void loadMenuContinueGraphics(enum GameState oldState) {
    int offset = 0;
    
    copyFileVRAM(BG_GFX, "menu_continue", FS_MENU_GRAPHICS);
    copyFileVRAM(BG_PALETTE, "menu_continue", FS_PALETTES);
    
    if (oldState != MENU_MAIN) {
/*        copyFileVRAM(BG_PALETTE_SUB, "logo", FS_PALETTES);
        copyFileVRAM(BG_GFX_SUB, "logo", FS_MENU_GRAPHICS);*/
        initAnimation("menu", 1, 1, 0);
        loadAnimationGraphics(oldState);
        drawAnimationBG();
        
        copyFileVRAM(SPRITE_PALETTE, "menu_bits", FS_PALETTES);
        offset += copyFileVRAM(SPRITE_GFX + (offset>>1), "menu_bit_font", FS_MENU_GRAPHICS);
        offset += copyFileVRAM(SPRITE_GFX + (offset>>1), "menu_bit_bar", FS_MENU_GRAPHICS);
        offset += copyFileVRAM(SPRITE_GFX + (offset>>1), "menu_bit_back", FS_MENU_GRAPHICS);
    }
}

int initMenuContinue() {
    int amountOfOptions=0;
    int i;
    
    if (getLevelCurrentFactionInfo() != -1 && getLevelCurrentFactionInfo() < 3) {
        if (getLevelCurrent() <= 9 || getLevelCurrentFactionInfo()+1 < 3)
            amountOfOptions++;
    }
    for (i=0; i<3 /*MAX_DIFFERENT_FACTIONS*/ && factionInfo[i].enabled; i++) {
        if (getLevelReached(i) > 1 && getLevelReached(i) <= 9 && (i != getLevelCurrentFactionInfo() || getLevelReached(i) > getLevelCurrent()))
            amountOfOptions++;
    }
    
    if (amountOfOptions == 1) {
        if (getLevelCurrentFactionInfo() != -1 && getLevelCurrentFactionInfo() < 3) {
            if (getLevelCurrent() <= 9) {
                setLevelLoadedViaLevelSelect(0);
                setFaction(FRIENDLY, getLevelCurrentFactionInfo());
                setLevel(getLevelCurrent());
                setGameState(MENU_REGIONS);
                return 1;
            } else if (getLevelCurrentFactionInfo()+1 < 3) {
                setLevelLoadedViaLevelSelect(0);
                setFaction(FRIENDLY, getLevelCurrentFactionInfo() + 1);
                setLevel(1);
                initCutsceneIntroductionFilename();
                setGameState(CUTSCENE_INTRODUCTION);
                return 1;
            }
        }
        for (i=0; i<3 /*MAX_DIFFERENT_FACTIONS*/ && factionInfo[i].enabled; i++) {
            if (getLevelReached(i) > 1 && getLevelReached(i) <= 9 && (i != getLevelCurrentFactionInfo() || getLevelReached(i) > getLevelCurrent())) {
                setLevelLoadedViaLevelSelect(0);
                setFaction(FRIENDLY, i);
                setLevel(getLevelReached(i));
                setGameState(MENU_REGIONS);
                return 1;
            }
        }
    }
    
    REG_DISPCNT = ( MODE_3_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT = BG_BMP8_256x256;
/*  REG_BG3PA = 1<<8; // scaling of 1
    REG_BG3PB = 0;
    REG_BG3PC = 0;
    REG_BG3PD = 1<<8; // scaling of 1
    REG_BG3X = 0;
    REG_BG3Y = 0;*/
    
/*  REG_DISPCNT_SUB = ( MODE_3_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT_SUB = BG_BMP8_256x256;
    REG_BG3PA_SUB = 1<<8; // scaling of 1
    REG_BG3PB_SUB = 0;
    REG_BG3PC_SUB = 0;
    REG_BG3PD_SUB = 1<<8; // scaling of 1
    REG_BG3X_SUB = 0;
    REG_BG3Y_SUB = 0;*/
    
    playMiscSoundeffect("menu_continue"); // startup sounds for non-GIF screens
    
    return 0;
}
