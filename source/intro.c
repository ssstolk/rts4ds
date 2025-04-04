// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "intro.h"

#include "animation.h"


void drawIntro() {
    drawAnimation();
}

void drawIntroBG() {
    drawAnimationBG();
}

void doIntroLogic() {
    if (doAnimationLogic())
        setGameState(MENU_MAIN);
}

void loadIntroGraphics(enum GameState oldState) {
    loadAnimationGraphics(oldState);
}

int initIntro() {
    if (initAnimation("intro", 0, 0, 0)) {
        setGameState(MENU_MAIN);
        return 1;
    }
    return 0;
}
