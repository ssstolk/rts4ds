// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "debug.h"

#include <nds.h>
#include <nds/arm9/console.h>
#include <stdio.h>
#include <stdlib.h>


#include "game.h"
void errorSaveGameMenu() {
    setGameState(MENU_INGAME);
    setGameState(MENU_SAVEGAME);
    for (;;) {
        drawGame();
        doGameLogic();
        swiWaitForVBlank();
        updateOAMafterVBlank();
    }
}


void _initErrorConsole() {
    REG_MASTER_BRIGHT_SUB = 0;

    REG_DISPCNT_SUB = (MODE_0_2D | DISPLAY_BG0_ACTIVE); //sub bg 0 will be used to print text
    REG_BG0CNT_SUB  = BG_MAP_BASE(31);
    BG_PALETTE_SUB[0] = RGB15(0, 0, 0);
    BG_PALETTE_SUB[255] = RGB15(31,31,31);//by default font will be rendered with color 255
    //consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);
    consoleDemoInit();
}

void _finishErrorConsole() {
    iprintf("\n\n"
            "Press Select to reset the DS.\n"
            "Press Start to be able to save \nthe ingame state.");
    
    irqInit();
    irqEnable(IRQ_VBLANK);
    for(;;) {
        scanKeys();
        if (keysUp() & KEY_SELECT)
            swiSoftReset();
        if (getGameState() == INGAME && (keysUp() & KEY_START))
            errorSaveGameMenu();
        swiWaitForVBlank();
    }
    exit(0);
}


void breakpointSI(char *string, int nr) {
    uint32 old_sub_display_cr = REG_DISPCNT_SUB;
    uint16 old_sub_bg0_cr = REG_BG0CNT_SUB;
    uint16 old_bg_palette_sub_0 = BG_PALETTE_SUB[0];
    uint16 old_bg_palette_sub_255 = BG_PALETTE_SUB[255];

    REG_MASTER_BRIGHT_SUB = 0;
    
    REG_DISPCNT_SUB = (MODE_0_2D | DISPLAY_BG0_ACTIVE); //sub bg 0 will be used to print text
    REG_BG0CNT_SUB  = BG_MAP_BASE(31);
    BG_PALETTE_SUB[0] = RGB15(0, 0, 0);
    BG_PALETTE_SUB[255] = RGB15(31,31,31);//by default font will be rendered with color 255
    //consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);
    consoleDemoInit();
    iprintf("%s\n%i", string, nr);
    
    iprintf("\n\n"
            "Press A to proceed.");
    
    for (;;) {
        scanKeys();
        if (keysDown() & KEY_A) {
            REG_DISPCNT_SUB = old_sub_display_cr;
            REG_BG0CNT_SUB  = old_sub_bg0_cr;
            BG_PALETTE_SUB[0] = old_bg_palette_sub_0;
            BG_PALETTE_SUB[255] = old_bg_palette_sub_255;
            break;
        }
        swiWaitForVBlank();
    }
}

void breakpoint(char *string1, char *string2) {
    uint32 old_sub_display_cr = REG_DISPCNT_SUB;
    uint16 old_sub_bg0_cr = REG_BG0CNT_SUB;
    uint16 old_bg_palette_sub_0 = BG_PALETTE_SUB[0];
    uint16 old_bg_palette_sub_255 = BG_PALETTE_SUB[255];

    REG_MASTER_BRIGHT_SUB = 0;
    
    REG_DISPCNT_SUB = (MODE_0_2D | DISPLAY_BG0_ACTIVE); //sub bg 0 will be used to print text
    REG_BG0CNT_SUB  = BG_MAP_BASE(31);
    BG_PALETTE_SUB[0] = RGB15(0, 0, 0);
    BG_PALETTE_SUB[255] = RGB15(31,31,31);//by default font will be rendered with color 255
    //consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);
    consoleDemoInit();
    iprintf("%s\n%s", string1, string2);

    iprintf("\n\n"
            "Press A to proceed.");
    
    for (;;) {
        scanKeys();
        if (keysDown() & KEY_A) {
            REG_DISPCNT_SUB = old_sub_display_cr;
            REG_BG0CNT_SUB  = old_sub_bg0_cr;
            BG_PALETTE_SUB[0] = old_bg_palette_sub_0;
            BG_PALETTE_SUB[255] = old_bg_palette_sub_255;
            break;
        }
        swiWaitForVBlank();
    }
}


void errorI4(int int1, int int2, int int3, int int4) {
    _initErrorConsole();
    iprintf("int1: %i\nint2: %i\nint3: %i\nint4: %i", int1, int2, int3, int4);
    _finishErrorConsole();
}

void errorSI(char *string, int nr) {
    _initErrorConsole();
    iprintf("%s\n%i", string, nr);
    _finishErrorConsole();
}

void error(char *string1, char *string2) {
    _initErrorConsole();
    iprintf("%s\n%s", string1, string2);
    _finishErrorConsole();
}

void *__stack_chk_guard = (void *)0xdeadfeed;

void __stack_chk_fail(void) {
    _initErrorConsole();
    fprintf(stderr, "Stack smashing detected.\n");
    _finishErrorConsole();
}
