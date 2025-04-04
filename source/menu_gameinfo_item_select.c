// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_gameinfo_item_select.h"

#include "menu_gameinfo.h"
#include "soundeffects.h"
#include "factions.h"

uint8 *menuGameInfoItemSelectPiecesGraphics;
int menuGameinfoItemSelectItemNr;

void setGameinfoItemNr(int nr) {
    menuGameinfoItemSelectItemNr = nr | BIT(31);
}

int getGameinfoItemNr() {
    int x, y;
    
    if (menuGameinfoItemSelectItemNr & BIT(31))
        return menuGameinfoItemSelectItemNr;
    
    if (getGameinfoIsStructure()) {
        x = 24 + (menuGameinfoItemSelectItemNr / 6 == 0 && menuGameinfoItemSelectItemNr % 6 > 0) * 8
               + (menuGameinfoItemSelectItemNr / 6 == 0 && menuGameinfoItemSelectItemNr % 6 > 3) * 8
               + (menuGameinfoItemSelectItemNr / 6 > 0 && menuGameinfoItemSelectItemNr % 6 == 5) * 16
               + (menuGameinfoItemSelectItemNr % 6) * 32;
        y = 32 + (menuGameinfoItemSelectItemNr / 6) * (24+8);
    } else {
        x = 32 + (menuGameinfoItemSelectItemNr == 0 ) * (32*2 + 16)
               + (menuGameinfoItemSelectItemNr >= 1  && menuGameinfoItemSelectItemNr <= 8 ) * (32 + ((menuGameinfoItemSelectItemNr - 1 ) % 4) * 32)
               + (menuGameinfoItemSelectItemNr >= 9  && menuGameinfoItemSelectItemNr <= 14) * (0  + ((menuGameinfoItemSelectItemNr - 9 ) % 6) * 32)
               + (menuGameinfoItemSelectItemNr >= 15 && menuGameinfoItemSelectItemNr <= 19) * (16 + ((menuGameinfoItemSelectItemNr - 15) % 5) * 32)
               + (menuGameinfoItemSelectItemNr >= 20 && menuGameinfoItemSelectItemNr <= 25) * (0  + ((menuGameinfoItemSelectItemNr - 20) % 6) * 32)
               + (menuGameinfoItemSelectItemNr == 26) * (32*2 + 16);
        y = 16 + (menuGameinfoItemSelectItemNr == 0 ) * 24*0
               + (menuGameinfoItemSelectItemNr >= 1  && menuGameinfoItemSelectItemNr <= 8 ) * (24*1 + ((menuGameinfoItemSelectItemNr - 1 ) / 4) * 24)
               + (menuGameinfoItemSelectItemNr >= 9  && menuGameinfoItemSelectItemNr <= 14) * (24*3 + ((menuGameinfoItemSelectItemNr - 9 ) / 6) * 24)
               + (menuGameinfoItemSelectItemNr >= 15 && menuGameinfoItemSelectItemNr <= 19) * (24*4 + ((menuGameinfoItemSelectItemNr - 15) / 5) * 24)
               + (menuGameinfoItemSelectItemNr >= 20 && menuGameinfoItemSelectItemNr <= 25) * (24*5 + ((menuGameinfoItemSelectItemNr - 20) / 6) * 24)
               + (menuGameinfoItemSelectItemNr == 26) * 24*6;
//        x = 32 + (menuGameinfoItemSelectItemNr % 6) * 32;
//        y = 32 + (menuGameinfoItemSelectItemNr / 6) * 24;
    }
    return menuGameInfoItemSelectPiecesGraphics[(y+1)*SCREEN_WIDTH + (x+1)] - 1;
}


