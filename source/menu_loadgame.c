// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "menu_loadgame.h"

#include "savegame.h"
#include "soundeffects.h"
#include "animation.h"
#include "factions.h"
#include "shared.h"

#define SAVEGAME_EMPTY 0xFF

int sel_mlg;
struct SavegameHeader_info sg_info_mlg[3];


static char my_toupper(char c) {
    if (c >= 'a' && c <= 'z') c = 'A' + (c - 'a');
    return c;
}

void drawMenuLoadGameCaption(char *string, int x, int y) {
    // this has been stolen from infoscreen.c and manipulated to the needs here
    while (*string != 0) { // 0 being end of string
        if (*string == ' ' || *string == '_') {
            x += 4;
        } else {
            setSpritePlayScreen(y, ATTR0_TALL,
                                x, SPRITE_SIZE_8, 0, 0,
                                0, 0,(my_toupper(*string)-'A')*2);
            x += 8; // spacing
        }
        string++;
    }
}

void drawMenuLoadGame() {
    initSpritesUsedPlayScreen();
/*
    setSpritePlayScreen(SCREEN_HEIGHT - 32, ATTR0_SQUARE, // the "back" sprite
                        0, SPRITE_SIZE_32, 0, 0,
                        0, 0, (8704+4096)/(8*8));
*/

    setSpritePlayScreen(72, ATTR0_SQUARE,          // the "slotselect" sprite
                        16+sel_mlg*(64+16), SPRITE_SIZE_64, 0, 0,
                        0, 0, 4608/(8*8));        // after the font: DKez style ;p

    
    // render savegame info
    if (sel_mlg!=SAVEGAME_EMPTY) {
        struct SavegameHeader_info *sg_info = &sg_info_mlg[sel_mlg];
		if (sg_info->level==SAVEGAME_EMPTY) {
            // print FACTION name
            drawMenuLoadGameCaption("none",116,152);
            // level
            setSpritePlayScreen(165, ATTR0_TALL,
                                102, SPRITE_SIZE_8, 0, 0,
                                0, 0, 0x34);
            // set other info to zero
            sg_info->map = 0;
            sg_info->date_year   = 0;
            sg_info->date_month  = 0;
            sg_info->date_day    = 0;
            sg_info->date_hour   = 0;
            sg_info->date_minute = 0;
        } else {
            // print FACTION name
            drawMenuLoadGameCaption(factionInfo[sg_info->faction].name,116,152);
            
            // level
            setSpritePlayScreen(165, ATTR0_TALL,
                                102, SPRITE_SIZE_8, 0, 0,
                                0, 0, 0x34+sg_info->level*2);
        }
        // map
        if (sg_info->map<10) {
            setSpritePlayScreen(165, ATTR0_TALL,
                                144, SPRITE_SIZE_8, 0, 0,
                                0, 0, 0x34+sg_info->map*2);
        } else {
            setSpritePlayScreen(165, ATTR0_TALL,
                                144, SPRITE_SIZE_8, 0, 0,
                                0, 0, 0x34+(sg_info->map/10)*2);
            setSpritePlayScreen(165, ATTR0_TALL,
                                144+8, SPRITE_SIZE_8, 0, 0,
                                0, 0, 0x34+(sg_info->map%10)*2);
        }
        // year 20xx
        setSpritePlayScreen(178, ATTR0_TALL,
                            60, SPRITE_SIZE_8, 0, 0,
                            0, 0, 0x34+2*2);
        setSpritePlayScreen(178, ATTR0_TALL,
                            60+8, SPRITE_SIZE_8, 0, 0,
                            0, 0, 0x34+0*2);
        setSpritePlayScreen(178, ATTR0_TALL,
                            60+16, SPRITE_SIZE_8, 0, 0,
                            0, 0, 0x34+(sg_info->date_year/10)*2);
        setSpritePlayScreen(178, ATTR0_TALL,
                            60+24, SPRITE_SIZE_8, 0, 0,
                            0, 0, 0x34+(sg_info->date_year%10)*2);

        // month
        setSpritePlayScreen(178, ATTR0_TALL,
                            98, SPRITE_SIZE_8, 0, 0,
                            0, 0, 0x34+(sg_info->date_month/10)*2);
        setSpritePlayScreen(178, ATTR0_TALL,
                            98+8, SPRITE_SIZE_8, 0, 0,
                            0, 0, 0x34+(sg_info->date_month%10)*2);

        // day
        setSpritePlayScreen(178, ATTR0_TALL,
                            120, SPRITE_SIZE_8, 0, 0,
                            0, 0, 0x34+(sg_info->date_day/10)*2);
        setSpritePlayScreen(178, ATTR0_TALL,
                            120+8, SPRITE_SIZE_8, 0, 0,
                            0, 0, 0x34+(sg_info->date_day%10)*2);


        // hour
        setSpritePlayScreen(178, ATTR0_TALL,
                            162, SPRITE_SIZE_8, 0, 0,
                            0, 0, 0x34+(sg_info->date_hour/10)*2);
        setSpritePlayScreen(178, ATTR0_TALL,
                            162+8, SPRITE_SIZE_8, 0, 0,
                            0, 0, 0x34+(sg_info->date_hour%10)*2);

        // minute
        setSpritePlayScreen(178, ATTR0_TALL,
                            181, SPRITE_SIZE_8, 0, 0,
                            0, 0, 0x34+(sg_info->date_minute/10)*2);
        setSpritePlayScreen(178, ATTR0_TALL,
                            181+8, SPRITE_SIZE_8, 0, 0,
                            0, 0, 0x34+(sg_info->date_minute%10)*2);
    }
    
    drawAnimation();
}

