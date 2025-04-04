// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "gallery.h"

#include <string.h>

#include "factions.h"
#include "animation.h"

void doGalleryLogic() {
    if (doAnimationLogic())
        setGameState(MENU_MAIN);
}

int initGallery() {

    // 1 means KO here
    // 0 means OK here
    if (initAnimation ("gallery",0,0,0)) {
        setGameState(getGameState()); // go back to where we started
        return 1;
    }
    
    // turn off sprites on main screen, or the menu will stay there!
    initSpritesUsedPlayScreen();
    
    return 0;
}

void drawGallery() {
    drawAnimation();
}

void loadGalleryGraphics(enum GameState oldState) {
    loadAnimationGraphics(oldState);
}

void drawGalleryBG() {
    drawAnimationBG();
}
