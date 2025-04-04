// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _FILEIO_H_
#define _FILEIO_H_

#include <nds.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#include "debug.h"


enum FileType { FS_RTS4DS_FILE,
                FS_PROJECT_FILE,
                FS_PALETTES,
                FS_OPTIONAL_ENVIRONMENT_PALETTES,
                FS_MENU_GRAPHICS,
                FS_BRIEFINGPICTURES_GRAPHICS,
                FS_ANIMATIONS_FILE,
                FS_ANIMATIONS_GRAPHICS,
                FS_ANIMATIONS_OVERLAY_GRAPHICS,
                FS_SUBTITLES_INFO,
                FS_SUBTITLES_GRAPHICS,
                FS_GUI_GRAPHICS,
                FS_ICONS_GRAPHICS,
                FS_ENVIRONMENT_INFO,
                FS_ENVIRONMENT_GRAPHICS,
                FS_ENVIRONMENT_PALCYCLE_INFO,
                FS_SHROUD_GRAPHICS,
                FS_FACTIONS_INFO,
                FS_FACTIONS_GRAPHICS,
                FS_STRUCTURES_INFO,
                FS_STRUCTURES_GRAPHICS,
                FS_FLAG_GRAPHICS,
                FS_UNITS_INFO,
                FS_UNITS_GRAPHICS,
                FS_PROJECTILES_INFO,
                FS_PROJECTILES_GRAPHICS,
                FS_SMOKE_GRAPHICS,
                FS_EXPLOSIONS_INFO,
                FS_EXPLOSIONS_GRAPHICS,
                FS_OVERLAY_GRAPHICS,
                FS_SCENARIO_FILE,
                FS_CURRENT_SCENARIO_FILE,
                FS_SCENARIO_MAP,
                FS_STRUCTURES_TECHTREE_FILE,
                FS_UNITS_TECHTREE_FILE,
                FS_AI_FILE,
                FS_GAMEINFO_GRAPHICS,
                FS_SOUNDEFFECT,
                FS_MUSIC,
                FS_MUSIC_INFO,
                FS_SAVEGAME
              };

void setCurrentProjectDirname(char *string);

void replaceEOLwithEOF(char *buff, int size);
void myGetStr(FILE *fp, char *buff, int size);
void readstr(FILE *fp, char *string);

void closeFile(FILE *fp);
FILE *openFile(char *string, enum FileType type);
FILE *openSaveFile(int slot, char *attributes);
FILE *openScreenshotFile(char *name, char *attributes);
unsigned int copyFile(void *dest, char *string, enum FileType type);
unsigned int copyFileVRAM(uint16 *dest, char *string, enum FileType type);
void initFileIO();

#endif
