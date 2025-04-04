// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_soundroom.h"

#include "animation.h"
#include "factions.h"
#include "soundeffects.h"
#include "music.h"


enum MusicType soundroomMusicType;
int soundroomSoundeffect;


void drawMenuSoundroom() {
    initSpritesUsedInfoScreen();
    initSpritesUsedPlayScreen();
    
    setSpritePlayScreen(SCREEN_HEIGHT - 32, ATTR0_SQUARE, // the "back" sprite
                        0, SPRITE_SIZE_32, 0, 0,
                        0, 0, (8704+4096)/(8*8));
    
    drawMenuOption(0, factionInfo[getFaction(FRIENDLY)].name, 1);
    
    if (soundroomMusicType == MT_MENU)
        drawMenuOption(1, "  music type   menu", 1);
    else if (soundroomMusicType == MT_MENU_FACTION)
        drawMenuOption(1, "  music type   faction", 1);
    else if (soundroomMusicType == MT_INGAME)
        drawMenuOption(1, "  music type   ingame", 1);
    else
        drawMenuOption(1, "  music type   none", 1);
    
    drawMenuOption(2, "  next music", 1);
    drawMenuOption(3, "  fx current", 1);
    drawMenuOption(4, "  fx next", 1);
    
    drawAnimation();
}

void drawMenuSoundroomBG() {
    REG_DISPCNT |= DISPLAY_BG3_ACTIVE;
/*    REG_DISPCNT_SUB |= DISPLAY_BG3_ACTIVE;*/
    drawAnimationBG();
}

void doMenuSoundroomLogic() {
    if (doAnimationLogic()) // loop the animation please.
        initAnimation("soundroom", 1, 1, 0);
    
    if ((keysDown() & KEY_B) ||
        ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px < 32 && touchReadLast().py >= SCREEN_HEIGHT - 32)) {
        setGameState(MENU_MAIN);
        playSoundeffect(SE_MENU_BACK);
    } else if (updateMenuOption(5)) {
        if (getMenuOption() == 0) {
            if (getFaction(FRIENDLY) + 1 < MAX_DIFFERENT_FACTIONS && factionInfo[getFaction(FRIENDLY) + 1].enabled)
                setFaction(FRIENDLY, getFaction(FRIENDLY) + 1);
            else
                setFaction(FRIENDLY, 0);
            if (soundroomMusicType == MT_INGAME) {
                soundroomMusicType = MT_NONE;
                stopMusic();
            }
            playSoundeffect(SE_MENU_OK);
            initSoundeffectsWithScenario();
            soundroomSoundeffect = 0;
        } else if (getMenuOption() == 1) {
            soundroomMusicType = (soundroomMusicType + 1) % MT_MISC;
            playSoundeffect(SE_MENU_OK);
            playMusic(soundroomMusicType, MUSIC_LOOP);
        } else if (getMenuOption() == 2) {
            if (soundroomMusicType == MT_INGAME) {
                playSoundeffect(SE_MENU_OK);
                stopMusic();
                playMusic(soundroomMusicType, MUSIC_LOOP);
            } else
                playSoundeffect(SE_CANNOT);
        } else if (getMenuOption() == 3) {
            playSoundeffect(soundroomSoundeffect);
        } else if (getMenuOption() == 4) {
            do {
                soundroomSoundeffect = (soundroomSoundeffect + 1) % SE_MISC;
            } while (!playSoundeffect(soundroomSoundeffect));
        }
    }
}

void loadMenuSoundroomGraphics(enum GameState oldState) {
    int offset = 0;
    
    copyFileVRAM(BG_GFX, "menu_soundroom", FS_MENU_GRAPHICS);
    copyFileVRAM(BG_PALETTE, "menu_soundroom", FS_PALETTES);
    
    if (oldState != MENU_MAIN) {
/*        copyFileVRAM(BG_PALETTE_SUB, "logo", FS_PALETTES);
        copyFileVRAM(BG_GFX_SUB, "logo", FS_MENU_GRAPHICS);
*/        
        copyFileVRAM(SPRITE_PALETTE, "menu_bits", FS_PALETTES);
        offset += copyFileVRAM(SPRITE_GFX + (offset>>1), "menu_bit_font", FS_MENU_GRAPHICS);
        offset += copyFileVRAM(SPRITE_GFX + (offset>>1), "menu_bit_bar", FS_MENU_GRAPHICS);
        offset += copyFileVRAM(SPRITE_GFX + (offset>>1), "menu_bit_back", FS_MENU_GRAPHICS);
    }
    
    loadAnimationGraphics(oldState);
}

int initMenuSoundroom() {
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
    
    soundroomMusicType = MT_NONE;
    soundroomSoundeffect = 0;
    
    setFaction(FRIENDLY, 0);
    initSoundeffectsWithScenario();
    
    initAnimation("soundroom", 1, 1, 0);
    
    playMiscSoundeffect("menu_soundroom");
    
    return 0;
}