void drawMenuLoadGameBG() {
    int i,y;
    struct SavegameHeader *header;
    header=malloc(sizeof(struct SavegameHeader));
    // check if malloc failed
    if (header==NULL)
        error("can't allocate temp area", "");   // TROUBLE!!!!
    
    // draw radar maps on BG
    for (i=0;i<3;i++) {
        if (saveGameLoadHeader(header,i)) {
            memcpy(&sg_info_mlg[i],&header->info,sizeof(struct SavegameHeader_info));
        } else {
            sg_info_mlg[i].level=SAVEGAME_EMPTY;
            copyFileVRAM((uint16*)header->radar.image, "menu_emptyslot", FS_MENU_GRAPHICS);
        }
        
        // copy image
        for (y=0;y<64;y++)
            memcpy(BG_GFX+(72+y)*256+(i*(64+16))+16, &(header->radar.image[y*64]), 64*2); // 32bit aligned copy
    }
    free(header);

    REG_DISPCNT |= DISPLAY_BG3_ACTIVE;
}

void drawMenuLoadGameShowInfoPanelBG() {
    int y;
    // copy image onscreen - the image is 146x48
    for (y=0;y<48;y++)
        memcpy(BG_GFX+(SCREEN_HEIGHT-48+y)*256+54, BG_GFX+SCREEN_HEIGHT*SCREEN_WIDTH+(y*146), 146*2); // 32bit aligned copy
}


