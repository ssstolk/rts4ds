// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_factions.h"

#include <stdlib.h>
#include <string.h>

#include "factions.h"
#include "animation.h"
#include "soundeffects.h"
#include "cutscene_introduction.h"
#include "shared.h"

static u8 *menuFactionsPieces;
static int soleFactionSelectable;
static int timer;


void drawMenuFactions() {
    int x;
    int i;
    
    initSpritesUsedInfoScreen();
    initSpritesUsedPlayScreen();
    setSpritePlayScreen(SCREEN_HEIGHT - 32, ATTR0_SQUARE, // the "back" sprite
                        0, SPRITE_SIZE_32, 0, 0,
                        0, 0, (8704+4096)/(8*8));

    
    for (i=0; i<3; i++) {
        if (getFactionMedalReceived(i) >= MEDAL_BRONZE && getFactionMedalReceived(i) <= MEDAL_GOLD) {
            x = 28;
            if (i == 2) x = 199;
            else if (i == 1) x = 113;
            setSpritePlayScreen(144, ATTR0_WIDE,              // the "faction medal" sprite
                                x, SPRITE_SIZE_32, 0, 0,
                                0, 0, ((8704+4096+32*32+1024) + getFactionMedalReceived(i)*32*16)/(8*8));
        }
    }
    
    x=-1;
    if (getFaction(FRIENDLY) == 0)
        x = 11;
    if (getFaction(FRIENDLY) == 1 || soleFactionSelectable == getFaction(FRIENDLY))
        x = 96;
    if (getFaction(FRIENDLY) == 2)
        x = 182;
    
    if (x>=0 && (soleFactionSelectable==-1 || ((timer/FPS)%2))) {
        setSpritePlayScreen(146, ATTR0_WIDE,                 // the "selection" sprite (first half)
                            x, SPRITE_SIZE_32, 0, 0,
                            0, 0, (8704+4096+32*32)/(8*8));
        
        setSpritePlayScreen(146, ATTR0_WIDE,                 // the "selection" sprite (second half)
                            x+32, SPRITE_SIZE_32, 0, 0,
                            0, 0, (8704+4096+32*32+32*16)/(8*8));
    }
    
    drawAnimation();
}

void drawMenuFactionsBG() {
    REG_DISPCNT |= DISPLAY_BG3_ACTIVE;
/*    REG_DISPCNT_SUB |= DISPLAY_BG3_ACTIVE;*/
}


void doMenuFactionsLogic() {
//    char filename[256];
    u8 paletteEntry;
    int factionNr;
    
    timer++;
    
    if (doAnimationLogic()) // loop the animation please.
        initAnimation("menu", 1, 1, 0);
    
    if (getKeysOnUp() & KEY_TOUCH) {
        if (touchReadLast().px < 32 && touchReadLast().py >= SCREEN_HEIGHT - 32) {
            free(menuFactionsPieces);
            setGameState(MENU_MAIN);
            playSoundeffect(SE_MENU_BACK);
        } else {
            paletteEntry = menuFactionsPieces[touchReadLast().py * 256 + touchReadLast().px];
            if (paletteEntry != 0) { // a faction was selected
                factionNr = paletteEntry - 1;
                if (getLevelReached(factionInfo[factionNr].unlock_faction_info) > factionInfo[factionNr].unlock_faction_level) {
                    if ((factionNr < 3 && getFaction(FRIENDLY) == factionNr) ||
                        (factionNr >= 3 && (factionNr == soleFactionSelectable
#ifndef DEBUG_BUILD
                                       || (getFactionMedalReceived(0) == MEDAL_GOLD && getFactionMedalReceived(1) == MEDAL_GOLD && getFactionMedalReceived(2) == MEDAL_GOLD)
#endif
                        ))) {
                        setLevelLoadedViaLevelSelect(0);
                        setFaction(FRIENDLY, factionNr);
                        free(menuFactionsPieces);
                        initCutsceneIntroductionFilename();
                        setGameState(CUTSCENE_INTRODUCTION);
                        playSoundeffect(SE_MENU_OK);
                    } else if (soleFactionSelectable < 0) { // i.e., if not a demo build in which only a single faction can be selected
                        setFaction(FRIENDLY, factionNr);
                        playSoundeffect(SE_MENU_SELECT);
                    }
                }
            }
        }
    }
    else if (soleFactionSelectable >= 0) {
        if ((keysDown() & (KEY_LEFT | KEY_RIGHT)) && getFaction(FRIENDLY) != soleFactionSelectable) {
            setFaction(FRIENDLY, soleFactionSelectable);
            playSoundeffect(SE_MENU_SELECT);
        }
    } 
    else if (keysDown() & KEY_LEFT) {
        if (getFaction(FRIENDLY) >= 3)
            factionNr = 0;
        else
            factionNr = getFaction(FRIENDLY) - 1;
        if (factionNr >= 0 && getLevelReached(factionInfo[factionNr].unlock_faction_info) > factionInfo[factionNr].unlock_faction_level) {
            setFaction(FRIENDLY, factionNr);
            playSoundeffect(SE_MENU_SELECT);
        }
    } 
    else if (keysDown() & KEY_RIGHT) {
        if (getFaction(FRIENDLY) >= 3)
            factionNr = 0;
        else
            factionNr = getFaction(FRIENDLY) + 1;
        if (factionNr <= 2 && getLevelReached(factionInfo[factionNr].unlock_faction_info) > factionInfo[factionNr].unlock_faction_level) {
            setFaction(FRIENDLY, factionNr);
            playSoundeffect(SE_MENU_SELECT);
        }
    }
    if ((keysDown() & KEY_A) || (soleFactionSelectable >= 0 && timer > 6*FPS)) {
        if (soleFactionSelectable >= 0)
            setFaction(FRIENDLY, soleFactionSelectable);
        if (soleFactionSelectable >= 0 || getFaction(FRIENDLY) < 3
#ifndef DEBUG_BUILD 
                                       || (getFactionMedalReceived(0) == MEDAL_GOLD && getFactionMedalReceived(1) == MEDAL_GOLD && getFactionMedalReceived(2) == MEDAL_GOLD)
#endif
        ) {

            setLevelLoadedViaLevelSelect(0);
            free(menuFactionsPieces);
            initCutsceneIntroductionFilename();
            setGameState(CUTSCENE_INTRODUCTION);
            playSoundeffect(SE_MENU_OK);
        }
    } else if (keysDown() & KEY_B) {
        free(menuFactionsPieces);
        setGameState(MENU_MAIN);
        playSoundeffect(SE_MENU_BACK);
    }
}


