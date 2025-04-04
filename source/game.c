// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "game.h"

#include <string.h>

#include "splash.h"
#include "menu_rts4ds.h"
#include "intro.h"
#include "outtro.h"
#include "menu_main.h"
#include "menu_continue.h"
#include "menu_settings.h"
#include "menu_soundroom.h"
#include "menu_factions.h"
#include "menu_regions.h"
#include "menu_ingame.h"
#include "menu_gameinfo.h"
#include "menu_gameinfo_objectives.h"
#include "menu_gameinfo_item_select.h"
#include "menu_gameinfo_item_info.h"
#include "menu_gameinfo_techtree.h"
#include "menu_loadgame.h"
#include "menu_savegame.h"
#include "menu_levelselect.h"
#include "cutscene_introduction.h"
#include "cutscene_debriefing.h"
#include "cutscene_briefing.h"
#include "guide.h"
#include "gallery.h"

#include "view.h"
#include "inputx.h"
#include "sprites.h"
#include "soundeffects.h"
#include "music.h"
#include "rumble.h"
#include "subtitles.h"
#include "settings.h"

#include "factions.h"
#include "environment.h"
#include "overlay.h"
#include "explosions.h"
#include "projectiles.h"
#include "structures.h"
#include "units.h"
#include "info.h"
#include "ai.h"
#include "shared.h"
#include "pathfinding.h"

#include "playscreen.h"
#include "infoscreen.h"
#include "objectives.h"
#include "timedtriggers.h"
#include "ingame_briefing.h"


enum GameState gameState;
int menuOption, menuOptionChanged;
int menuIdleTime;
int gameLevel, gameRegion;


