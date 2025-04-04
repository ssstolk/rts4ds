// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_ingame.h"

#include <time.h>

#include "view.h"
#include "soundeffects.h"
#include "guide.h"


uint16 *menuIngameVolumeGraphics;


static void displayTwoDigitNumber(int nr, int x, int y) {
    char time[3];
    
    time[0] = '0' + (nr / 10);
    time[1] = '0' + (nr % 10);
    time[2] = '\0';
    drawCaptionGame(time, x, y, 0);
}

void drawMenuIngame() {
    int i;
    
    initSpritesUsedInfoScreen();
    drawMenuOption(0, "  continue", 0);
    drawMenuOption(1, "  save game", 0);
    drawMenuOption(2, "  gameinfo", 0);
    for (i=0; i<32*16/2; i++) {
        (SPRITE_GFX_SUB + (((8704+4096)+32*32)>>1))[i] = (menuIngameVolumeGraphics + ((32*16 * (getMusicVolume() / 10))>>1))[i];
    }
    setSpriteInfoScreen(48 + 3*24, ATTR0_WIDE,
                        151, SPRITE_SIZE_32, 0, 0,
                        0, 0, ((8704+4096)+32*32) / (8*8));
    drawMenuOption(3, "  music volume", 0);
    drawMenuOption(4, "  exit mission", 0);

    {
        time_t unixTime = time(NULL);
        struct tm* timeStruct = gmtime((const time_t *)&unixTime);
        
        displayTwoDigitNumber(timeStruct->tm_hour, 101, 174);
        displayTwoDigitNumber(timeStruct->tm_min,  120, 174);
        displayTwoDigitNumber(timeStruct->tm_sec,  139, 174);
    }
    
    setSpriteInfoScreen(SCREEN_HEIGHT - 32, ATTR0_SQUARE, // the "back" sprite
                        0, SPRITE_SIZE_32, 0, 0,
                        0, 0, (8704+4096)/(8*8));
}

void drawMenuIngameBG() {
    REG_DISPCNT_SUB |= DISPLAY_BG3_ACTIVE;
}

void doMenuIngameLogic() {
    if (keysDown() & KEY_START) {
        free(menuIngameVolumeGraphics);
        setGameState(INGAME);
        playSoundeffect(SE_MENU_OK);
        return;
    }
    
    if ((keysUp() & KEY_B) || // needs to be on up, to ensure the B-button doesn't appear as hotkey press ingame
        ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px < 32 && touchReadLast().py >= SCREEN_HEIGHT - 32)) {
        free(menuIngameVolumeGraphics);
        updateKeys(); // also to ensure the B-button doesn't appear as hotkey press ingame
        setGameState(INGAME);
        playSoundeffect(SE_MENU_BACK);
    
    } else if ((getKeysOnUp() & KEY_TOUCH) // GUIDE key = 172,168 - 220,192
         && touchReadLast().px >= 172 && touchReadLast().px <= 220 
         && touchReadLast().py >= 168) {
        setGameState(GUIDE_INGAME);
        playSoundeffect(SE_MENU_OK);
    
	} else if ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px >= 238 && touchReadLast().px < 238+16 &&
                                             touchReadLast().py >= 3   && touchReadLast().py < 3+16) {
        
        cycleBacklightLevel();
		playSoundeffect(SE_MENU_OK);
	
    } else if (updateMenuOption(5)) {
        if (getMenuOption() == 0) {
            free(menuIngameVolumeGraphics);
            setGameState(INGAME);
            playSoundeffect(SE_MENU_OK);
        } else if (getMenuOption() == 1) {
            setGameState(MENU_SAVEGAME);
            playSoundeffect(SE_MENU_OK);
        } else if (getMenuOption() == 2) {
            setGameState(MENU_GAMEINFO);
            playSoundeffect(SE_MENU_OK);
        } else if (getMenuOption() == 3) {
            if (getMusicVolume() == 100)
                setMusicVolume(0);
            else
                setMusicVolume(getMusicVolume() + 10);
            playSoundeffect(SE_MENU_OK);
            writeSaveFile();
        } else if (getMenuOption() == 4) {
            lcdMainOnBottom(); // SUPERFLUOUS! see setGameState()
            free(menuIngameVolumeGraphics);
            setGameState(MENU_MAIN);
            playSoundeffect(SE_MENU_OK);
        }
    } else {
        if (getMenuOption() == 3) {
            if ((keysDown() & KEY_LEFT) || (getKeysHeldTimeMin(KEY_LEFT) >= SCREEN_REFRESH_RATE/2 && ((getKeysHeldTimeMin(KEY_LEFT) - SCREEN_REFRESH_RATE/2) % (SCREEN_REFRESH_RATE/6)) == 0)) {
                if (getMusicVolume() > 0) {
                    setMusicVolume(getMusicVolume() - 10);
                    playSoundeffect(SE_MENU_OK);
                    writeSaveFile();
                }
            } else if ((keysDown() & KEY_RIGHT) || (getKeysHeldTimeMin(KEY_RIGHT) >= SCREEN_REFRESH_RATE/2 && ((getKeysHeldTimeMin(KEY_RIGHT) - SCREEN_REFRESH_RATE/2) % (SCREEN_REFRESH_RATE/6)) == 0)) {
                if (getMusicVolume() < 100) {
                    setMusicVolume(getMusicVolume() + 10);
                    playSoundeffect(SE_MENU_OK);
                    writeSaveFile();
                }
            }
        }
    }
}

void loadMenuIngameGraphics(enum GameState oldState) {
    int offset = 0;
    
    copyFileVRAM(BG_PALETTE_SUB, "menu_ingame", FS_PALETTES);
    copyFileVRAM(BG_GFX_SUB, "menu_ingame", FS_MENU_GRAPHICS);
    
    copyFileVRAM(SPRITE_PALETTE_SUB, "menu_bits", FS_PALETTES);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "menu_bit_font", FS_MENU_GRAPHICS);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "menu_bit_bar", FS_MENU_GRAPHICS);
    
    menuIngameVolumeGraphics = (uint16*) malloc(11 * 32*16);
    copyFile(menuIngameVolumeGraphics, "menu_bit_volume", FS_MENU_GRAPHICS);
    
    if (oldState == INGAME)
        copyFileVRAM(SPRITE_GFX_SUB + ((8704+4096)>>1), "menu_bit_back", FS_MENU_GRAPHICS);
}

int initMenuIngame() {
    REG_DISPCNT_SUB = ( MODE_3_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT_SUB = BG_BMP8_256x256;
/*  REG_BG3PA_SUB = 1<<8; // scaling of 1
    REG_BG3PB_SUB = 0;
    REG_BG3PC_SUB = 0;
    REG_BG3PD_SUB = 1<<8; // scaling of 1*/
    REG_BG3X_SUB  = 0;
    REG_BG3Y_SUB  = 0;
    
    lcdMainOnTop();
    
    playMiscSoundeffect("menu_ingame");
    
    return 0;
}
