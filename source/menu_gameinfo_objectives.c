// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_gameinfo_objectives.h"

#include <stdio.h>

#include "menu_gameinfo.h"
#include "factions.h"
#include "info.h"
#include "screen_gif.h"
#include "soundeffects.h"


static pScreenGifHandler screenGif;




void drawMenuGameinfoObjectives() {
    initSpritesUsedInfoScreen();
    setSpriteInfoScreen(SCREEN_HEIGHT - 32, ATTR0_SQUARE, // the "back" sprite
                        0, SPRITE_SIZE_32, 0, 0,
                        0, 0, (8704+4096)/(8*8));
    
    drawScreenGif(screenGif);
}



void drawMenuGameinfoObjectivesBG() {
    BG_PALETTE_SUB[0] = 0;
}



void doMenuGameinfoObjectivesLogic() {
    if ((keysUp() & KEY_B) ||
        ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px < 32 && touchReadLast().py >= SCREEN_HEIGHT - 32)) {
        setGameState(MENU_GAMEINFO);
        playSoundeffect(SE_MENU_BACK);
        screenGifHandlerDestroy(screenGif);
        screenGif = NULL;
        return;
    }
    
    doScreenGifLogic(screenGif);
}



void loadMenuGameinfoObjectivesGraphics(enum GameState oldState) {
}



int initMenuGameinfoObjectives() {
    char filename[256];
    
    sprintf(filename, "scenario_%s_level%i_region%i", factionInfo[getFaction(FRIENDLY)].name, getLevel(), getRegion());
    screenGif = initScreenGif(filename, FS_GAMEINFO_GRAPHICS, SGO_SCREEN_SUB | SGO_REPEAT);
    
    REG_DISPCNT_SUB |= (DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64);
    
    lcdMainOnTop();
    
    playMiscSoundeffect("menu_gameinfo_objectives");
    
    return 0;
}