void setGameState(enum GameState state) {
    lcdMainOnBottom();
    updateKeys(); // make sure key presses don't carry over to other states
    if (state != SPLASH) {
        if (!(state == INGAME && gameState >= MENU_INGAME && gameState <= GALLERY)) {
            if (state >= MENU_INGAME && state <= GUIDE_GAMEINFO) {
                if (getGameType() == SINGLEPLAYER) {
                    REG_MASTER_BRIGHT = (2<<14) | 8; // 2 for downing brightness from original, pause
                    lcdMainOnTop();
                }
            } else
                REG_MASTER_BRIGHT = (2<<14) | 16; // 2 for downing brightness from original
        }
        if (!(state == MENU_MAIN && (gameState == MENU_CONTINUE || gameState == MENU_SETTINGS || gameState == MENU_FACTIONS)) &&
            !(gameState == MENU_MAIN && (state == MENU_CONTINUE || state == MENU_SETTINGS || state == MENU_FACTIONS)))
            REG_MASTER_BRIGHT_SUB = (2<<14) | 16;
    }
    
    REG_BLDCNT = BLEND_NONE;
    REG_BLDCNT_SUB = BLEND_NONE;
    
    setMenuOption(0);
    menuIdleTime = 0;
    
    if (!((state >= MENU_INGAME && state < OUTTRO) || (state == INGAME && gameState == MENU_INGAME)))
        initSpritesUsedPlayScreen();
    initSpritesUsedInfoScreen();
    updateOAMafterVBlank();
    
    switch (state) {
        #ifdef DISABLE_MENU_RTS4DS
        case SPLASH:
            setPlayScreenSpritesMode(PSSM_NORMAL);
            if (initSplash()) return;
            loadSplashGraphics(gameState);
            drawSplashBG();
            stopMusic();
            break;
        #else
        case MENU_RTS4DS:
            setPlayScreenSpritesMode(PSSM_NORMAL);
            if (initMenuRts4ds()) return;
            loadMenuRts4dsGraphics(gameState);
            drawMenuRts4dsBG();
            stopMusic();
            break;
        #endif
        case INTRO:
            setPlayScreenSpritesMode(PSSM_NORMAL);
            #ifndef DISABLE_MENU_RTS4DS
            if (gameState == MENU_RTS4DS) {
                initFactions();
                initEnvironment();
                initOverlay();
                initExplosions();
                initProjectiles();
                initStructures();
                initUnits();
                initPriorityStructureAI();
                initSoundeffects();
                initSubtitles();
                #ifndef REMOVE_ASTAR_PATHFINDING
                initPathfinding();
                #endif
                readSaveFile();
                preloadPlayScreen3DGraphics();
            }
            #endif
            initMusic();
 
            #ifdef DEBUG_BUILD
            // special quick scenario load
            if (getKeysDown() & KEY_R) {
                quickLoadScenario();
                setGameState(INGAME);
                state = INGAME;
                return;
            }
			#endif
			
            if (initIntro()) return;
            loadIntroGraphics(gameState);
            drawIntroBG();
            break;
        case MENU_MAIN:
            setPlayScreenSpritesMode(PSSM_NORMAL);
            if (initMenuMain()) return;
            loadMenuMainGraphics(gameState);
            drawMenuMainBG();
            playMusic(MT_MENU, MUSIC_LOOP);
            break;
        case MENU_CONTINUE:
            setPlayScreenSpritesMode(PSSM_NORMAL);
            if (initMenuContinue()) return;
            loadMenuContinueGraphics(gameState);
            drawMenuContinueBG();
            playMusic(MT_MENU, MUSIC_LOOP);
            break;
        case MENU_LEVELSELECT:
            setPlayScreenSpritesMode(PSSM_NORMAL);
            if (initMenuLevelSelect()) return;
            loadMenuLevelSelectGraphics(gameState);
            drawMenuLevelSelectBG();
            playMusic(MT_MENU, MUSIC_LOOP);
            break;
        case MENU_SETTINGS:
            setPlayScreenSpritesMode(PSSM_NORMAL);
            if (initMenuSettings()) return;
            loadMenuSettingsGraphics(gameState);
            drawMenuSettingsBG();
            playMusic(MT_MENU, MUSIC_LOOP);
            break;
        case MENU_SOUNDROOM:
            setPlayScreenSpritesMode(PSSM_NORMAL);
            if (initMenuSoundroom()) return;
            loadMenuSoundroomGraphics(gameState);
            drawMenuSoundroomBG();
            stopMusic();
            break;
        case MENU_FACTIONS:
            setPlayScreenSpritesMode(PSSM_NORMAL);
            if (initMenuFactions()) return;
            loadMenuFactionsGraphics(gameState);
            drawMenuFactionsBG();
            playMusic(MT_MENU, MUSIC_LOOP);
            break;
        case MENU_REGIONS:
            setPlayScreenSpritesMode(PSSM_NORMAL);
            if (initMenuRegions()) return;
            loadMenuRegionsGraphics(gameState);
            drawMenuRegionsBG();
            playMusic(MT_MENU_FACTION, MUSIC_LOOP);
            break;
        case CUTSCENE_INTRODUCTION:
            setPlayScreenSpritesMode(PSSM_NORMAL);
            if (initCutsceneIntroduction()) return;
            loadCutsceneIntroductionGraphics(gameState);
            drawCutsceneIntroductionBG();
            break;
        case CUTSCENE_DEBRIEFING:
            setPlayScreenSpritesMode(PSSM_NORMAL);
            if (initCutsceneDebriefing()) return;
            loadCutsceneDebriefingGraphics(gameState);
            drawCutsceneDebriefingBG();
            break;
        case CUTSCENE_BRIEFING:
            setPlayScreenSpritesMode(PSSM_NORMAL);
            if (initCutsceneBriefing()) return;
            loadCutsceneBriefingGraphics(gameState);
            drawCutsceneBriefingBG();
            break;
        case INGAME:
            if (gameState == MENU_LOADGAME ||
                (gameState < MENU_INGAME || gameState > MENU_GAMEINFO_TECHTREE)) {
                setPlayScreenSpritesMode(PSSM_EXPANDED);
            }
            
            if (gameState != MENU_LOADGAME &&
                (gameState < MENU_INGAME || gameState > MENU_GAMEINFO_TECHTREE)) {
                delayNewRumble();
                
                initIngameBriefing();
                initView(HORIZONTAL, 0, 0);
                initPlayScreen();
                initInfoScreen();
                
                initTimedtriggers();
                initObjectives();
                initMusicWithScenario();
            }
            
            if (gameState == MENU_LOADGAME ||
                (gameState < MENU_INGAME || gameState > MENU_GAMEINFO_TECHTREE)) {
                loadPlayScreenGraphics();
                drawPlayScreenBG();
            }
            loadInfoScreenGraphics();
            drawInfoScreenBG();
            
            if (gameState >= MENU_INGAME && gameState <= MENU_GAMEINFO_TECHTREE && getScreenSetUp() == SUB_TOUCH)
                lcdMainOnTop();
            
            if (gameState == MENU_LOADGAME ||
                (gameState < MENU_INGAME || gameState > MENU_GAMEINFO_TECHTREE)) {
                executeWaitingRumble();
            }
            
            playMusic(MT_INGAME, MUSIC_ONE_SHOT);
            break;
        case MENU_INGAME:
            if (initMenuIngame()) return;
            loadMenuIngameGraphics(gameState);
            drawMenuIngameBG();
            playMusic(MT_MENU_FACTION, MUSIC_LOOP);
            break;
        case MENU_GAMEINFO:
            if (initMenuGameinfo()) return;
            loadMenuGameinfoGraphics(gameState);
            drawMenuGameinfoBG();
            // restart game music if we're coming back from GUIDE
            if (gameState==GUIDE_GAMEINFO)
                playMusic(MT_INGAME, MUSIC_ONE_SHOT);
            break;
        case MENU_GAMEINFO_OBJECTIVES:
            if (initMenuGameinfoObjectives()) return;
            loadMenuGameinfoObjectivesGraphics(gameState);
            drawMenuGameinfoObjectivesBG();
            break;
        case MENU_GAMEINFO_ITEM_SELECT:
            if (initMenuGameinfoItemSelect()) return;
            loadMenuGameinfoItemSelectGraphics(gameState);
            drawMenuGameinfoItemSelectBG();
            break;
        case MENU_GAMEINFO_ITEM_INFO:
            if (initMenuGameinfoItemInfo()) return;
            loadMenuGameinfoItemInfoGraphics(gameState);
            drawMenuGameinfoItemInfoBG();
            break;
        case MENU_GAMEINFO_TECHTREE:
            if (initMenuGameinfoTechtree()) return;
            loadMenuGameinfoTechtreeGraphics(gameState);
            drawMenuGameinfoTechtreeBG();
            break;
        case MENU_LOADGAME:
            if (initMenuLoadGame()) return;
            loadMenuLoadGameGraphics(gameState);
            drawMenuLoadGameBG();
            break;
        case MENU_SAVEGAME:
            if (initMenuSaveGame()) return;
            loadMenuSaveGameGraphics(gameState);
            drawMenuSaveGameBG();
            break;
        case GUIDE_INGAME:
        case GUIDE_GAMEINFO:
        case GUIDE_MAIN:
            if (initGuide(state!=GUIDE_MAIN)) return;  // keep main free if not GUIDE_MAIN
            loadGuideGraphics(gameState);
            drawGuideBG();
            break;
        case GALLERY:
            if (initGallery()) return;
            loadGalleryGraphics(gameState);
            drawGalleryBG();
            break;
        case OUTTRO:
            setPlayScreenSpritesMode(PSSM_NORMAL);
            if (initOuttro()) return;
            loadOuttroGraphics(gameState);
            drawOuttroBG();
            break;
        default:
            break;
    }
    gameState = state;
    
    drawGame(); // draw the new screen once already to get rid of any glitches before showing it to the user
    swiWaitForVBlank();
    updateOAMafterVBlank();
        
    if (state != SPLASH) {
        if (!(state >= MENU_INGAME && state < GUIDE_MAIN && getGameType() == SINGLEPLAYER))
            REG_MASTER_BRIGHT = 0;
        REG_MASTER_BRIGHT_SUB = 0;
    }
}

