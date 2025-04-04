// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _SAVEGAME_H_
#define _SAVEGAME_H_

struct SavegameHeader_radar {
    unsigned short image[64*64];       // 64x64 15bpp image   ( 8 KB )
};

struct SavegameHeader_info {
    unsigned char faction;     // index of factionInfo[MAX_DIFFERENT_FACTIONS].name
    unsigned char level;       // 0 to ?
    unsigned char map;         // 0 to ?
    unsigned char date_year;   // 0 to 255 (2000 to 2255)
    unsigned char date_month;  // 1 to 12
    unsigned char date_day;    // 1 to 31
    unsigned char date_hour;   // 0 to 23
    unsigned char date_minute; // 0 to 59
    // some other info we need even if we won't display them in the panel
    int matchtime;
    int view_X;
    int view_Y;
    int gamespeed;
    int level_loaded_via_level_select;
};

struct SavegameHeader {
    struct SavegameHeader_radar radar;
    struct SavegameHeader_info info;
    // do we need padding?
};

int saveGameLoadHeader(struct SavegameHeader *header, int slot);  // 0 = FAILURE
int saveGame(int slot);                                           // 0 = FAILURE
int loadGame(int slot);                                           // 0 = FAILURE
void clearSaveGame(int slot);

#endif

