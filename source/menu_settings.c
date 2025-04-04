// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_settings.h"

#include "settings.h"
#include "animation.h"
#include "soundeffects.h"


uint16 *menuSettingsVolumeGraphics;


void drawMenuSettings() {
    int i;
    
    initSpritesUsedInfoScreen();
    initSpritesUsedPlayScreen();
    
    setSpritePlayScreen(SCREEN_HEIGHT - 32, ATTR0_SQUARE, // the "back" sprite
                        0, SPRITE_SIZE_32, 0, 0,
                        0, 0, (8704+4096)/(8*8));
    
    if (getPrimaryHand() == RIGHT_HANDED)
        drawMenuOption(0, "  controls      right", 1);
    else
        drawMenuOption(0, "  controls      left", 1);
    
    if (getScreenSwitchMethod() == HOLD_TRIGGER)
        drawMenuOption(1, "  screen-swap   hold", 1);
    else
        drawMenuOption(1, "  screen-swap   press", 1);
    
    if (getGameSpeed() == 2)
        drawMenuOption(2, "  game speed    slower", 1);
    else if (getGameSpeed() == 3)
        drawMenuOption(2, "  game speed    normal", 1);
    else
        drawMenuOption(2, "  game speed    slowest", 1);

    if (getSubtitlesEnabled())
        drawMenuOption(4, "  subtitles      enabled", 1);
    else
        drawMenuOption(4, "  subtitles      disabled", 1);

	#ifdef _MONAURALSWITCH_
	if (getMonauralEnabled())
	    drawMenuOption(5, "  audio          mono", 1);
    else
	    drawMenuOption(5, "  audio          stereo", 1);
    #endif
    
    for (i=0; i<32*16/2; i++) {
        (SPRITE_GFX + ((8704+4096+32*32)>>1))[i] = (menuSettingsVolumeGraphics + ((32*16 * (getMusicVolume() / 10))>>1))[i];
    }
    setSpritePlayScreen(48 + 3*24, ATTR0_WIDE,
                        151, SPRITE_SIZE_32, 0, 0,
                        0, 0, (8704+4096+32*32) / (8*8));
    drawMenuOption(3, "  music volume", 1);
    drawAnimation();
}

void drawMenuSettingsBG() {
    REG_DISPCNT |= DISPLAY_BG3_ACTIVE;
/*    REG_DISPCNT_SUB |= DISPLAY_BG3_ACTIVE;*/
}

void doMenuSettingsLogic() {
    if (doAnimationLogic()) // loop the animation please.
        initAnimation("menu", 1, 1, 0);
    
    if ((keysDown() & KEY_B) ||
        ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px < 32 && touchReadLast().py >= SCREEN_HEIGHT - 32)) {
        free(menuSettingsVolumeGraphics);
        setGameState(MENU_MAIN);
        playSoundeffect(SE_MENU_BACK);
    } else if ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px >= 238 && touchReadLast().px < 238+16 &&
                                             touchReadLast().py >= 3   && touchReadLast().py < 3+16) {
        cycleBacklightLevel();
		playSoundeffect(SE_MENU_OK);
        writeSaveFile();  // this is meant to slow down...
	} else
    #ifdef _MONAURALSWITCH_
    if (updateMenuOption(6)) {
    #else
    if (updateMenuOption(5)) {
    #endif
        if (getMenuOption() == 0) {
            if (getPrimaryHand() == RIGHT_HANDED)
                setPrimaryHand(LEFT_HANDED);
            else
                setPrimaryHand(RIGHT_HANDED);
            playSoundeffect(SE_MENU_OK);
        } else if (getMenuOption() == 1) {
            if (getScreenSwitchMethod() == PRESS_TRIGGER)
                setScreenSwitchMethod(HOLD_TRIGGER);
            else
                setScreenSwitchMethod(PRESS_TRIGGER);
            playSoundeffect(SE_MENU_OK);
        } else if (getMenuOption() == 2) {
            setGameSpeed((getGameSpeed() % 3) + 1);
            playSoundeffect(SE_MENU_OK);
        } else if (getMenuOption() == 3) {
            if (getMusicVolume() == 100)
                setMusicVolume(0);
            else
                setMusicVolume(getMusicVolume() + 10);
            playSoundeffect(SE_MENU_OK);
        } else if (getMenuOption() == 4) {
            setSubtitlesEnabled((getSubtitlesEnabled() + 1) % 2);
            playSoundeffect(SE_MENU_OK);
        }
        #ifdef _MONAURALSWITCH_
		else if (getMenuOption() == 5) {
		   setMonauralEnabled(1-getMonauralEnabled());   // (swap 0/1)
		   playSoundeffect(SE_MENU_OK);
		}
		#endif
        writeSaveFile();
    } else {
        if (getMenuOption() == 2) {
            if (keysDown() & KEY_LEFT) {
                if (getGameSpeed() > 1) {
                    setGameSpeed(getGameSpeed() - 1);
                    playSoundeffect(SE_MENU_OK);
                    writeSaveFile();
                }
            } else if (keysDown() & KEY_RIGHT) {
                if (getGameSpeed() < 3) {
                    setGameSpeed(getGameSpeed() + 1);
                    playSoundeffect(SE_MENU_OK);
                    writeSaveFile();
                }
            }
        } else if (getMenuOption() == 3) {
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

void loadMenuSettingsGraphics(enum GameState oldState) {
    int offset = 0;
    
    copyFileVRAM(BG_GFX, "menu_settings", FS_MENU_GRAPHICS);
    copyFileVRAM(BG_PALETTE, "menu_settings", FS_PALETTES);
    
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
    
    menuSettingsVolumeGraphics = (uint16*) malloc(11 * 32*16);
    copyFile(menuSettingsVolumeGraphics, "menu_bit_volume", FS_MENU_GRAPHICS);
}

int initMenuSettings() {
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
    
    playMiscSoundeffect("menu_settings");
    
    return 0;
}