enum GameState getGameState() { // might not return truthful gameState due to setGameState() still being busy, awkward. xD
    return gameState;
}


void drawCaptionGame(char *string, int x, int y, int mainScreen) {
    int i=0;
    int curChar;
    
    while (*string != 0) { // 0 being end of string
        i++;
        if (*string == ' ' || *string == '_') {
            x += 8;
        } else {
            curChar = ((int)*string);
            if (curChar >= 48 && curChar <= 57) { // number
                if (mainScreen)
                    setSpritePlayScreen(y, ATTR0_TALL,
                                        x, SPRITE_SIZE_8, 0, 0,
                                        0, 0, 2*26 + 2*(curChar-48));
                else
                    setSpriteInfoScreen(y, ATTR0_TALL,
                                        x, SPRITE_SIZE_8, 0, 0,
                                        0, 0, 2*26 + 2*(curChar-48));
                x += 6;
            } else { // letter
                if (curChar < 97) // capital letter
                    curChar -= 65;
                else // small letter
                    curChar -= 97;
                if (mainScreen)
                    setSpritePlayScreen(y, ATTR0_TALL,
                                        x, SPRITE_SIZE_8, 0, 0,
                                        0, 0, 2*curChar);
                else
                    setSpriteInfoScreen(y, ATTR0_TALL,
                                        x, SPRITE_SIZE_8, 0, 0,
                                        0, 0, 2*curChar);
                
                curChar += 97;
                if (curChar == 'i')
                    x += 1;
                else if (curChar == 'm' || curChar == 'w')
                    x += 7;
                else
                    x += 6;
            }
            x += 2; // spacing
        }
        string++;
    }
}

