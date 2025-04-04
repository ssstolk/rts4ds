// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "fileio.h"

#include "game.h"
#include "factions.h"
#include "view.h"
#include "settings.h"
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <filesystem.h> // include NitroFS lib


#ifndef DEFAULT_LOCATION_NDS
  #define DEFAULT_LOCATION_NDS  "fat:/rts4ds.nds"
#endif
#ifndef FS_ROOT_NITRO
  #define FS_ROOT_NITRO    "nitro:/"
#endif
#ifndef FS_ROOT_FAT
  #define FS_ROOT_FAT      "fat:/rts4ds/"
#endif

//#define ENABLE_LZSS_COMPRESSION


#define FIO_MAX_FILES_OPEN  10
#define PROJECT_PALETTES_LOCATION               "palettes/"
#define PROJECT_MENUS_LOCATION                  "menus/"
#define PROJECT_BRIEFINGPICTURES_LOCATION       "briefingpics/"
#define PROJECT_ANIMATIONS_LOCATION             "animations/"
#define PROJECT_SUBTITLES_LOCATION              "subtitles/"
#define PROJECT_GUI_LOCATION                    "gui/"
#define PROJECT_ICONS_LOCATION                  "icons/"
#define PROJECT_FACTIONS_LOCATION               "factions/"
#define PROJECT_ENVIRONMENT_LOCATION            "environment/"
#define PROJECT_SHROUD_LOCATION                 "shroud/"
#define PROJECT_STRUCTURES_LOCATION             "structures/"
#define PROJECT_FLAG_LOCATION                   "flags/"
#define PROJECT_UNITS_LOCATION                  "units/"
#define PROJECT_PROJECTILES_LOCATION            "projectiles/"
#define PROJECT_SMOKE_LOCATION                  "smoke/"
#define PROJECT_EXPLOSIONS_LOCATION             "explosions/"
#define PROJECT_OVERLAY_LOCATION                "overlay/"
#define PROJECT_SCENARIOS_LOCATION              "scenarios/"
#define PROJECT_STRUCTURES_TECHTREE_LOCATION    "techtrees/"
#define PROJECT_UNITS_TECHTREE_LOCATION         "techtrees/"
#define PROJECT_AI_LOCATION                     "ai/"
#define PROJECT_GAMEINFO_LOCATION               "gameinfo/"
#define PROJECT_SOUNDEFFECTS_LOCATION           "sound/"
#define PROJECT_MUSIC_LOCATION                  "music/"
#define PREFIX_FS_SCENARIO_MAP                  "MAP" /* .TXT */


bool useFAToverNitroFS;
char *currentProjectDirname;


void setCurrentProjectDirname(char *string) {
    currentProjectDirname = string;
}

void replaceEOLwithEOF(char *buff, int size) {
    int i;
    for (i=0; i<size; i++) {
        if (buff[i] == '\n') {
            buff[i] = '\0';
            return;
        }
    }
}

void myGetStr(FILE *fp, char *buff, int size) {
    *buff = fgetc(fp);
    
    while((*buff != '\n') && (*buff != 0xD) && (*buff != 0x0)) {
        buff++;
        *buff = fgetc(fp);
    }
    if (buff[0] != 0) {
        buff[0] = '\n';	
        buff[1] = 0;
    }
}

void readstr(FILE *fp, char *string) {
    do {
        myGetStr(fp, string, 255);
    } while ((string[0] == '/') || (string[0] == '\n' ));
}


