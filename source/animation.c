// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "animation.h"

#include "gif/gif.h"
#include "factions.h"
#include "subtitles.h"
#include "soundeffects.h"
#include "music.h"
#include "ingame_briefing.h"
#include "fileio.h"
#include "inputx.h"

#define ANIMATION_FADE_TIME             SCREEN_REFRESH_RATE

#define MAX_ANIMATION_OVERLAY_TILES     10
#define ANIMATION_OVERLAY_TILE_DURATION (SCREEN_REFRESH_RATE/30)
#define ANIMATION_OVERLAY_DEFAULT_BLEND_A        6
#define MAX_ANIMATION_OVERLAY_BLEND_A_DEVIATION  3
#define ANIMATION_OVERLAY_DEFAULT_BLEND_B       12
#define MAX_ANIMATION_OVERLAY_BLEND_B_DEVIATION  3

// The values below are chosen to overlap as little as possible with the ingame BGs,
// so that we don't have to reload a lot of graphics when the animation stops and
// the game logic needs us to return to INGAME state
#define ANIMATION_BG3_GRAPHICS_OFFSET   5
#define ANIMATION_BG0_GRAPHICS_OFFSET   2


enum SceneState { SCS_FADEIN, SCS_NORMAL, SCS_FADEOUT };
enum FadeType { FT_NONE, FT_WHITE=1, FT_BLACK=2 };

char animationFilename[256];
int animationKeepMainFree;
int animationKeepMusicFree;
int animationKeepSoundFree;

int amountOfScenes;
int currentScene;
int newScene;
int manualMode; // 0 or 1 => false/true
enum SceneState currentSceneState;
int fadeTimer;
int animationTimer;
int sceneFirstFrame;
enum FadeType fadeIn, fadeOut;
float currentScenePlaybackSpeed;

uint16 animationOverlayGraphics[16*16/2 * MAX_ANIMATION_OVERLAY_TILES];
int animationOverlayTimer;
int animationOverlayTiles;

int animationInputDelay;

pGifHandler animationAni = NULL;

int animationLinked = 0;



void setAnimationOverlayTile(int nr) {
    uint16* animationBG0Base = (uint16*)(CHAR_BASE_BLOCK_SUB(ANIMATION_BG0_GRAPHICS_OFFSET));
    int i;
    
    for (i=0; i<(16*16/2); i++)
        animationBG0Base[((8*8)/2) + i] = animationOverlayGraphics[nr * ((16*16)/2) + i];
}