void drawMenuOption(int nr, char *name, int mainScreen) {
    int i;
    static int frame=0;
    
    drawCaptionGame(name, 21, 48 + 3 + nr*24, mainScreen);
    if (nr == menuOption) {
        if (mainScreen) {
            for (i=0; i<8; i++)
                setSpritePlayScreen(48 + nr*24, ATTR0_WIDE,
                                    i*32, SPRITE_SIZE_32, 0, 0,
                                    0, 0, (26+10)*2 + 8*i + 64*(frame<(SCREEN_REFRESH_RATE/4)));
        } else {
            for (i=0; i<8; i++)
                setSpriteInfoScreen(48 + nr*24, ATTR0_WIDE,
                                    i*32, SPRITE_SIZE_32, 0, 0,
                                    0, 0, (26+10)*2 + 8*i + 64*(frame<(SCREEN_REFRESH_RATE/4)));
        }
        frame=(frame+1)%(SCREEN_REFRESH_RATE/2); // reset counter every half second
    }
}

int getMenuIdleTime() {
    return menuIdleTime;
}

void setMenuOption(int nr) {
    menuOption = nr;
    menuOptionChanged = 0;
}

int getMenuOption() {
    return menuOption;
}

int updateMenuOption(int optionAmount) {
    static int lastMenuOptionTouched = -1;
    int i, j;
    
    if (keysDown() & KEY_DOWN) {
        if (menuOption + 1 < optionAmount) {
            menuOption++;
            menuOptionChanged = 0;
            playSoundeffect(SE_MENU_SELECT);
        }
        menuIdleTime = 0;
    } else if (keysDown() & KEY_UP) {
        if (menuOption > 0) {
            menuOption--;
            menuOptionChanged = 0;
            playSoundeffect(SE_MENU_SELECT);
        }
        menuIdleTime = 0;
    } else if (getKeysDown() & KEY_TOUCH) {
        lastMenuOptionTouched = -1;
        if (touchReadLast().py >= 48) {
            i = touchReadLast().py - 48;
            for (j=0; j<optionAmount; j++) {
                if (i < 0)
                    break;
                if (i < 16) {
                    if (menuOption != j) {
                        menuOption = j;
                        menuOptionChanged = 1;
                        playSoundeffect(SE_MENU_SELECT);
                    }
                    lastMenuOptionTouched = j;
                    break;
                }
                i -= 24;
            }
        }
        menuIdleTime = 0;
    } else if (getKeysOnUp() & KEY_TOUCH) {
        if (!menuOptionChanged && lastMenuOptionTouched != -1)
            return 1;
        menuOptionChanged = 0;
        menuIdleTime = 0;
    } else if (menuIdleTime < 300*SCREEN_REFRESH_RATE)
        menuIdleTime++;
    if (keysDown() & KEY_A)
        return 1;
    
    return 0;
}

void setRegion(int region) {
    gameRegion = region;
}

int getRegion() {
    return gameRegion;
}

void setLevel(int level) {
    gameLevel = level;
}

int getLevel() {
    return gameLevel;
}