void drawMenuGameinfoItemSelect() {
    int x, y;
    
    initSpritesUsedInfoScreen();
    setSpriteInfoScreen(SCREEN_HEIGHT - 32, ATTR0_SQUARE, // the "back" sprite
                        0, SPRITE_SIZE_32, 0, 0,
                        0, 0, (8704+4096)/(8*8));
    
    if (getGameinfoIsStructure()) {
        x = 24 + (menuGameinfoItemSelectItemNr / 6 == 0 && menuGameinfoItemSelectItemNr % 6 > 0) * 8
               + (menuGameinfoItemSelectItemNr / 6 == 0 && menuGameinfoItemSelectItemNr % 6 > 3) * 8
               + (menuGameinfoItemSelectItemNr / 6 > 0 && menuGameinfoItemSelectItemNr % 6 == 5) * 16
               + (menuGameinfoItemSelectItemNr % 6) * 32;
        y = 32 + (menuGameinfoItemSelectItemNr / 6) * (24+8);
    } else {
        x = 32 + (menuGameinfoItemSelectItemNr == 0 ) * (32*2 + 16)
               + (menuGameinfoItemSelectItemNr >= 1  && menuGameinfoItemSelectItemNr <= 8 ) * (32 + ((menuGameinfoItemSelectItemNr - 1 ) % 4) * 32)
               + (menuGameinfoItemSelectItemNr >= 9  && menuGameinfoItemSelectItemNr <= 14) * (0  + ((menuGameinfoItemSelectItemNr - 9 ) % 6) * 32)
               + (menuGameinfoItemSelectItemNr >= 15 && menuGameinfoItemSelectItemNr <= 19) * (16 + ((menuGameinfoItemSelectItemNr - 15) % 5) * 32)
               + (menuGameinfoItemSelectItemNr >= 20 && menuGameinfoItemSelectItemNr <= 25) * (0  + ((menuGameinfoItemSelectItemNr - 20) % 6) * 32)
               + (menuGameinfoItemSelectItemNr == 26) * (32*2 + 16);
        y = 16 + (menuGameinfoItemSelectItemNr == 0 ) * 24*0
               + (menuGameinfoItemSelectItemNr >= 1  && menuGameinfoItemSelectItemNr <= 8 ) * (24*1 + ((menuGameinfoItemSelectItemNr - 1 ) / 4) * 24)
               + (menuGameinfoItemSelectItemNr >= 9  && menuGameinfoItemSelectItemNr <= 14) * (24*3 + ((menuGameinfoItemSelectItemNr - 9 ) / 6) * 24)
               + (menuGameinfoItemSelectItemNr >= 15 && menuGameinfoItemSelectItemNr <= 19) * (24*4 + ((menuGameinfoItemSelectItemNr - 15) / 5) * 24)
               + (menuGameinfoItemSelectItemNr >= 20 && menuGameinfoItemSelectItemNr <= 25) * (24*5 + ((menuGameinfoItemSelectItemNr - 20) / 6) * 24)
               + (menuGameinfoItemSelectItemNr == 26) * 24*6;
//        x = 32 + (menuGameinfoItemSelectItemNr % 6) * 32;
//        y = 32 + (menuGameinfoItemSelectItemNr / 6) * 24;
    }
    setSpriteInfoScreen(y, ATTR0_SQUARE, // the "select" sprite
                        x, SPRITE_SIZE_32, 0, 0,
                        0, 0, (8704+4096+32*32)/(8*8));
}


void drawMenuGameinfoItemSelectBG() {
    REG_DISPCNT_SUB |= DISPLAY_BG3_ACTIVE;
}


