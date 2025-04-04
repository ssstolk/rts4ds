// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_main.h"

#include "factions.h"
#include "settings.h"
#include "animation.h"
#include "soundeffects.h"


static int continueGameAvailable, extrasAvailable;




void drawMenuMain() {
    initSpritesUsedInfoScreen();
    initSpritesUsedPlayScreen();
    
    if (continueGameAvailable)
        drawMenuOption(0, "  continue", 1);
    
    drawMenuOption(0 + continueGameAvailable, "  new game", 1);
    drawMenuOption(1 + continueGameAvailable, "  load game", 1);
    
    if (extrasAvailable)
        drawMenuOption(2 + continueGameAvailable, "  level select", 1);
    
    drawMenuOption(2 + continueGameAvailable + extrasAvailable, "  settings", 1);
    
    drawAnimation();
}



void drawMenuMainBG() {
    REG_DISPCNT |= DISPLAY_BG3_ACTIVE;
    REG_DISPCNT_SUB |= DISPLAY_BG3_ACTIVE;
}



void doMenuMainLogic() {
    if (doAnimationLogic()) // loop the animation please.
        initAnimation("menu", 1, 1, 0);
    
    #ifndef DISABLE_MENU_RTS4DS
    if (keysDown() & KEY_B) {
        setGameState(MENU_RTS4DS);
        playSoundeffect(SE_MENU_BACK);
    } else
    #endif
    
    if ((getKeysOnUp() & KEY_TOUCH) // GUIDE key = 172,168 - 220,192
         && touchReadLast().px >= 172 && touchReadLast().px <= 220 
         && touchReadLast().py >= 168) {
        setGameState(GUIDE_MAIN);
        playSoundeffect(SE_MENU_OK);
    }
    
#ifndef DEBUG_BUILD
    if (extrasAvailable) {
#endif
        if ((getKeysOnUp() & KEY_TOUCH) // SOUNDROOM key = 111,168 - 159,192
             && touchReadLast().px >= 111 && touchReadLast().px <= 159 
             && touchReadLast().py >= 168) {
            setGameState(MENU_SOUNDROOM);
            playSoundeffect(SE_MENU_OK);
        }
#ifndef DEBUG_BUILD
    }
#endif
    
#ifndef DEBUG_BUILD
    if (extrasAvailable) {
#endif
        if ((getKeysOnUp() & KEY_TOUCH) // GALLERY key = 50,168 - 98,192
             && touchReadLast().px >= 50 && touchReadLast().px <= 98 
             && touchReadLast().py >= 168) {
            setGameState(GALLERY);
            playSoundeffect(SE_MENU_OK);
        }
#ifndef DEBUG_BUILD
    }
#endif
    
    if (updateMenuOption(3 + continueGameAvailable + extrasAvailable)) {
        if (getMenuOption() == 0 && continueGameAvailable) {
            setGameState(MENU_CONTINUE);
            playSoundeffect(SE_MENU_OK);
        } else if (getMenuOption() == 0 + continueGameAvailable) {
            setLevel(1);
            setGameState(MENU_FACTIONS);
            playSoundeffect(SE_MENU_OK);
        } else if (getMenuOption() == 1 + continueGameAvailable) {
            // load saved game
            setGameState(MENU_LOADGAME);
            playSoundeffect(SE_MENU_OK);
        } else if ((getMenuOption() == 2 + continueGameAvailable) && extrasAvailable) {
            setGameState(MENU_LEVELSELECT);
            playSoundeffect(SE_MENU_OK);
        } else if (getMenuOption() == 2 + continueGameAvailable + extrasAvailable) {
            setGameState(MENU_SETTINGS);
            playSoundeffect(SE_MENU_OK);
        }
    } else if (getMenuIdleTime() >= 110*SCREEN_REFRESH_RATE)
        setGameState(INTRO);
}



void loadMenuMainGraphics(enum GameState oldState) {
    int offset = 0;
    
    if (oldState != MENU_CONTINUE && oldState != MENU_SETTINGS && oldState != MENU_FACTIONS && oldState != MENU_SCENARIOS) {
        copyFileVRAM(BG_PALETTE_SUB, "logo", FS_PALETTES);
        copyFileVRAM(BG_GFX_SUB, "logo", FS_MENU_GRAPHICS);
        initAnimation("menu", 1, 1, 0);
        loadAnimationGraphics(oldState);
        drawAnimationBG();
        
        copyFileVRAM(SPRITE_PALETTE, "menu_bits", FS_PALETTES);
        offset += copyFileVRAM(SPRITE_GFX + (offset>>1), "menu_bit_font", FS_MENU_GRAPHICS);
        offset += copyFileVRAM(SPRITE_GFX + (offset>>1), "menu_bit_bar", FS_MENU_GRAPHICS);
        offset += copyFileVRAM(SPRITE_GFX + (offset>>1), "menu_bit_back", FS_MENU_GRAPHICS);
    }
    
#ifndef DEBUG_BUILD
    if (!extrasAvailable)
        copyFileVRAM(BG_GFX, "menu_main", FS_MENU_GRAPHICS);
    else
#endif
        copyFileVRAM(BG_GFX, "menu_main_unlock", FS_MENU_GRAPHICS);
    copyFileVRAM(BG_PALETTE, "menu_main", FS_PALETTES);
}



int initMenuMain() {
    int i;
    
    continueGameAvailable = 0;
    
    if (getLevelCurrentFactionInfo() != -1 && getLevelCurrentFactionInfo() < 3) {
        if (getLevelCurrent() <= 9 /*|| getLevelCurrentFactionInfo()+1 < 3*/)
            continueGameAvailable = 1;
    }
    for (i=0; i<3 /*MAX_DIFFERENT_FACTIONS*/ && factionInfo[i].enabled; i++) {
        if (getLevelReached(i) > 1 && getLevelReached(i) <= 9 && (i != getLevelCurrentFactionInfo() || getLevelReached(i) > getLevelCurrent()))
            continueGameAvailable = 1;
    }
    
    for (i=0; i<3 && getFactionMedalReceived(i) >= MEDAL_BRONZE && getFactionMedalReceived(i) <= MEDAL_GOLD; i++);
    extrasAvailable = (i==3);
    
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
    
    playMiscSoundeffect("menu_main"); // startup sounds for non-GIF screens
    
    return 0;
}
