// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "screen_gif.h"

#include <nds.h>
#include <stdio.h>
#include <string.h>


void drawScreenGif(pScreenGifHandler screenGifHandler) {
    int i, j, k;
    pGifHandler gifHandler;
    uint16 *curBG;
    uint16 *curPALETTE;
    vint32 *curCX;
    vint32 *curCY;
    
    if (screenGifHandler == NULL)
        return;
    
    if (screenGifHandler->timer != 0)
        return;
    
    gifHandler = screenGifHandler->gifHandler;
    if (gifHandler == NULL)
        return;
    
    if (screenGifHandler->options & SGO_SCREEN_MAIN) {
        curBG = BG_GFX;
        curPALETTE = BG_PALETTE;
        curCX = &REG_BG3X;
        curCY = &REG_BG3Y;
        REG_DISPCNT |= DISPLAY_BG3_ACTIVE;
    } else {
        curBG = BG_GFX_SUB;
        curPALETTE = BG_PALETTE_SUB;
        curCX = &REG_BG3X_SUB;
        curCY = &REG_BG3Y_SUB;
        REG_DISPCNT_SUB |= DISPLAY_BG3_ACTIVE;
    }
    
    if (screenGifHandler->firstFrame) {
        swiWaitForVBlank();
        curPALETTE[0] = 0;
        for (i=1; i<gifHandler->colCount; i++) {
            curPALETTE[i] = gifHandler->gifPalette[i];
        }
        
        *curCX = -(((SCREEN_WIDTH - gifHandler->dimX) / 2) << 8);
        *curCY = -(((SCREEN_HEIGHT - gifHandler->dimY) / 2) << 8);
        
        for (i=0; i<gifHandler->dimY; i++) {
            for (j=0; j<gifHandler->dimX/2; j++) {
                curBG[i*256/2 + j] = ((uint16*)gifHandler->gifFrame)[i*(gifHandler->dimX/2) + j];
            }
            for (; j<256/2; j++) {
                curBG[i*256/2 + j] = 0;
            }
        }
        for (i*=256/2; i<256*192/2; i++) {
            curBG[i] = 0;
        }
    } else {
        for (i=0; i<gifHandler->dimY; i++) {
            for (j=0; j<gifHandler->dimX/2; j++) {
                k = ((uint16*)gifHandler->gifFrame)[i*(gifHandler->dimX/2) + j];
                // only draw if it's not transparent, otherwise it'd just cost precious VRAM access time
                if (k) {
                    if (!(k & 0x00FF))
                        k |= (curBG[i*256/2 + j] & 0x00FF);
                    else if (!(k & 0xFF00))
                        k |= (curBG[i*256/2 + j] & 0xFF00);
                    curBG[i*256/2 + j] = k;
                }
            }
        }
    }
    
    screenGifHandler->firstFrame = 0;
}



void drawScreenGifBG(pScreenGifHandler screenGifHandler) {
    int i;
    
    if (screenGifHandler->options & SGO_SCREEN_MAIN) {
        BG_PALETTE[0] = 0;
        for (i=0; i<256*192/2; i++)
            BG_GFX[i] = 0;
    } else {
        BG_PALETTE_SUB[0] = 0;
        for (i=0; i<256*192/2; i++)
            BG_GFX_SUB[i] = 0;
    }
}



void doScreenGifLogic(pScreenGifHandler screenGifHandler) {
    pGifHandler gifHandler;
    int i;
    
    if (screenGifHandler == NULL)
        return;
    
    gifHandler = screenGifHandler->gifHandler;
    if (gifHandler == NULL)
        return;
    
    screenGifHandler->timer++;
    if (screenGifHandler->timer >= gifHandler->gifTiming / 15) {
        memset(gifHandler->gifFrame, 0, gifHandler->dimX * gifHandler->dimY); // this ensures gifFrame will only have a "diff" stored instead of the exact frame
        i = gifHandler->frmCount;
        if (i < gifHandlerLoadNextFrame(gifHandler))
            screenGifHandler->timer = 0;
        else {
            if (i <= 1 || !(screenGifHandler->options & SGO_REPEAT)) {
                // it either was a still frame (no need to reload, would crash the engine), or repeat was not desired.
                gifHandlerDestroy(screenGifHandler->gifHandler);
                screenGifHandler->gifHandler = NULL;
            } else {
                // repeat was desired
                screenGifHandler->gifHandler = gifHandlerLoad(openFile(screenGifHandler->filename, screenGifHandler->filetype), screenGifHandler->gifHandler);
                screenGifHandler->timer = 0;
                screenGifHandler->firstFrame = 1;
            }
        }
    }
}



pScreenGifHandler initScreenGif(char *filename, enum FileType filetype, unsigned int options) {
    pScreenGifHandler screenGifHandler;
    pGifHandler gifHandler;
    
    gifHandler = gifHandlerLoad(openFile(filename, filetype), NULL);
    if (!gifHandler)
        return NULL;
    
    screenGifHandler = (pScreenGifHandler)malloc(sizeof(tScreenGifHandler));
    if (!screenGifHandler)
        return NULL;
    
    strcpy(screenGifHandler->filename, filename);
    screenGifHandler->filetype = filetype;
    
    screenGifHandler->gifHandler = gifHandler;
    
    screenGifHandler->options = options;
    screenGifHandler->timer = 0;
    screenGifHandler->firstFrame = 1;
    
    if (options & SGO_SCREEN_MAIN) {
        REG_DISPCNT = ( MODE_3_2D );
        REG_BG3CNT = BG_BMP8_256x256;
    } else {
        REG_DISPCNT_SUB = ( MODE_3_2D );
        REG_BG3CNT_SUB = BG_BMP8_256x256;
    }
    
    return screenGifHandler;
}



void screenGifHandlerDestroy(pScreenGifHandler screenGifHandler) {
    gifHandlerDestroy(screenGifHandler->gifHandler);
    free(screenGifHandler);
}
