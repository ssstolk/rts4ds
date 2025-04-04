// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _GALLERY_H_
#define _GALLERY_H_

#include "game.h"

void doGalleryLogic();
int initGallery();
void drawGallery();
void drawGalleryBG();
void loadGalleryGraphics(enum GameState oldState);

#endif
