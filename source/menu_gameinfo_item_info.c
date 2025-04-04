// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_gameinfo_item_info.h"

#include "menu_gameinfo.h"
#include "menu_gameinfo_item_select.h"
#include "structures.h"
#include "units.h"
#include "screen_gif.h"
#include "soundeffects.h"
#include "game.h"


static pScreenGifHandler screenGif;
static enum GameState previousGameState;




void drawMenuGameinfoItemInfo() {
    initSpritesUsedInfoScreen();
    setSpriteInfoScreen(SCREEN_HEIGHT - 32, ATTR0_SQUARE, // the "back" sprite
                        0, SPRITE_SIZE_32, 0, 0,
                        0, 0, (8704+4096)/(8*8));
    
    drawScreenGif(screenGif);
}



void drawMenuGameinfoItemInfoBG() {
    BG_PALETTE_SUB[0] = 0;
}



void doMenuGameinfoItemInfoLogic() {
    if ((keysUp() & KEY_B) ||
        ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px < 32 && touchReadLast().py >= SCREEN_HEIGHT - 32)) {
        if (previousGameState == INGAME)
            setGameState(INGAME);
        else {
            setGameState(MENU_GAMEINFO_ITEM_SELECT);
            playSoundeffect(SE_MENU_BACK);
        }
        screenGifHandlerDestroy(screenGif);
        screenGif = NULL;
        return;
    }
    
    doScreenGifLogic(screenGif);
}



void loadMenuGameinfoItemInfoGraphics(enum GameState oldState) {
    if (oldState < MENU_INGAME) {
        copyFileVRAM(SPRITE_PALETTE_SUB, "menu_bits", FS_PALETTES);
        copyFileVRAM(SPRITE_GFX_SUB + ((8704+4096)>>1), "menu_bit_back", FS_MENU_GRAPHICS);
    }
}



int initMenuGameinfoItemInfo() {
    screenGif = initScreenGif((getGameinfoIsStructure() ? structureInfo[getGameinfoItemNr()].name : unitInfo[getGameinfoItemNr()].name), FS_GAMEINFO_GRAPHICS, SGO_SCREEN_SUB | SGO_REPEAT);
    previousGameState = getGameState();
    
    REG_DISPCNT_SUB = ( MODE_3_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT_SUB = BG_BMP8_256x256;
    
    lcdMainOnTop();
    
    playMiscSoundeffect("menu_gameinfo_item_info"); // startup sounds for non-GIF screens
    
    return 0;
}