void quickLoadScenario() {
    FILE *fp;
    char oneline[256];
    int i;
    
    fp = fopen("/scenario_select.txt", "rb");
    if (!fp)
        error("Could not open file:", "/scenario_select.txt");
    // Faction
    readstr(fp, oneline);
    replaceEOLwithEOF(oneline, 256);
    for (i=0; i<MAX_DIFFERENT_FACTIONS && factionInfo[i].enabled; i++) {
        if (!strcmp(oneline, factionInfo[i].name)) {
            setFaction(FRIENDLY, i);
            break;
        }
    }
    if (i == MAX_DIFFERENT_FACTIONS || !factionInfo[i].enabled)
        error("Non existant Faction was specified in /scenario_select.txt", oneline);
    // Level
    readstr(fp, oneline);
    if (sscanf(oneline, "%i", &i) < 1)
        error("Could not read in a level nr in /scenario_select.txt", oneline);
    setLevel(i);
    // Region
    readstr(fp, oneline);
    if (sscanf(oneline, "%i", &i) < 1)
        error("Could not read in a region nr in /scenario_select.txt", oneline);
    setRegion(i);
    fclose(fp);
}

void drawGame() {
    switch (gameState) {
        #ifdef DISABLE_MENU_RTS4DS
        case SPLASH:
            drawSplash();
            break;
        #else
        case MENU_RTS4DS:
            drawMenuRts4ds();
            break;
        #endif
        case INTRO:
            drawIntro();
            break;
        case MENU_MAIN:
            drawMenuMain();
            break;
        case MENU_CONTINUE:
            drawMenuContinue();
            break;
        case MENU_LEVELSELECT:
            drawMenuLevelSelect();
            break;
        case MENU_SETTINGS:
            drawMenuSettings();
            break;
        case MENU_SOUNDROOM:
            drawMenuSoundroom();
            break;
        case MENU_FACTIONS:
            drawMenuFactions();
            break;
        case MENU_REGIONS:
            drawMenuRegions();
            break;
        case MENU_LOADGAME:
            drawMenuLoadGame();
            break;
        case MENU_SAVEGAME:
            drawMenuSaveGame();
            drawPlayScreen();
            break;
        case CUTSCENE_INTRODUCTION:
            drawCutsceneIntroduction();
            break;
        case CUTSCENE_DEBRIEFING:
            drawCutsceneDebriefing();
            break;
        case CUTSCENE_BRIEFING:
            drawCutsceneBriefing();
            break;
        case INGAME:
            if (getIngameBriefingState() == IBS_ACTIVE)
                drawIngameBriefing();
            else {
                drawInfoScreen();
                drawPlayScreen();
            }
            break;
        case MENU_INGAME:
            drawMenuIngame();
            drawPlayScreen();
            break;
        case MENU_GAMEINFO:
            drawMenuGameinfo();
            drawPlayScreen();
            break;
        case MENU_GAMEINFO_OBJECTIVES:
            drawMenuGameinfoObjectives();
            drawPlayScreen();
            break;
        case MENU_GAMEINFO_ITEM_SELECT:
            drawMenuGameinfoItemSelect();
            drawPlayScreen();
            break;
        case MENU_GAMEINFO_ITEM_INFO:
            drawMenuGameinfoItemInfo();
            drawPlayScreen();
            break;
        case MENU_GAMEINFO_TECHTREE:
            drawMenuGameinfoTechtree();
            drawPlayScreen();
            break;
        case GUIDE_INGAME:
        case GUIDE_GAMEINFO:
            drawPlayScreen();
            // there's no break here, so as to continue to next line
        case GUIDE_MAIN:
            drawGuide();
            break;
        case GALLERY:
            drawGallery();
            break;
        case OUTTRO:
            drawOuttro();
            break;
        default:
            break;
    }
}