void loadMenuFactionsGraphics(enum GameState oldState) {
    char filename[256];
    u8 paletteEntry;
    int factionNr;
    int i;
    
    if (oldState != MENU_MAIN) {
/*        copyFileVRAM(BG_PALETTE_SUB, "logo", FS_PALETTES);
        copyFileVRAM(BG_GFX_SUB, "logo", FS_MENU_GRAPHICS);*/
        initAnimation("menu", 1, 1, 0);
        loadAnimationGraphics(oldState);
        drawAnimationBG();
        
        copyFileVRAM(SPRITE_PALETTE, "menu_bits", FS_PALETTES);
        copyFileVRAM(SPRITE_GFX + ((8704+4096)>>1), "menu_bit_back", FS_MENU_GRAPHICS);
    }
    
    copyFileVRAM(BG_PALETTE, "factions", FS_PALETTES);
    
    strcpy(filename, "factions");
    for (i=0; i<MAX_DIFFERENT_FACTIONS && factionInfo[i].enabled; i++) {
        if (getLevelReached(factionInfo[i].unlock_faction_info) > factionInfo[i].unlock_faction_level) {
            if (i<3 || (getFactionMedalReceived(0) == MEDAL_GOLD && getFactionMedalReceived(1) == MEDAL_GOLD && getFactionMedalReceived(2) == MEDAL_GOLD)) {
                strcat(filename, "_");
                strcat(filename, factionInfo[i].name);
            }
        }
    }
    copyFileVRAM(BG_GFX, filename, FS_MENU_GRAPHICS);
    
    menuFactionsPieces = (u8*) malloc(256 * 192);
    copyFileVRAM((uint16*) menuFactionsPieces, "factions_pieces", FS_MENU_GRAPHICS);
    soleFactionSelectable = -1;
    for (i=0; i<256 * 192; i++) {
        paletteEntry = menuFactionsPieces[i];
        if (paletteEntry != 0) { // a faction is selectable
            factionNr = paletteEntry - 1;
            if (soleFactionSelectable == -1 || factionNr == soleFactionSelectable) {
                soleFactionSelectable = factionNr;
            }
            else { // two or more factions can be selected on this screen!
                soleFactionSelectable = -1;
                break;
            }
        }
    }
    if (soleFactionSelectable >= 0)
        setFaction(FRIENDLY, soleFactionSelectable);
    
    copyFileVRAM(SPRITE_GFX + ((8704+4096+32*32)>>1), "factions_bit_selection", FS_MENU_GRAPHICS);
    copyFileVRAM(SPRITE_GFX + ((8704+4096+32*32+1024)>>1), "menu_bit_medals", FS_MENU_GRAPHICS);
}


int initMenuFactions() {
    //int i;
    
    timer = 0;
    
    REG_DISPCNT = ( MODE_3_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT = BG_BMP8_256x256;
    
    /*setFaction(FRIENDLY, 0);
    for (i=2; i>=0; i--) {
        if (getLevelReached(factionInfo[i].unlock_faction_info) > factionInfo[i].unlock_faction_level) {
            setFaction(FRIENDLY, i);
            break;
        }
    }*/
    
    setFaction(FRIENDLY, 0);
    
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
    
    playMiscSoundeffect("menu_factions"); // startup sounds for non-GIF screens
    
    return 0;
}
