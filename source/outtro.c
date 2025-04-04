// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "outtro.h"

#include <string.h>

#include "factions.h"
#include "animation.h"


void drawOuttro() {
    drawAnimation();
}

void drawOuttroBG() {
    drawAnimationBG();
}

void doOuttroLogic() {
    if (doAnimationLogic())
        setGameState(MENU_MAIN);
}

void loadOuttroGraphics(enum GameState oldState) {
    loadAnimationGraphics(oldState);
}

int initOuttro() {
    char filename[256];
    
    strcpy(filename, factionInfo[getFaction(FRIENDLY)].name);
    strcat(filename, "_outtro");
    if (initAnimation(filename, 0, 0, 0)) {
        setGameState(MENU_MAIN);
        return 1;
    }
    return 0;
}