void doMenuLoadGameLogic() {
    int i;
    
    if (doAnimationLogic()) // loop the animation please.
        initAnimation("menu", 1, 1, 0);

    // back to previous menu
    if ((keysUp() & KEY_B) ||
       ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px < 32 && touchReadLast().py >= SCREEN_HEIGHT - 32)) {
        setGameState(MENU_MAIN);
        playSoundeffect(SE_MENU_BACK);
        return;
    }
    
    if (keysUp() & KEY_LEFT) {
        if (sel_mlg==SAVEGAME_EMPTY) {
            drawMenuLoadGameShowInfoPanelBG();
            sel_mlg=2;
            playSoundeffect(SE_MENU_SELECT);
        } else if (sel_mlg>0) {
            sel_mlg--;
            playSoundeffect(SE_MENU_SELECT);
        }
        return;
    }
    
    if (keysUp() & KEY_RIGHT) {
        if (sel_mlg==SAVEGAME_EMPTY) {
            drawMenuLoadGameShowInfoPanelBG();
            sel_mlg=0;
            playSoundeffect(SE_MENU_SELECT);
        } else if (sel_mlg<2) {
            sel_mlg++;
            playSoundeffect(SE_MENU_SELECT);
        }
        return;
    }
    
    // confirm
    if ((keysDown() & KEY_A) ||
       ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px >(256-32) && touchReadLast().py >= SCREEN_HEIGHT - 32) || // load button touched
       ((getKeysOnUp() & KEY_TOUCH) && sel_mlg!=SAVEGAME_EMPTY && touchReadLast().py>=72 && touchReadLast().py<(72+64) && touchReadLast().px>=(16+80*sel_mlg) && touchReadLast().px<=(16+80*sel_mlg+64))) { // selected slot touched again
        if (sel_mlg==SAVEGAME_EMPTY || sg_info_mlg[sel_mlg].level==SAVEGAME_EMPTY) {
            playSoundeffect(SE_CANNOT);
        } else {
            playSoundeffect(SE_MENU_OK);
            // show LOADING message
            initSpritesUsedPlayScreen();
            updateOAMafterVBlank();
            
            copyFileVRAM(BG_GFX, "menu_loading", FS_MENU_GRAPHICS);
            for (i=0; i<2*SCREEN_REFRESH_RATE; i++)
                swiWaitForVBlank();
            if (loadGame(sel_mlg)) {                  // load savegame
                setLevelLoadedViaLevelSelect(0);
                setGameState(INGAME);
            } else {
                playSoundeffect(SE_CANNOT);
                copyFileVRAM(BG_GFX, "menu_loadfailed", FS_MENU_GRAPHICS);
                for (i=0; i<2*SCREEN_REFRESH_RATE; i++)
                    swiWaitForVBlank();
                setGameState(MENU_MAIN);
            }
        }
        return;
    }
    
    if ((getKeysOnUp() & KEY_TOUCH) && (touchReadLast().py>=72) && (touchReadLast().py<72+64)) {
        if ((touchReadLast().px>=16) && (touchReadLast().px<=64+16)) {
            if (sel_mlg==SAVEGAME_EMPTY)
                drawMenuLoadGameShowInfoPanelBG();
            sel_mlg=0;
            playSoundeffect(SE_MENU_SELECT);
        } else if ((touchReadLast().px>=16+64+16) && (touchReadLast().px<=16+64*2+16)) {
            if (sel_mlg==SAVEGAME_EMPTY)
                drawMenuLoadGameShowInfoPanelBG();
            sel_mlg=1;
            playSoundeffect(SE_MENU_SELECT);
        } else if ((touchReadLast().px>=16+64*2+16*2) && (touchReadLast().px<=16+64*3+16*2)) {
            if (sel_mlg==SAVEGAME_EMPTY)
                drawMenuLoadGameShowInfoPanelBG();
            sel_mlg=2;
            playSoundeffect(SE_MENU_SELECT);
        }
    }
}

void loadMenuLoadGameGraphics(enum GameState oldState) {
    int offset = 0;
    
    copyFileVRAM(BG_GFX, "menu_loadmenu", FS_MENU_GRAPHICS);
    // copyFileVRAM(BG_PALETTE, "menu_loadmenu", FS_PALETTES);
    BG_PALETTE[0] = 0; // show a black background if nothing is displayed
    
    // store infobox offscreen
    copyFileVRAM(BG_GFX+SCREEN_HEIGHT*SCREEN_WIDTH, "menu_infobox", FS_MENU_GRAPHICS);

    copyFileVRAM(SPRITE_PALETTE, "menu_bits", FS_PALETTES);
    offset += copyFileVRAM(SPRITE_GFX + (offset>>1), "menu_bit_font", FS_MENU_GRAPHICS);
//errorSI("offset is here:", offset); // 4608
    offset += copyFileVRAM(SPRITE_GFX + (offset>>1), "menu_bit_slotselect", FS_MENU_GRAPHICS);
}

int initMenuLoadGame() {
    REG_DISPCNT = ( MODE_3_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT = BG_BMP16_256x256;
    sel_mlg=SAVEGAME_EMPTY;
    return 0;
}