void drawAnimation() {
    unsigned int i, j, k;
    unsigned int fadeLevel;
    uint16* animationBG3Base = (uint16*)(CHAR_BASE_BLOCK_SUB(ANIMATION_BG3_GRAPHICS_OFFSET));
    
    if (!animationKeepMainFree)
        initSpritesUsedPlayScreen();
    initSpritesUsedInfoScreen();
    
    if (currentScene == 0)
        return;
    
    if (sceneFirstFrame) {
        swiWaitForVBlank();
        drawAnimationBG();
        for (i=1; i<animationAni->colCount; i++)
            BG_PALETTE_SUB[i] = animationAni->gifPalette[i];
    }
    
    switch (currentSceneState) {
        case SCS_FADEIN:
            fadeLevel = (fadeTimer * (16+1)) / ANIMATION_FADE_TIME;
            if (fadeLevel > 16)
                fadeLevel = 16;
            REG_MASTER_BRIGHT_SUB = fadeLevel | (fadeIn<<14);
        case SCS_NORMAL:
            if (fadeTimer == ANIMATION_FADE_TIME || animationTimer == 0) {
                if (sceneFirstFrame) {
                    REG_BG3X_SUB = -(((SCREEN_WIDTH - animationAni->dimX) / 2) << 8);
                    REG_BG3Y_SUB = -(((SCREEN_HEIGHT - animationAni->dimY) / 2) << 8);
                    
                    for (i=0; i<animationAni->dimY; i++) {
                        for (j=0; j<animationAni->dimX/2; j++) {
                            animationBG3Base[i*256/2 + j] = ((uint16*)animationAni->gifFrame)[i*(animationAni->dimX/2) + j];
                        }
                        for (; j<256/2; j++) {
                            animationBG3Base[i*256/2 + j] = 0;
                        }
                    }
                    for (i*=256/2; i<256*192/2; i++) {
                        animationBG3Base[i] = 0;
                    }
                } else {
                    for (i=0; i<animationAni->dimY; i++) {
                        for (j=0; j<animationAni->dimX/2; j++) {
                            k = ((uint16*)animationAni->gifFrame)[i*(animationAni->dimX/2) + j];
                            // only draw if it's not transparent, otherwise it'd just cost precious VRAM access time
                            if (k) {
                                if (!(k & 0x00FF))
                                    k |= (animationBG3Base[i*256/2 + j] & 0x00FF);
                                else if (!(k & 0xFF00))
                                    k |= (animationBG3Base[i*256/2 + j] & 0xFF00);
                                animationBG3Base[i*256/2 + j] = k;
                            }
                        }
                    }
                }
                
                /*if (sceneFirstFrame) {
                    // make block above actual GIF colour 0 (black)
                    j = 192-animationAni->dimY)/2;
                    k = j * 256 / 2;
                    for (i=0; i<k; i++)
                        BG_GFX_SUB[i] = 0;
                    
                    // make block below actual GIF colour 0 (black)
                    j = 192-animationAni->dimY)/2;
                    k = j * 256 / 2;
                    for (i=0; i<256*192/2; i++)
                        BG_GFX_SUB[i] = 0;
                }
                
                
                for (i=0; i<animationAni->dimY; i++) {
                    for (j=0; j<animationAni->dimX/2; j++) {
                        k = ((uint16*)animationAni->gifFrame)[i*(animationAni->dimX/2) + j];
                        if (k) // if not transparent, otherwise it'd just cost precious VRAM access time
                            BG_GFX_SUB[i*256/2 + j] = k;
                    }
                }*/
                
            }
            break;
        case SCS_FADEOUT:
            fadeLevel = (fadeTimer * (16+1)) / ANIMATION_FADE_TIME;
            if (fadeLevel > 16)
                fadeLevel = 16;
            fadeLevel = 16 - fadeLevel;
            REG_MASTER_BRIGHT_SUB = fadeLevel | (fadeOut<<14);
            break;
    }
    
    if (animationOverlayTiles > 0) {
        animationOverlayTimer++;
        if (animationOverlayTimer >= animationOverlayTiles * ANIMATION_OVERLAY_TILE_DURATION)
            animationOverlayTimer = 0;
        if (animationOverlayTimer % ANIMATION_OVERLAY_TILE_DURATION == 0)
            setAnimationOverlayTile(animationOverlayTimer / ANIMATION_OVERLAY_TILE_DURATION);
        REG_BLDALPHA_SUB = ((ANIMATION_OVERLAY_DEFAULT_BLEND_A - MAX_ANIMATION_OVERLAY_BLEND_A_DEVIATION) + (rand() % (MAX_ANIMATION_OVERLAY_BLEND_A_DEVIATION*2 + 1))) |
                          (((ANIMATION_OVERLAY_DEFAULT_BLEND_B - MAX_ANIMATION_OVERLAY_BLEND_B_DEVIATION) + (rand() % (MAX_ANIMATION_OVERLAY_BLEND_B_DEVIATION*2 + 1))) << 8);
    }
    
    drawSubtitles();
    sceneFirstFrame = 0;
}

void drawAnimationBG() {
    int i;
    uint16* animationBG3Base = (uint16*)(CHAR_BASE_BLOCK_SUB(ANIMATION_BG3_GRAPHICS_OFFSET));
    
    BG_PALETTE_SUB[0] = 0; // set black as background
    for (i=0; i<256*256/2; i++)
        animationBG3Base[i] = 0;
    
    if (!animationKeepMainFree)
        REG_DISPCNT |= DISPLAY_BG3_ACTIVE;
    REG_DISPCNT_SUB |= DISPLAY_BG3_ACTIVE;
}


