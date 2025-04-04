// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "cutscene_briefing.h"

#include "animation.h"
#include "fileio.h"


static char animationFilename[256];




void drawCutsceneBriefing() {
    drawAnimation();
}



void drawCutsceneBriefingBG() {
    drawAnimationBG();
}



void doCutsceneBriefingLogic() {
    if (doAnimationLogic())
        setGameState(INGAME);
}



void loadCutsceneBriefingGraphics(enum GameState oldState) {
    loadAnimationGraphics(oldState);
}



int initCutsceneBriefing() {
    if (initAnimation(animationFilename, 0, 0, 0)) {
        setGameState(INGAME);
        return 1;
    }
    return 0;
}



void initCutsceneBriefingFilename() {
    char oneline[256];
    FILE *fp;
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    // CUTSCENES section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[CUTSCENES]", strlen("[CUTSCENES]")));
    readstr(fp, oneline);
    closeFile(fp);
    // Briefing line here
    replaceEOLwithEOF(oneline, 255);
    strcpy(animationFilename, oneline + strlen("Briefing="));
}