/*bool fileLocator(char *start, char *target, bool isDir, int depth, char *result) { 
    char *childName;
    unsigned char childType;
    char temp[256]; 
    DIR *dir = opendir(start); 
    
    if (dir != NULL) { 
        while(true) {
			struct dirent* dirEntry = readdir(dir);
			if(dirEntry == NULL) break;
            childName = dirEntry->d_name;
            childType = dirEntry->d_type;
            if ((strlen(childName) == 1 && childName[0] == '.') ||
                (strlen(childName) == 2 && childName[0] == '.' && childName[1] == '.'))
                continue; 
            if ((childType == DT_DIR) == isDir) {
                if (strcmp(target, childName) == 0) {
                    strcpy(result, start); 
                    if (start[strlen(start)-1] != '/') 
                        strcat(result, "/"); 
                    strcat(result, childName); 
                    if (isDir)
                        strcat(result, "/"); 
                    closedir(dir); 
                    return true; 
                } 
            } 
            if ((childType == DT_DIR) && depth > 1) {
                strcpy(temp, start); 
                if (start[strlen(start)-1] != '/') 
                    strcat(temp, "/"); 
                strcat(temp, childName); 
                strcat(temp, "/"); 
                if (fileLocator(temp, target, isDir, depth-1, result)) {
                    closedir(dir); 
                    return true; 
                } 
            } 
        } 
    } 
    closedir(dir); 
    return false; 
}*/


void closeFile(FILE *fp) {
    if (fclose(fp))
        error("FAT could not close a file", "");
}

