// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _GAME_H_
#define _GAME_H_

#include "inputx.h"
#include "sprites.h"
#include "fileio.h"
#include "debug.h"

#include <nds.h>

#define DISABLE_MENU_RTS4DS

enum GameState { 
                 #ifdef DISABLE_MENU_RTS4DS
                 SPLASH,
                 #else
                 MENU_RTS4DS,
                 #endif
                 INTRO, MENU_MAIN, MENU_CONTINUE, MENU_LEVELSELECT, MENU_SETTINGS, MENU_SOUNDROOM, MENU_FACTIONS, MENU_REGIONS, MENU_SCENARIOS, CUTSCENE_INTRODUCTION, CUTSCENE_DEBRIEFING, CUTSCENE_BRIEFING, INGAME, MENU_INGAME, MENU_GAMEINFO, MENU_GAMEINFO_OBJECTIVES, MENU_GAMEINFO_ITEM_SELECT, MENU_GAMEINFO_ITEM_INFO, MENU_GAMEINFO_TECHTREE,
                 GUIDE_INGAME,
                 MENU_SAVEGAME,
                 GUIDE_GAMEINFO, GUIDE_MAIN, GALLERY,
                 MENU_LOADGAME,
                 OUTTRO
               };

void setGameState(enum GameState state);
enum GameState getGameState();
void drawCaptionGame(char *string, int x, int y, int mainScreen);
void drawMenuOption(int nr, char *name, int mainScreen);

void setMenuOption(int nr);
int getMenuOption();
int updateMenuOption(int optionAmount);
int getMenuIdleTime();

void setRegion(int region);
int getRegion();

void setLevel(int level);
int getLevel();

void drawGame();
void doGameLogic();
void initGame();

#endif