void doMenuGameinfoItemSelectLogic() {
    int pixel;
    int i;
    
    if ((keysUp() & KEY_B) ||
        ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px < 32 && touchReadLast().py >= SCREEN_HEIGHT - 32)) {
        free(menuGameInfoItemSelectPiecesGraphics);
        setGameState(MENU_GAMEINFO);
        playSoundeffect(SE_MENU_BACK);
    } else if (keysDown() & KEY_A) {
        if (getGameinfoItemNr() >= 0) {
//          free(menuGameInfoItemSelectPiecesGraphics);
            setGameState(MENU_GAMEINFO_ITEM_INFO);
            playSoundeffect(SE_MENU_OK);
        } else {
            playSoundeffect(SE_CANNOT);
        }
    } else if (keysDown() & KEY_UP) {
        if (getGameinfoIsStructure()) {
            if (menuGameinfoItemSelectItemNr / 6 > 0) {
                menuGameinfoItemSelectItemNr -= 6;
                playSoundeffect(SE_MENU_SELECT);
            }
        } else {
            if (menuGameinfoItemSelectItemNr != 0)
                playSoundeffect(SE_MENU_SELECT);
            
            if      (menuGameinfoItemSelectItemNr == 26) menuGameinfoItemSelectItemNr -= 4;
            else if (menuGameinfoItemSelectItemNr >= 23) menuGameinfoItemSelectItemNr -= 6;
            else if (menuGameinfoItemSelectItemNr >= 20) menuGameinfoItemSelectItemNr -= 5;
            else if (menuGameinfoItemSelectItemNr >= 18) menuGameinfoItemSelectItemNr -= 5;
            else if (menuGameinfoItemSelectItemNr >= 15) menuGameinfoItemSelectItemNr -= 6;
            else if (menuGameinfoItemSelectItemNr >= 14) menuGameinfoItemSelectItemNr -= 6;
            else if (menuGameinfoItemSelectItemNr >= 10) menuGameinfoItemSelectItemNr -= 5;
            else if (menuGameinfoItemSelectItemNr >= 9 ) menuGameinfoItemSelectItemNr -= 4;
            else if (menuGameinfoItemSelectItemNr >= 5 ) menuGameinfoItemSelectItemNr -= 4;
            else if (menuGameinfoItemSelectItemNr >= 1 ) menuGameinfoItemSelectItemNr  = 0;
        }
    } else if (keysDown() & KEY_DOWN) {
//        if ((getGameinfoIsStructure() && menuGameinfoItemSelectItemNr / 6 < 3) ||
//            (!getGameinfoIsStructure() && menuGameinfoItemSelectItemNr / 6 < 4)) {
        if (getGameinfoIsStructure()) {
            if (menuGameinfoItemSelectItemNr / 6 < 3) {
                menuGameinfoItemSelectItemNr += 6;
                playSoundeffect(SE_MENU_SELECT);
            }
        } else {
            if (menuGameinfoItemSelectItemNr != 26)
                playSoundeffect(SE_MENU_SELECT);
            
            if      (menuGameinfoItemSelectItemNr == 0 ) menuGameinfoItemSelectItemNr += 2;
            else if (menuGameinfoItemSelectItemNr <= 4 ) menuGameinfoItemSelectItemNr += 4;
            else if (menuGameinfoItemSelectItemNr <= 8 ) menuGameinfoItemSelectItemNr += 5;
            else if (menuGameinfoItemSelectItemNr <= 11) menuGameinfoItemSelectItemNr += 6;
            else if (menuGameinfoItemSelectItemNr <= 14) menuGameinfoItemSelectItemNr += 5;
            else if (menuGameinfoItemSelectItemNr <= 17) menuGameinfoItemSelectItemNr += 5;
            else if (menuGameinfoItemSelectItemNr <= 19) menuGameinfoItemSelectItemNr += 6;
            else if (menuGameinfoItemSelectItemNr <= 25) menuGameinfoItemSelectItemNr  = 26;
        }
    } else if (keysDown() & KEY_LEFT) {
        if (getGameinfoIsStructure()) {
            if (menuGameinfoItemSelectItemNr % 6 > 0) {
                menuGameinfoItemSelectItemNr--;
                playSoundeffect(SE_MENU_SELECT);
            }
        } else {
            if ((menuGameinfoItemSelectItemNr > 1  && menuGameinfoItemSelectItemNr <= 4 ) ||
                (menuGameinfoItemSelectItemNr > 5  && menuGameinfoItemSelectItemNr <= 8 ) ||
                (menuGameinfoItemSelectItemNr > 9  && menuGameinfoItemSelectItemNr <= 14) ||
                (menuGameinfoItemSelectItemNr > 15 && menuGameinfoItemSelectItemNr <= 19) ||
                (menuGameinfoItemSelectItemNr > 20 && menuGameinfoItemSelectItemNr <= 25))
            { 
                menuGameinfoItemSelectItemNr--;
                playSoundeffect(SE_MENU_SELECT);
            }
        }
    } else if (keysDown() & KEY_RIGHT) {
        if (getGameinfoIsStructure()) {
            if (menuGameinfoItemSelectItemNr % 6 < 5) {
                menuGameinfoItemSelectItemNr++;
                playSoundeffect(SE_MENU_SELECT);
            }
        } else {
            if ((menuGameinfoItemSelectItemNr >= 1  && menuGameinfoItemSelectItemNr < 4 ) ||
                (menuGameinfoItemSelectItemNr >= 5  && menuGameinfoItemSelectItemNr < 8 ) ||
                (menuGameinfoItemSelectItemNr >= 9  && menuGameinfoItemSelectItemNr < 14) ||
                (menuGameinfoItemSelectItemNr >= 15 && menuGameinfoItemSelectItemNr < 19) ||
                (menuGameinfoItemSelectItemNr >= 20 && menuGameinfoItemSelectItemNr < 25))
            { 
                menuGameinfoItemSelectItemNr++;
                playSoundeffect(SE_MENU_SELECT);
            }
        }
    } else if (getKeysOnUp() & KEY_TOUCH) {
        pixel = touchReadLast().py * SCREEN_WIDTH + touchReadLast().px;
        if (menuGameInfoItemSelectPiecesGraphics[pixel] > 0) {
            if (getGameinfoItemNr() == menuGameInfoItemSelectPiecesGraphics[pixel] - 1) {
//              free(menuGameInfoItemSelectPiecesGraphics);
                setGameState(MENU_GAMEINFO_ITEM_INFO);
                playSoundeffect(SE_MENU_OK);
            } else {
                for (i=0; i<6*5; i++) {
                    menuGameinfoItemSelectItemNr = i;
                    if (getGameinfoItemNr() == menuGameInfoItemSelectPiecesGraphics[pixel] - 1)
                        break;
                }
                playSoundeffect(SE_MENU_SELECT);
            }
        }
    }
}