FILE *openFile(char *string, enum FileType type) {
    char filepath_relative[1024];
    FILE *fp;
    
    if (type == FS_RTS4DS_FILE)
        strcpy(filepath_relative, string);
    else {
        strcpy(filepath_relative, currentProjectDirname);
        strcat(filepath_relative, "/");
        
        switch (type) {
            case FS_PROJECT_FILE:
                strcat(filepath_relative, string);
                break;
            case FS_PALETTES:
            case FS_OPTIONAL_ENVIRONMENT_PALETTES:
                strcat(filepath_relative, PROJECT_PALETTES_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_MENU_GRAPHICS:
                strcat(filepath_relative, PROJECT_MENUS_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_BRIEFINGPICTURES_GRAPHICS:
                strcat(filepath_relative, PROJECT_BRIEFINGPICTURES_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".gif");
                break;
            case FS_ANIMATIONS_FILE:
                strcat(filepath_relative, PROJECT_ANIMATIONS_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".ini");
                break;
            case FS_ANIMATIONS_GRAPHICS:
                strcat(filepath_relative, PROJECT_ANIMATIONS_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".gif");
                break;
            case FS_ANIMATIONS_OVERLAY_GRAPHICS:
                strcat(filepath_relative, PROJECT_ANIMATIONS_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".bin");
                break;
            case FS_SUBTITLES_INFO:
                strcat(filepath_relative, PROJECT_SUBTITLES_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".ini");
                break;
            case FS_SUBTITLES_GRAPHICS:
                strcat(filepath_relative, PROJECT_SUBTITLES_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_GUI_GRAPHICS:
                strcat(filepath_relative, PROJECT_GUI_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_ICONS_GRAPHICS:
                strcat(filepath_relative, PROJECT_ICONS_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_FACTIONS_INFO:
                strcat(filepath_relative, PROJECT_FACTIONS_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".ini");
                break;
            case FS_FACTIONS_GRAPHICS:
                strcat(filepath_relative, PROJECT_FACTIONS_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_ENVIRONMENT_INFO:
            case FS_ENVIRONMENT_PALCYCLE_INFO:
                strcat(filepath_relative, PROJECT_ENVIRONMENT_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".ini");
                break;
            case FS_ENVIRONMENT_GRAPHICS:
                strcat(filepath_relative, PROJECT_ENVIRONMENT_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_SHROUD_GRAPHICS:
                strcat(filepath_relative, PROJECT_SHROUD_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_STRUCTURES_INFO:
                strcat(filepath_relative, PROJECT_STRUCTURES_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".ini");
                break;
            case FS_STRUCTURES_GRAPHICS:
                strcat(filepath_relative, PROJECT_STRUCTURES_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_FLAG_GRAPHICS:
                strcat(filepath_relative, PROJECT_FLAG_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_UNITS_INFO:
                strcat(filepath_relative, PROJECT_UNITS_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".ini");
                break;
            case FS_UNITS_GRAPHICS:
                strcat(filepath_relative, PROJECT_UNITS_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_PROJECTILES_INFO:
                strcat(filepath_relative, PROJECT_PROJECTILES_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".ini");
                break;
            case FS_PROJECTILES_GRAPHICS:
                strcat(filepath_relative, PROJECT_PROJECTILES_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_SMOKE_GRAPHICS:
                strcat(filepath_relative, PROJECT_SMOKE_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_EXPLOSIONS_INFO:
                strcat(filepath_relative, PROJECT_EXPLOSIONS_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".ini");
                break;
            case FS_EXPLOSIONS_GRAPHICS:
                strcat(filepath_relative, PROJECT_EXPLOSIONS_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_OVERLAY_GRAPHICS:
                strcat(filepath_relative, PROJECT_OVERLAY_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin");
                #endif
                break;
            case FS_SCENARIO_FILE:
                strcat(filepath_relative, PROJECT_SCENARIOS_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".ini");
                break;
            case FS_CURRENT_SCENARIO_FILE:
                strcat(filepath_relative, PROJECT_SCENARIOS_LOCATION);
                strcat(filepath_relative, "scenario_");
if (getGameType() != SINGLEPLAYER)
strcat(filepath_relative, "multiplayer_testlevel");
else {
                strcat(filepath_relative, factionInfo[getFaction(FRIENDLY)].name);
                sprintf(filepath_relative + strlen(filepath_relative), "_level%i_region%i", getLevel(), getRegion());
}
                strcat(filepath_relative, ".ini");
                break;
            case FS_SCENARIO_MAP:
                strcat(filepath_relative, PROJECT_SCENARIOS_LOCATION);
                strcat(filepath_relative, PREFIX_FS_SCENARIO_MAP);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".txt");
                break;
            case FS_STRUCTURES_TECHTREE_FILE:
                strcat(filepath_relative, PROJECT_STRUCTURES_TECHTREE_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, "_structures.ini");
                break;
            case FS_UNITS_TECHTREE_FILE:
                strcat(filepath_relative, PROJECT_UNITS_TECHTREE_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, "_units.ini");
                break;
            case FS_AI_FILE:
                strcat(filepath_relative, PROJECT_AI_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".ini");
                break;
            case FS_GAMEINFO_GRAPHICS:
                strcat(filepath_relative, PROJECT_GAMEINFO_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".gif");
                break;
            case FS_SOUNDEFFECT:
                strcat(filepath_relative, PROJECT_SOUNDEFFECTS_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".wav.lzc");
                #else
                strcat(filepath_relative, ".wav");
                #endif
                break;
            case FS_MUSIC:
                strcat(filepath_relative, PROJECT_MUSIC_LOCATION);
                strcat(filepath_relative, string);
                #ifdef ENABLE_LZSS_COMPRESSION
                strcat(filepath_relative, ".bin.lzc");
                #else
                strcat(filepath_relative, ".bin"); // used to be .mod only; now soundbanks created by mmutils from .mod, .it, or other tracker files
                #endif
                break;
            case FS_MUSIC_INFO:
                strcat(filepath_relative, PROJECT_MUSIC_LOCATION);
                strcat(filepath_relative, string);
                strcat(filepath_relative, ".ini");
                break;
            default:
                break;
        }
    }

    // open the filepath (which is relative to root)
    chdir(useFAToverNitroFS ? FS_ROOT_FAT : FS_ROOT_NITRO);
    fp = fopen(filepath_relative, "r");
    if (!fp && type != FS_OPTIONAL_ENVIRONMENT_PALETTES && type != FS_ENVIRONMENT_PALCYCLE_INFO && type != FS_SOUNDEFFECT && type != FS_MUSIC && type != FS_MUSIC_INFO && type != FS_ANIMATIONS_FILE && type != FS_GAMEINFO_GRAPHICS && (type != FS_MENU_GRAPHICS || strncmp(string, "gameinfo", strlen("gameinfo"))))
        error("Could not open requested file:", filepath_relative);
    return fp;
}

// Function openSaveFile.
// param slot: >= 0 is ingame savestate
//              < 0 is settings file
FILE *openSaveFile(int slot, char *attributes) {
    char filepath[1024];
    
    // argv[0] is sure to have been initialized (see initFileIO)
    strcpy(filepath, __system_argv->argv[0]);
    #ifndef DISABLE_MENU_RTS4DS
	strcat(filepath, currentProjectDirname);
    #endif
	if (slot < 0)
        strcat(filepath, ".settings.sav");
    else// {
        sprintf(filepath + strlen(filepath), "%s%i%s", ".slot", slot, ".sav");
    
    chdir("fat:/");
    return fopen(filepath, attributes);
}

FILE *openScreenshotFile(char *name, char *attributes) {
    char filepath[1024];
    
    // argv[0] is sure to have been initialized (see initFileIO)
    strcpy(filepath, __system_argv->argv[0]);
	strcat(filepath, ".");
	strcat(filepath, name);
	return fopen(filepath, attributes);
}

#ifdef ENABLE_LZSS_COMPRESSION
// BIOS Function helper
int getSize(uint8 * source, uint16 * dest, uint32 r2) {
    u32 size = *((u32*)source) >> 8;
    return (size<<8) | 16;
}

// BIOS Function helper
uint8 readByte(uint8 * source) {
    return *source++;
}

// Decompresses using BIOS LZ77 Compression, writing half-words at a time
int decompressToVRAM(void* dest, char *string, enum FileType type) {
    int result;
    void *buffer;
    FILE *fp;
    int size;
    TDecompressionStream decStream = {getSize, NULL, readByte};
    
    fp = openFile(string, type);
    if (!fp)
        return 0;
    
    fseek(fp, 0, SEEK_END); 
    size = ftell(fp); 
    rewind(fp);
    
    if (size == 0)
        return 0;
    
    buffer = malloc(size);
    if (!buffer)
        errorSI("Failed to malloc in decompressToVRAM", size);
    fread(buffer, 1, size, fp);
    closeFile(fp);
    result = swiDecompressLZSSVram(buffer, dest, 0, &decStream);
    free(buffer);
    return result;
}

inline unsigned int copyFile(void *dest, char *string, enum FileType type) {
    return decompressToVRAM(dest, string, type);
}

inline unsigned int copyFileVRAM(uint16 *dest, char *string, enum FileType type) {
    return decompressToVRAM(dest, string, type);
}

#else
unsigned int copyFile(void *dest, char *string, enum FileType type) {
    FILE *fp;
    int size;
    
    fp = openFile(string, type);
    if (!fp)
        return 0;
    
    fseek(fp, 0, SEEK_END); 
    size = ftell(fp); 
    rewind(fp);
    
    fread(dest, size, 1, fp);
    closeFile(fp);
    return size;
}

unsigned int copyFileVRAM(uint16 *dest, char *string, enum FileType type) {
    FILE *fp;
    int size;
    int i;
    uint16 *buff;
    
    fp = openFile(string, type);
    if (!fp)
        return 0;
    
    fseek(fp, 0, SEEK_END); 
    size = ftell(fp); 
    rewind(fp);
    
    buff = (uint16*) malloc(size);
    if (!buff)
        error("Failed to dynamically allocate space", string);
    fread(buff, size, 1, fp);
    for (i=0; i<(size>>1); i++) {
        dest[i] = buff[i];
    }
    closeFile(fp);
    free(buff);
    return size;
}
#endif


#define SRAM_WAIT_18  0x03
void initFileIO() {
    if (!isDSiMode()) {
        // these two are to make sure GH grip can be used when inserted
        REG_EXMEMCNT &= ~0x0880;
        REG_EXMEMCNT |= SRAM_WAIT_18;
    }
    // initialize NitroFS, in combination with FAT
    if (!( __system_argv->argvMagic == ARGV_MAGIC && __system_argv->argc >= 1 )) {
        // attempt default .nds location by setting it in argv
        __system_argv->argvMagic = ARGV_MAGIC;
        __system_argv->argc = 1;
        __system_argv->argv[0] = malloc(strlen(DEFAULT_LOCATION_NDS)+1);
        strcpy(__system_argv->argv[0], DEFAULT_LOCATION_NDS);
    }
    if (!nitroFSInit(NULL))
        error("Initializing file access failed.",
              "Please start the .nds file  by  "
              "using a launcher such as        "
              "github.com/devkitPro/nds-hb-menu\n");

    DIR* dir = opendir(FS_ROOT_FAT);
    useFAToverNitroFS = dir ? true : false;
    if (dir)
        closedir(dir);
}
