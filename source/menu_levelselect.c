// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_levelselect.h"

#include <string.h>

#include "factions.h"
#include "settings.h"
#include "savegame.h"
#include "animation.h"
#include "soundeffects.h"


enum LevelSelectState { LSS_NORMAL, LSS_CLEAR_SAVEFILE };
enum LevelSelectState levelSelectState;

void drawMenuLevelSelect() {
    int i, j;
    
    initSpritesUsedInfoScreen();
    initSpritesUsedPlayScreen();
    
    if (levelSelectState == LSS_NORMAL) {
        setSpritePlayScreen(SCREEN_HEIGHT - 32, ATTR0_SQUARE, // the "back" sprite
                            0, SPRITE_SIZE_32, 0, 0,
                            0, 0, (8704+4096)/(8*8));

        for (i=0; i<3; i++) { // faction
            for (j=0; j<9; j++) { // level
                setSpritePlayScreen(70+36*i, ATTR0_SQUARE,              // the "medal" sprite
                                    50+22*j, SPRITE_SIZE_16, 0, 0,
                                    0, 0, ((8704+4096+32*32+1024) + getMedalReceived(i, 1+j)*16*16)/(8*8));
            }
        }
    }
    
    drawAnimation();
}

void drawMenuLevelSelectBG() {
    REG_DISPCNT |= DISPLAY_BG3_ACTIVE;
/*    REG_DISPCNT_SUB |= DISPLAY_BG3_ACTIVE;*/
}

void doMenuLevelSelectLogic() {
    int slot;
    
    if (levelSelectState == LSS_CLEAR_SAVEFILE) {
        if ((keysDown() & KEY_B) ||
            ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px >= 133 && touchReadLast().px <= 172 && touchReadLast().py >= 137 && touchReadLast().py <= 150)) {
            setGameState(MENU_LEVELSELECT);
            playSoundeffect(SE_MENU_BACK);
            return;
        }
        if ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px >= 85 && touchReadLast().px <= 124 && touchReadLast().py >= 137 && touchReadLast().py <= 150) {
            for (slot=0; slot<3; slot++)
                clearSaveGame(slot);
            clearProgress();
            playSoundeffect(SE_MENU_OK);
        }    
        return;
    }
    
    if (doAnimationLogic()) // loop the animation please.
        initAnimation("menu", 1, 1, 0);
    
    if ((keysDown() & KEY_B) ||
        ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px < 32 && touchReadLast().py >= SCREEN_HEIGHT - 32)) {
        setGameState(MENU_MAIN);
        playSoundeffect(SE_MENU_BACK);
    } else {
        if ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px > 48 && touchReadLast().px < 246) {
            slot = (touchReadLast().px - 48) / 22;
            
            if (touchReadLast().py >= 63 && touchReadLast().py < 63+28) {
                setFaction(FRIENDLY, 0);
            } else if (touchReadLast().py >= 99 && touchReadLast().py < 99+28) {
                slot += 1*9;
                setFaction(FRIENDLY, 1);
            } else if (touchReadLast().py >= 135 && touchReadLast().py < 135+28) {
                slot += 2*9;
                setFaction(FRIENDLY, 2);
            } else
                slot = -1;
            
            if (slot >= 0) {
                setLevelLoadedViaLevelSelect(1);
                setLevel(1 + (slot % 9));
                setGameState(MENU_REGIONS);
                playSoundeffect(SE_MENU_OK);
                return;
            }
        }
        
        if ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px >= 139 && touchReadLast().px <= 213 && touchReadLast().py >= 171 && touchReadLast().py <= 188) {
            swiWaitForVBlank();
            REG_MASTER_BRIGHT = (2<<14) | 16; // 2 for downing brightness from original
            levelSelectState = LSS_CLEAR_SAVEFILE;
            copyFileVRAM(BG_GFX, "menu_clearsavefile", FS_MENU_GRAPHICS);
            copyFileVRAM(BG_PALETTE, "menu_clearsavefile", FS_PALETTES);
            swiWaitForVBlank();
            REG_MASTER_BRIGHT = 0;
        }
    }
}

void loadMenuLevelSelectGraphics(enum GameState oldState) {
    int offset = 0;
    
    copyFileVRAM(BG_GFX, "menu_levelselect", FS_MENU_GRAPHICS);
    copyFileVRAM(BG_PALETTE, "menu_levelselect", FS_PALETTES);
    
    if (oldState != MENU_LEVELSELECT) {
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
        copyFileVRAM(SPRITE_GFX + ((8704+4096+32*32+1024)>>1), "menu_bit_medals_small", FS_MENU_GRAPHICS);
    }
}

int initMenuLevelSelect() {
    levelSelectState = LSS_NORMAL;
    
    REG_DISPCNT = ( MODE_3_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT = BG_BMP8_256x256;
/*  REG_BG3PA = 1<<8; // scaling of 1
    REG_BG3PB = 0;
    REG_BG3PC = 0;
    REG_BG3PD = 1<<8; // scaling of 1
    REG_BG3X  = 0;
    REG_BG3Y  = 0;*/
    
/*  REG_DISPCNT_SUB = ( MODE_3_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT_SUB = BG_BMP8_256x256;
    REG_BG3PA_SUB = 1<<8; // scaling of 1
    REG_BG3PB_SUB = 0;
    REG_BG3PC_SUB = 0;
    REG_BG3PD_SUB = 1<<8; // scaling of 1
    REG_BG3X_SUB  = 0;
    REG_BG3Y_SUB  = 0;*/
    
    playMiscSoundeffect("menu_levelselect");
    
    return 0;
}
