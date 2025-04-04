// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_gameinfo.h"

#include "view.h"
#include "factions.h"
#include "soundeffects.h"


static int gameinfoIsStructure;
static int currentObjectivesAvailable;




void setGameinfoIsStructure(int on) {
    gameinfoIsStructure = on;
}

int getGameinfoIsStructure() {
    return gameinfoIsStructure;
}



void drawMenuGameinfo() {
    initSpritesUsedInfoScreen();
    
    if (currentObjectivesAvailable)
        drawMenuOption(0, "  mission objectives", 0);
    
    drawMenuOption(0 + currentObjectivesAvailable, "  info structures", 0);
    drawMenuOption(1 + currentObjectivesAvailable, "  info units", 0);
    drawMenuOption(2 + currentObjectivesAvailable, "  techtree structures", 0);
    drawMenuOption(3 + currentObjectivesAvailable, "  techtree units", 0);

    setSpriteInfoScreen(SCREEN_HEIGHT - 32, ATTR0_SQUARE, // the "back" sprite
                        0, SPRITE_SIZE_32, 0, 0,
                        0, 0, (8704+4096)/(8*8));
}



void drawMenuGameinfoBG() {
    REG_DISPCNT_SUB |= DISPLAY_BG3_ACTIVE;
}



void doMenuGameinfoLogic() {
    if ((keysUp() & KEY_B) ||
        ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px < 32 && touchReadLast().py >= SCREEN_HEIGHT - 32) ||
        (keysUp() & KEY_START)) {
        setGameState(MENU_INGAME);
        setMenuOption(2); // which is the option to go back into gameinfo
        playSoundeffect(SE_MENU_BACK);
    
    } else if ((getKeysOnUp() & KEY_TOUCH) // GUIDE key = 172,168 - 220,192
         && touchReadLast().px >= 172 && touchReadLast().px <= 220 
         && touchReadLast().py >= 168) {
        setGameState(GUIDE_GAMEINFO);
        playSoundeffect(SE_MENU_OK);
    
    } else if (updateMenuOption(4 + currentObjectivesAvailable)) {
        if (getMenuOption() == 0 && currentObjectivesAvailable) {
            setGameState(MENU_GAMEINFO_OBJECTIVES);
            playSoundeffect(SE_MENU_OK);
        } else if (getMenuOption() == 0 + currentObjectivesAvailable) {
            gameinfoIsStructure = 1;
            setGameState(MENU_GAMEINFO_ITEM_SELECT);
            playSoundeffect(SE_MENU_OK);
        } else if (getMenuOption() == 1 + currentObjectivesAvailable) {
            gameinfoIsStructure = 0;
            setGameState(MENU_GAMEINFO_ITEM_SELECT);
            playSoundeffect(SE_MENU_OK);
        } else if (getMenuOption() == 2 + currentObjectivesAvailable) {
            gameinfoIsStructure = 1;
            setGameState(MENU_GAMEINFO_TECHTREE);
            playSoundeffect(SE_MENU_OK);
        } else if (getMenuOption() == 3 + currentObjectivesAvailable) {
            gameinfoIsStructure = 0;
            setGameState(MENU_GAMEINFO_TECHTREE);
            playSoundeffect(SE_MENU_OK);
        }
    }
}



void loadMenuGameinfoGraphics(enum GameState oldState) {
    int offset = 0;
    
    if ((oldState < MENU_INGAME) || (oldState==GUIDE_GAMEINFO)) {
        copyFileVRAM(SPRITE_PALETTE_SUB, "menu_bits", FS_PALETTES);
        offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "menu_bit_font", FS_MENU_GRAPHICS);
        offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "menu_bit_bar", FS_MENU_GRAPHICS);
        copyFileVRAM(SPRITE_GFX_SUB + ((8704+4096)>>1), "menu_bit_back", FS_MENU_GRAPHICS);
    }
    
    copyFileVRAM(BG_PALETTE_SUB, "menu_gameinfo", FS_PALETTES);
    copyFileVRAM(BG_GFX_SUB, "menu_gameinfo", FS_MENU_GRAPHICS);
}



int initMenuGameinfo() {
    FILE *fp;
    char filename[256];
    
    sprintf(filename, "scenario_%s_level%i_region%i", factionInfo[getFaction(FRIENDLY)].name, getLevel(), getRegion());
    fp = openFile(filename, FS_GAMEINFO_GRAPHICS);
    if (fp) {
        currentObjectivesAvailable = 1;
        closeFile(fp);
    } else
        currentObjectivesAvailable = 0;
    
    REG_DISPCNT_SUB = ( MODE_3_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT_SUB = BG_BMP8_256x256;
    
    lcdMainOnTop();

    playMiscSoundeffect("menu_gameinfo");
    
    return 0;
}