enum FadeType getFadeType(char *oneline) {
    char *charPosition = strchr(oneline, '=');
    if (charPosition == NULL)
        error("Could not find the type of\nthe fade in\n", "");
    if (*(charPosition + 2) == 'B' || *(charPosition + 2) == 'b')
        return FT_BLACK;
    if (*(charPosition + 2) == 'W' || *(charPosition + 2) == 'w')
        return FT_WHITE;
    return FT_NONE;
}


void doAnimationFrameSoundeffectAndSubtitles(int frame) {
    FILE *fp;
    char oneline[256];
    char *charPosition;
    int amountOfSoundsToPlay;
    int amountOfSubtitlesToShow;
    int frameRead;
    int i;
    
    if (frame == 1)
        setSubtitles("");
    
    fp = openFile(animationFilename, FS_ANIMATIONS_FILE);
    for (i=0; i<currentScene; i++) {
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[SCENE", strlen("[SCENE")));
    }
    readstr(fp, oneline); // Name
    readstr(fp, oneline); // Overlay or Fade-In
    if (oneline[0] == 'O')
        readstr(fp, oneline); // Fade-In
    readstr(fp, oneline); // Fade-Out
    readstr(fp, oneline); // Playback-Speed
    readstr(fp, oneline); // Sounds to play
    charPosition = strchr(oneline, '=');
    if (charPosition == NULL)
        error("Could not find the amount of\nsounds to play for current scene\n", "");
    sscanf(charPosition, "= %i", &amountOfSoundsToPlay);
    
    if (animationKeepSoundFree)
        i=0;
    else {
        for (i=0; i<amountOfSoundsToPlay; i++) {
            readstr(fp, oneline);
            sscanf(oneline, "Frame%i ", &frameRead);
            if (frameRead == frame) {
                charPosition = strchr(oneline, '=');
                if (charPosition == NULL)
                    error("Could not find the name of\nthe sound to play\n", "");
                replaceEOLwithEOF(charPosition+2, 255);
                playMiscSoundeffect(charPosition+2);
                i++;
                break;
            } else if (frameRead > frame) {
                i++;
                break;
            }
        }
    }
    for (; i<amountOfSoundsToPlay; i++)
        readstr(fp, oneline);
    
    if (getSubtitlesEnabled()) {
        readstr(fp, oneline); // Subtitles to show
        charPosition = strchr(oneline, '=');
        if (charPosition == NULL)
            error("Could not find the amount of\nsubs to play for current scene\n", "");
        sscanf(charPosition, "= %i", &amountOfSubtitlesToShow);
        for (i=0; i<amountOfSubtitlesToShow; i++) {
            readstr(fp, oneline);
            sscanf(oneline, "Frame%i ", &frameRead);
            if (frameRead == frame) {
                charPosition = strchr(oneline, '=');
                if (charPosition == NULL)
                    error("Could not find the subtitles", "");
                replaceEOLwithEOF(charPosition+2, 255);
                setSubtitles(charPosition+2);
                break;
            } else if (frameRead > frame)
                break;
        }
    }
    
    closeFile(fp);
}


int shutdownAnimation(int stopMusicOnLinked, int stopSoundeffectsOnLinked) {
    char oneline[256];
    FILE *fp;
    
    setSubtitles("");
//  drawAnimation();
    swiWaitForVBlank();
    updateOAMafterVBlank();
    
    if (animationAni != NULL) {
        gifHandlerDestroy(animationAni);
        animationAni = NULL;
    }
    
    fp = openFile(animationFilename, FS_ANIMATIONS_FILE);
    if (!fp) { // shouldn't occur, but ok.
        if (!animationKeepMainFree)
            REG_MASTER_BRIGHT_SUB = 0;
        if (!animationKeepMusicFree)
            stopMusic();
        if (!animationKeepSoundFree)
            stopSoundeffects();
        animationLinked = 0;
        return 1;
    }
    
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[GENERAL]", strlen("[GENERAL]")));
    readstr(fp, oneline);
    readstr(fp, oneline);
    readstr(fp, oneline);
    closeFile(fp);
    replaceEOLwithEOF(oneline, 255);
    if (oneline[0] != '[') { // a Linked animation was specified
        if (!initAnimation(oneline + strlen("Linked="), animationKeepMainFree, animationKeepMusicFree, animationKeepSoundFree)) { // Linked animation was a success
            if (stopMusicOnLinked)
                stopMusic();
            if (stopSoundeffectsOnLinked)
                stopSoundeffects();
            animationLinked = 1;
            return 0;
        }
    }
    
//    if (!animationKeepMainFree)
//        REG_MASTER_BRIGHT_SUB = 0; // setGameState() will take care of setting REG_MASTER_BRIGHT_SUB to 0
    if (!animationKeepMusicFree)
        stopMusic();
    if (!animationKeepSoundFree)
        stopSoundeffects();
    animationLinked = 0;
    return 1;
}


int doAnimationLogic() {
    char oneline[256];
    char *charPosition = NULL;
    int i;
    FILE *fp;
    
    unsigned x1, y1, x2, y2;
    char overlayName[128];
    unsigned ui, uj;
    
    uint16* animationBG0Base = (uint16*)(CHAR_BASE_BLOCK_SUB(ANIMATION_BG0_GRAPHICS_OFFSET));
    
    if (animationInputDelay > 0)
        animationInputDelay--;

    if (amountOfScenes <= 0 ||
        (animationInputDelay==0 &&
         ((!manualMode && !animationKeepMainFree && ((getKeysOnUp() & KEY_TOUCH) || (getKeysOnUp() & (KEY_A | KEY_START)))) ||
          ( manualMode && (((getKeysOnUp() & (KEY_A | KEY_RIGHT)) && (currentScene==amountOfScenes)) ||
                           ((getKeysOnUp() & KEY_TOUCH)            && (currentScene==amountOfScenes)) ||
                            ((getKeysOnUp() & (KEY_B | KEY_LEFT))  && (currentScene==1)) ||
                            (getKeysOnUp() & KEY_START))) ||
          (getGameState() == INGAME && ((getKeysOnUp() & KEY_TOUCH) || (getKeysOnUp() & (KEY_A | KEY_START))))))) {
        setKeysToIgnoreOnceOnUp(0);
        return shutdownAnimation(0, 1);
    }
    
    // guide keys
    if (manualMode && ((currentSceneState == SCS_NORMAL) || (currentSceneState == SCS_FADEIN))) {
        newScene=currentScene;
        if ((getKeysOnUp() & KEY_A) || (getKeysOnUp() & KEY_RIGHT) || (getKeysOnUp() & KEY_TOUCH))
            newScene = currentScene+1;
        if ((getKeysOnUp() & KEY_B) || (getKeysOnUp() & KEY_LEFT))
            newScene = currentScene-1;
        if ((getKeysOnUp() & KEY_L) || (getKeysOnUp() & KEY_UP))
            newScene = (currentScene>10) ? (currentScene-10) : 1;
        if ((getKeysOnUp() & KEY_R) || (getKeysOnUp() & KEY_DOWN))
            newScene = (currentScene<(amountOfScenes-10)) ? (currentScene+10) : amountOfScenes;
        
        // if scene needs to be changed
        if (newScene != currentScene) {
            if (currentSceneState == SCS_FADEIN)
                fadeTimer = (ANIMATION_FADE_TIME-fadeTimer) * (fadeOut != FT_NONE);
            else
                fadeTimer = ANIMATION_FADE_TIME * (fadeOut != FT_NONE);
            animationTimer = -1;
            currentSceneState = SCS_FADEOUT;
        }
    }
    
    if (currentSceneState == SCS_FADEIN) {
        fadeTimer--;
        if (fadeTimer == 0)
            currentSceneState = SCS_NORMAL;
    } 
    
    if (animationTimer == -1 && currentSceneState == SCS_FADEOUT) {
        fadeTimer--;
        if (fadeTimer <= 0) {
            
            if (manualMode)
                currentScene=newScene;
            else                      
                currentScene++;
            
            currentSceneState = SCS_FADEIN;
            fadeTimer = ANIMATION_FADE_TIME;
            
            if (currentScene > amountOfScenes) {
                setKeysToIgnoreOnceOnUp(0);
                return shutdownAnimation(0, 0);
            }
            
            fp = openFile(animationFilename, FS_ANIMATIONS_FILE);
            for (i=0; i<currentScene; i++) {
                do {
                    readstr(fp, oneline);
                } while (strncmp(oneline, "[SCENE", strlen("[SCENE")));
            }
            readstr(fp, oneline);
            replaceEOLwithEOF(oneline, 255);
            charPosition = strchr(oneline, '=');
            
            if (charPosition == NULL)
                error("Could not find the name of the\nnext scene", "");
            
            animationAni = gifHandlerLoad(openFile(charPosition + 2, FS_ANIMATIONS_GRAPHICS), animationAni);
            if (animationAni == NULL)
                error("Could not start up animation of", charPosition + 2);
            
            readstr(fp, oneline);
            if (oneline[0] != 'O') {
                animationOverlayTiles = 0;
                REG_DISPCNT_SUB &= (~DISPLAY_BG0_ACTIVE);
                REG_BLDCNT_SUB = BLEND_NONE;
            } else {
                replaceEOLwithEOF(oneline, 255);
                charPosition = strchr(oneline, '=');
                if (charPosition == NULL)
                    error("Could not find the settings of the overlay", "");
                if (sscanf(charPosition, "= %u,%u,%u,%u,%s", &x1, &y1, &x2, &y2, overlayName) < 5 ||
                    x2 < x1 || y2 < y1 || x2 > SCREEN_WIDTH || y2 > SCREEN_HEIGHT)
                    error("Could not use settings of the overlay", charPosition+2);
                
                for (i=0; i<(8*8)/2; i++)
                    animationBG0Base[i] = 0;
                
                animationOverlayTiles = copyFile(animationOverlayGraphics, overlayName, FS_ANIMATIONS_OVERLAY_GRAPHICS);
                setAnimationOverlayTile(0);
                // unfortunately it has to share palette with the GIF playing
                
                animationOverlayTimer = 0;
                animationOverlayTiles /= (16*16);
                if (animationOverlayTiles > MAX_ANIMATION_OVERLAY_TILES)
                    errorSI("Animation overlay tiles limit was surpassed. Limit is:", MAX_ANIMATION_OVERLAY_TILES);
                
                x1 /= 16;
                x2 /= 16;
                y1 /= 16;
                y2 /= 16;
                
                for (ui=0; ui<y1; ui++) { // blank out the first unwanted lines
                    for (uj=0; uj<2*(SCREEN_WIDTH/8); uj++)
                        mapSubBG0[ui*(2*(SCREEN_WIDTH/8)) + uj] = 0;
                }
                
                for (; ui<y2; ui++) { // fill in the overlay lines
                    for (uj=0; uj<x1; uj++) { // blank out the first unwanted columns
                        mapSubBG0[ui*(2*(SCREEN_WIDTH/8)) + 2*uj]                        = 0;
                        mapSubBG0[ui*(2*(SCREEN_WIDTH/8)) + 2*uj + 1]                    = 0;
                        mapSubBG0[ui*(2*(SCREEN_WIDTH/8)) + (SCREEN_WIDTH/8) + 2*uj]     = 0;
                        mapSubBG0[ui*(2*(SCREEN_WIDTH/8)) + (SCREEN_WIDTH/8) + 2*uj + 1] = 0;
                    }
                    for (; uj<x2; uj++) { // fill in the overlay columns
                        mapSubBG0[ui*(2*(SCREEN_WIDTH/8)) + 2*uj]                        = 1;
                        mapSubBG0[ui*(2*(SCREEN_WIDTH/8)) + 2*uj + 1]                    = 2;
                        mapSubBG0[ui*(2*(SCREEN_WIDTH/8)) + (SCREEN_WIDTH/8) + 2*uj]     = 3;
                        mapSubBG0[ui*(2*(SCREEN_WIDTH/8)) + (SCREEN_WIDTH/8) + 2*uj + 1] = 4;
                    }
                    for (; uj<(SCREEN_WIDTH / 16); uj++) { // blank out the last unwanted columns
                        mapSubBG0[ui*(2*(SCREEN_WIDTH/8)) + 2*uj]                        = 0;
                        mapSubBG0[ui*(2*(SCREEN_WIDTH/8)) + 2*uj + 1]                    = 0;
                        mapSubBG0[ui*(2*(SCREEN_WIDTH/8)) + (SCREEN_WIDTH/8) + 2*uj]     = 0;
                        mapSubBG0[ui*(2*(SCREEN_WIDTH/8)) + (SCREEN_WIDTH/8) + 2*uj + 1] = 0;
                    }
                }
                
                for (; ui<(SCREEN_HEIGHT/16); ui++) { // blank out the last unwanted lines
                    for (uj=0; uj<2*(SCREEN_WIDTH/8); uj++)
                        mapSubBG0[ui*(2*(SCREEN_WIDTH/8)) + uj] = 0;
                }
                
                REG_DISPCNT_SUB |= DISPLAY_BG0_ACTIVE;
                REG_BLDCNT_SUB = BLEND_ALPHA | BLEND_SRC_BG0 | BLEND_DST_BG3;
                
                readstr(fp, oneline);
            }
            fadeIn = getFadeType(oneline);
            readstr(fp, oneline);
            fadeOut = getFadeType(oneline);
            readstr(fp, oneline);
            charPosition = strchr(oneline, '=');
            if (charPosition == NULL)
                error("Could not find playback speed\nof the next scene\n", "");
            if (sscanf(charPosition, "= %f", &currentScenePlaybackSpeed) < 1)
                currentScenePlaybackSpeed = 1.0f;
            
            closeFile(fp);
            
            sceneFirstFrame = 1;
            
            if (fadeIn == FT_NONE) {
                currentSceneState = SCS_NORMAL;
                REG_MASTER_BRIGHT_SUB = 0;
            } else 
                REG_MASTER_BRIGHT_SUB = 16 | (fadeIn<<14);
            
            doAnimationFrameSoundeffectAndSubtitles(animationAni->frmCount);
        }
    }
    
    if (!manualMode) { // no animations in 'Manual' mode
        if (currentSceneState == SCS_NORMAL) {
            animationTimer++;
            
            if (animationAni->gifTiming == 0) // if a still frame is found, make it last 100 ms
                animationAni->gifTiming = (int) (((float) 1000) / currentScenePlaybackSpeed);
            
            if (animationTimer >= (int) (((float) (animationAni->gifTiming / 15)) / currentScenePlaybackSpeed)) {
                memset(animationAni->gifFrame, 0, animationAni->dimX * animationAni->dimY); // this ensures gifFrame will only have a "diff" stored instead of the exact frame
                i = animationAni->frmCount;
                if (i < gifHandlerLoadNextFrame(animationAni)) {
                    animationTimer = 0;
                    doAnimationFrameSoundeffectAndSubtitles(animationAni->frmCount);
                } else {
                    animationTimer = -1;
                    currentSceneState = SCS_FADEOUT;
                    fadeTimer = ANIMATION_FADE_TIME * (fadeOut != FT_NONE);
                }
            }
        }
    }
    
    return 0;
}

void loadAnimationGraphics(enum GameState oldState) {
    char filename[256];
    
    loadSubtitlesGraphics();
    
    if (animationKeepMainFree)
        return;
    
    strcpy(filename, "animation_");
    
    if (oldState <= MENU_MAIN)
        strcat(filename, "intro");
    else
        strcat(filename, factionInfo[getFaction(FRIENDLY)].name);
    
    copyFileVRAM(BG_PALETTE, filename, FS_PALETTES);
    copyFileVRAM(BG_GFX, filename, FS_MENU_GRAPHICS);
}

int initAnimation(char *filename, int keepMainFree, int keepMusicFree, int keepSoundFree) {
    char manualModeChar;
    char oneline[256];
    FILE *fp;
    int i;
    uint16* animationBG3Base;
    
    setKeysToIgnoreOnceOnUp(getKeysDown() | getKeysOnUp()); // all but the buttons which have been up for more than one frame already
    
    initSpritesUsedInfoScreen();
    swiWaitForVBlank();
    updateOAMafterVBlank();
    
    setSubtitles("");
    
    strcpy(animationFilename, filename);
    animationKeepMainFree  = keepMainFree;
    animationKeepMusicFree = keepMusicFree;
    animationKeepSoundFree = keepSoundFree;
    
    amountOfScenes = 0;
    currentScene = 0;
    newScene = 1;
    animationTimer = -1; // load up a next scene animation
    animationInputDelay = (getIngameBriefingState() == IBS_ACTIVE) ? (2*FPS) : 0; // only ingame briefing has input delayed
    
    if (animationAni != NULL) {
        gifHandlerDestroy(animationAni);
        animationAni = NULL;
    }
    
    fadeIn = FT_NONE;
    fadeOut = FT_NONE;
    currentSceneState = SCS_FADEOUT;
    fadeTimer = 0;
    sceneFirstFrame = 0;
    
    fp = openFile(animationFilename, FS_ANIMATIONS_FILE);
    if (!fp)
        return 1;
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[GENERAL]", strlen("[GENERAL]")));
    readstr(fp, oneline);
    sscanf(oneline, "Amount-Of-Scenes = %i", &amountOfScenes);
    if (amountOfScenes <= 0) {
        closeFile(fp);
        return 1;
    }
    readstr(fp, oneline);
    
    manualMode=0;
    if (!strncmp(oneline, "Manual = ", strlen("Manual = "))) {
        manualModeChar = oneline[strlen("Manual = ")];
        manualMode = (manualModeChar=='Y') || (manualModeChar=='y');
        readstr(fp, oneline); // read another line because this was present
    }
    
    closeFile(fp);
    replaceEOLwithEOF(oneline, 255);
    if (!animationKeepMusicFree) {
        if (strncmp(oneline + strlen("Music = "), "None", strlen("None"))) // !None
            playMiscMusic(oneline + strlen("Music = "), MUSIC_LOOP);
        else if (!animationLinked)
            stopMusic();
    }
    
    if (animationLinked)
        return 0;
    
    animationBG3Base = (uint16*)(CHAR_BASE_BLOCK_SUB(ANIMATION_BG3_GRAPHICS_OFFSET));

    BG_PALETTE_SUB[0] = 0; // set black as background
    for (i=0; i<256*256/2; i++)
        animationBG3Base[i] = 0;
    
    REG_DISPCNT_SUB = ( MODE_3_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT_SUB  = BG_PRIORITY_3 | BG_BMP_BASE(ANIMATION_BG3_GRAPHICS_OFFSET) | BG_BMP8_256x256;
    REG_BG3PA_SUB   = 1<<8; // scaling of 1
    REG_BG3PB_SUB   = 0;
    REG_BG3PC_SUB   = 0;
    REG_BG3PD_SUB   = 1<<8; // scaling of 1
    REG_BG3X_SUB    = 0;
    REG_BG3Y_SUB    = 0;
    REG_BG3HOFS_SUB = 0;
    REG_BG3VOFS_SUB = 0;
    REG_BG0CNT_SUB  = BG_PRIORITY_0 | BG_TILE_BASE(ANIMATION_BG0_GRAPHICS_OFFSET) | BG_COLOR_256 | BG_MAP_BASE(31) | BG_32x32;
    REG_BG0HOFS_SUB = 0;
    REG_BG0VOFS_SUB = 0;
   
    if (animationKeepMainFree)
        return 0;
    
    REG_DISPCNT = ( MODE_3_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT  = BG_BMP8_256x256;
    REG_BG3PA   = 1<<8; // scaling of 1
    REG_BG3PB   = 0;
    REG_BG3PC   = 0;
    REG_BG3PD   = 1<<8; // scaling of 1
    REG_BG3X    = 0;
    REG_BG3Y    = 0;
    REG_BG3HOFS = 0;
    REG_BG3VOFS = 0;
    
    return 0;
}
