// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "cutscene_introduction.h"

#include "animation.h"
#include "factions.h"
#include "fileio.h"


static char animationFilename[256];




void drawCutsceneIntroduction() {
    drawAnimation();
}



void drawCutsceneIntroductionBG() {
    drawAnimationBG();
}



void doCutsceneIntroductionLogic() {
    if (doAnimationLogic())
        setGameState(MENU_REGIONS);
}



void loadCutsceneIntroductionGraphics(enum GameState oldState) {
    loadAnimationGraphics(oldState);
}



int initCutsceneIntroduction() {
    if (initAnimation(animationFilename, 0, 0, 0)) {
        setGameState(MENU_REGIONS);
        return 1;
    }
    return 0;
}



void initCutsceneIntroductionFilename() {
    char oneline[256];
    FILE *fp;
    
    strcpy(oneline, "levels_");
    strcat(oneline, factionInfo[getFaction(FRIENDLY)].name);
    fp = openFile(oneline, FS_SCENARIO_FILE);
    // CUTSCENES section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[CUTSCENES]", strlen("[CUTSCENES]")));
    readstr(fp, oneline);
    closeFile(fp);
    // Introduction line here
    replaceEOLwithEOF(oneline, 255);
    strcpy(animationFilename, oneline + strlen("Introduction="));
}
