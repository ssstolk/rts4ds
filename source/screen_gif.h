// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _SCREEN_GIF_H_
#define _SCREEN_GIF_H_

#include "fileio.h"
#include "gif/gif.h"

typedef struct 
{
    pGifHandler gifHandler;
    unsigned int options;
    unsigned int timer;
    unsigned int firstFrame;
    char filename[256];
    enum FileType filetype;
} tScreenGifHandler, *pScreenGifHandler;

#define SGO_SCREEN_MAIN     BIT(0)
#define SGO_SCREEN_SUB      0
#define SGO_REPEAT          BIT(1)

void drawScreenGif(pScreenGifHandler screenGifHandler);
void drawScreenGifBG(pScreenGifHandler screenGifHandler);
void doScreenGifLogic(pScreenGifHandler screenGifHandler);

pScreenGifHandler initScreenGif(char *filename, enum FileType filetype, unsigned int options);
void screenGifHandlerDestroy(pScreenGifHandler screenGifHandler);

#endif
