// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include <nds.h>
#include <stdlib.h>
#include <time.h>

#include "game.h"
#include "sprites.h"
#include "soundeffects.h"
#include "music.h"
#include "fileio.h"
#include "screenshot.h"
#include "shared.h"
#include "gameticks.h"
#include "vblankcount.h"
#include "debug.h"



int main() {
    powerOn(POWER_ALL);
    
    REG_MASTER_BRIGHT     = (1<<14) | 16; // 1 for upping brightness from original.
    REG_MASTER_BRIGHT_SUB = (1<<14) | 16; // bootup seems to start off with white, so comes across more natural to force it like that here
    
    defaultExceptionHandler();
    
    lcdMainOnBottom();
    
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;                // 128 kb
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_TEXTURE_SLOT0;          // 128 kb
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;                 // 128 kb
    VRAM_D_CR = VRAM_ENABLE | VRAM_D_SUB_SPRITE;             // 128 kb
    VRAM_E_CR = VRAM_ENABLE | VRAM_E_BG_EXT_PALETTE;         //  64 kb, extended palettes for BGs
    VRAM_F_CR = VRAM_ENABLE | VRAM_F_TEX_PALETTE;            //  16 kb, palettes for 3D (sprites)
    VRAM_G_CR = VRAM_ENABLE | VRAM_G_MAIN_SPRITE;            //  16 kb
    
    // Sound setup
    soundEnable();
    
    // FAT setup
    initFileIO();
    
    // IRQ setup
//    irqInit(); // Avoid initializing IRQs. Libnds requires its set FIFO IRQs to operate!
    irqSet(IRQ_VBLANK, handleVBlankInterrupt);
    irqEnable(IRQ_VBLANK);
    
    // Seeding random number generator 
    {
        time_t unixTime = time(NULL);
        srand(-(~ unixTime) - 1);
    }
    
    startProfiling();

    // Game and 3D initialization
    initGame();
    init3DforExpandedSprites();
    
    for (;;) {
        drawGame();
        doGameLogic();
        doMusicLogic();
        swiWaitForVBlank();
        startGameticks();
        updateOAMafterVBlank();
        updateSEafterVBlank();
        
        if (getGameticks() > GAMETICKS_PER_FRAME / 10) {
            swiWaitForVBlank();
            startGameticks();
        }
        
        // make sure to keep a steady framerate ingame (by making sure nothing happens too fast)
        if (getGameState() == INGAME) {
            while (getVBlankCount() < SCREEN_REFRESH_RATE/FPS) {
                swiWaitForVBlank();
                startGameticks();
            }
        }
        
        resetVBlankCount();
        
#ifdef DEBUG_BUILD
        if (keysDown() & KEY_SELECT)
            takeScreenshot(0);
#endif
    }
    return 0;
}