void loadMenuGameinfoItemSelectGraphics(enum GameState oldState) {
    char filename[256];
    char *charPos;
    int i;
    
    strcpy(filename, "gameinfo_");
    if (getGameinfoIsStructure())
        strcat(filename, "structures_");
    else
        strcat(filename, "units_");
    strcat(filename, factionInfo[getFaction(FRIENDLY)].name);
    copyFileVRAM(BG_PALETTE_SUB, filename, FS_PALETTES);
    
    charPos = filename + strlen(filename);
    
    for (i=getFactionTechLevel(FRIENDLY); i>0; i--) {
        sprintf(charPos, "_level%i", i);
        if (copyFileVRAM(BG_GFX_SUB, filename, FS_MENU_GRAPHICS))
            break;
    }
    if (i == 0)
        error("Could not find menu graphics:", filename);
    
    copyFileVRAM(SPRITE_GFX_SUB + ((8704+4096+32*32)>>1), "gameinfo_bit_select", FS_MENU_GRAPHICS);
    for (i=0; i<32*8/2; i++)
        (SPRITE_GFX_SUB + ((8704+4096+32*32+32*24)>>1))[i] = 0;
    
    if (oldState != MENU_GAMEINFO_ITEM_INFO) {
        menuGameInfoItemSelectPiecesGraphics = (uint8 *) malloc(256*192);
        if (!menuGameInfoItemSelectPiecesGraphics)
            error("Could not malloc in menu_gameinfo_item_select.c", "menuGameInfoItemSelectPiecesGraphics's 256*192");
        strcat(filename, "_pieces");
        copyFile(menuGameInfoItemSelectPiecesGraphics, filename, FS_MENU_GRAPHICS);
    }
}


int initMenuGameinfoItemSelect() {
    if (getGameState() != MENU_GAMEINFO_ITEM_INFO)
        menuGameinfoItemSelectItemNr = 0;
    
    REG_DISPCNT_SUB = ( MODE_3_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT_SUB = BG_BMP8_256x256;
    
    lcdMainOnTop();
    
    playMiscSoundeffect("menu_gameinfo_item_select");
    
    return 0;
}