#ifdef DEBUG_BUILD
// TEMPORARILY FOR INFAMOUS MONEY CHEAT!!11!1!1!
#include "info.h"
#endif
void doGameLogic() {
    static unsigned int gameFrame = 0;
    
    updateKeys();
    gameFrame++;
    
    switch (gameState) {
        #ifdef DISABLE_MENU_RTS4DS
        case SPLASH:
            doSplashLogic();
            break;
        #else
        case MENU_RTS4DS:
            doMenuRts4dsLogic();
            break;
        #endif
        case INTRO:
            doIntroLogic();
            break;
        case MENU_MAIN:
            doMenuMainLogic();
            break;
        case MENU_LEVELSELECT:
            doMenuLevelSelectLogic();
            break;
        case MENU_CONTINUE:
            doMenuContinueLogic();
            break;
        case MENU_SETTINGS:
            doMenuSettingsLogic();
            break;
        case MENU_SOUNDROOM:
            doMenuSoundroomLogic();
            break;
        case MENU_FACTIONS:
            doMenuFactionsLogic();
            break;
        case MENU_REGIONS:
            doMenuRegionsLogic();
            break;
        case CUTSCENE_INTRODUCTION:
            doCutsceneIntroductionLogic();
            break;
        case CUTSCENE_DEBRIEFING:
            doCutsceneDebriefingLogic();
            break;
        case CUTSCENE_BRIEFING:
            doCutsceneBriefingLogic();
            break;
        case INGAME:
            doIngameBriefingLogic();
            if (getIngameBriefingState() == IBS_ACTIVE)
                break;
            
            /* check which screen is considered the main screen and give input to it accordingly */
            if (getScreenSetUp() == SUB_TOUCH)
                inputInfoScreen();
            else if (getScreenSetUp() == MAIN_TOUCH)
                inputPlayScreen();
            else
                doScreenSetUpLogic();
            
            /* then do logic for both screens which do not rely on user input */
            //if (getObjectivesState() == OBJECTIVES_INCOMPLETE || (gameFrame & 1)) {
                doPlayScreenLogic();
                doInfoScreenLogic();
            //}
            
            doTimedtriggersLogic();
            doObjectivesLogic();
            
            if (getObjectivesState() == OBJECTIVES_INCOMPLETE && (keysDown() & KEY_START)) {
                // stop any scrolling as well
                if (getScreenSetUp() == MAIN_TO_SUB_TOUCH) {
                    setScreenSetUp(SUB_TOUCH);
                    swiWaitForVBlank();
                    drawGame(); // ensuring every layer is shifted to its proper position again
                } else if (getScreenSetUp() == SUB_TO_MAIN_TOUCH) {
                    setScreenSetUp(MAIN_TOUCH);
                    swiWaitForVBlank();
                    drawGame(); // ensuring every layer is shifted to its proper position again
                }
                setGameState(MENU_INGAME);
                playSoundeffect(SE_MENU_OK);
            }
            else {
                #ifndef REMOVE_ASTAR_PATHFINDING
                doPathfindingLogic();
                #endif
            }
            
#ifdef DEBUG_BUILD
// INFAMOUS MONEY CHEAT!!11!1!1! and Profiling cheat and Level Win cheat.
if (keysDown() & KEY_R) changeCredits(FRIENDLY, 1000);
if ((getKeysDown() & KEY_L) && (getKeysDown() & KEY_R)) {
  setScreenSetUp(MAIN_TOUCH);
  stopSoundeffects();
  initCutsceneDebriefingFilename(1);
  setLevel(getLevel() + 1);
  setLevelCurrent(getFaction(FRIENDLY), getLevel());
  if (getLevelReached(getFaction(FRIENDLY)) < getLevel())
    setLevelReached(getFaction(FRIENDLY), getLevel());
  writeSaveFile();
  setGameState(CUTSCENE_DEBRIEFING);
}
if (keysDown() & KEY_SELECT) stopProfiling();
#endif
            break;
        case MENU_INGAME:
            doMenuIngameLogic();
            break;
        case MENU_GAMEINFO:
            doMenuGameinfoLogic();
            break;
        case MENU_GAMEINFO_OBJECTIVES:
            doMenuGameinfoObjectivesLogic();
            break;
        case MENU_GAMEINFO_ITEM_SELECT:
            doMenuGameinfoItemSelectLogic();
            break;
        case MENU_GAMEINFO_ITEM_INFO:
            doMenuGameinfoItemInfoLogic();
            break;
        case MENU_GAMEINFO_TECHTREE:
            doMenuGameinfoTechtreeLogic();
            break;
        case MENU_LOADGAME:
            doMenuLoadGameLogic();
            break;
        case MENU_SAVEGAME:
            doMenuSaveGameLogic();
            break;
        case GUIDE_INGAME:
        case GUIDE_MAIN:
        case GUIDE_GAMEINFO:
            doGuideLogic();
            break;
        case GALLERY:
            doGalleryLogic();
            break;
        case OUTTRO:
            doOuttroLogic();
            break;
        default:
            break;
    }
}

void initGame() {
    gameLevel = 0;
    #ifdef DISABLE_MENU_RTS4DS
    gameState = SPLASH;
    setGameState(SPLASH);
    #else
    gameState = MENU_RTS4DS;
    setGameState(MENU_RTS4DS);
    #endif
}
