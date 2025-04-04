// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "ingame_briefing.h"

#include "animation.h"
#include "infoscreen.h"
#include "view.h"
#include "sprites.h"
#include "playscreen.h"


struct IngameBriefing {
    enum IngameBriefingState state;
    char file[256];
    int  delay;
};

struct IngameBriefing ingameBriefing;



enum IngameBriefingState getIngameBriefingState() {
    return ingameBriefing.state;
}

void drawIngameBriefing() {
    if (ingameBriefing.state != IBS_ACTIVE)
        return;
    
    REG_BG0VOFS_SUB = -getScreenScrollingInfoScreenShort();
    REG_BG3Y_SUB = -(getScreenScrollingInfoScreen() * 256);
    drawAnimation();
}

void doIngameBriefingLogic() {
    // count down delay if an ingame briefing is indeed queued up
    if (ingameBriefing.state == IBS_QUEUED && ingameBriefing.delay > 0) {
        if (--ingameBriefing.delay == 0)
            startIngameBriefing(ingameBriefing.file);
        return;
    }
    
    if (ingameBriefing.state != IBS_ACTIVE)
        return;
    
    doScreenSetUpLogic();
    if (doAnimationLogic()) {
        // the animation stopped, time to return to normal
        REG_MASTER_BRIGHT_SUB = (2<<14) | 16; // first mask screen by making it fully black
        
        ingameBriefing.state = IBS_INACTIVE;
        
        loadInfoScreenGraphics();
        drawInfoScreenBG();
        
        drawGame(); // draw the new screen once already to get rid of any glitches before showing it to the user
        swiWaitForVBlank();
        updateOAMafterVBlank();
        
        REG_MASTER_BRIGHT_SUB = 0; // make sure to deactivate any potential fade
    }
}


void startIngameBriefing(char *animation) {
    ingameBriefing.state = IBS_ACTIVE; // required for initAnimation to set delay input correctly
    ingameBriefing.state = (initAnimation(animation, 1, 1, 0) == 0) ? IBS_ACTIVE : IBS_INACTIVE;
    if (ingameBriefing.state != IBS_ACTIVE)
        return;
    
//    if (getScreenSetUp() == SUB_TOUCH || getScreenSetUp() == MAIN_TO_SUB_TOUCH)
//        setScreenSetUp(SUB_TO_MAIN_TOUCH);
    setScreenSetUp(MAIN_TOUCH);
    
    REG_MASTER_BRIGHT_SUB = 16 | (2<<14);
    initSpritesUsedInfoScreen();
    loadAnimationGraphics(INGAME);
    drawAnimationBG();
    
    drawPlayScreen(); // has to occur once before the animation kicks in, not during the animation itself
    
    drawGame(); // draw the new screen once already to get rid of any glitches before showing it to the user
    swiWaitForVBlank();
    updateOAMafterVBlank();
    
    REG_MASTER_BRIGHT_SUB = 0;
}

void startIngameBriefingWithDelay(char *animation, int delay) { // delay measured in frames
    if (delay == 0) {
        startIngameBriefing(animation);
        return;
    }
    
    strcpy(ingameBriefing.file, animation);
    ingameBriefing.delay = delay;
    ingameBriefing.state = IBS_QUEUED;
}

void initIngameBriefing() {
    ingameBriefing.state = IBS_INACTIVE;
    ingameBriefing.delay = 0;
}

/* // cannot pause (and therefore not save) when an ingame briefing is playing
inline struct IngameBriefing *getIngameBriefing() {
    return &ingameBriefing;
}

int getIngameBriefingSaveSize(void) {
    return sizeof(ingameBriefing);
}

int getIngameBriefingSaveData(void *dest, int max_size) {
    int size=sizeof(ingameBriefing);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&ingameBriefing,size);
    return (size);
}
*/
