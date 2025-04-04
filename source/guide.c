// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "guide.h"

#include <string.h>

#include "factions.h"
#include "animation.h"

void doGuideLogic() {
    if (doAnimationLogic()) {       // if true then it means to exit
        switch (getGameState()) {
            case GUIDE_INGAME:setGameState(MENU_INGAME);
                              break;
            case GUIDE_MAIN:setGameState(MENU_MAIN);
                            break;
            case GUIDE_GAMEINFO:setGameState(MENU_GAMEINFO);
                                break;
            default:break;   // to suppress warnings
        }
    }
}

int initGuide(int keepMainFree) {

    // 1 means KO here
    // 0 means OK here
    if (initAnimation ("guide",keepMainFree,0,0)) {
        setGameState(getGameState()); // go back to where we started
        return 1;
    }
    
    // turn off sprites on main screen, or the menu will stay there!
    if (!keepMainFree)
      initSpritesUsedPlayScreen();

    return 0;
}

void drawGuide() {
    drawAnimation();
}

void loadGuideGraphics(enum GameState oldState) {
    loadAnimationGraphics(oldState);
}

void drawGuideBG() {
    drawAnimationBG();
}
