// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "settings.h"
#include "factions.h"
#include "music.h"
#include "fileio.h"

#define SAVEFILE "/save.dat"

enum SaveFileSetting { SFS_SCREEN_SWITCHING_METHOD, SFS_GAME_SPEED, SFS_PRIMARY_HAND, 
                       SFS_MUSIC_VOLUME, SFS_SUBTITLES_ENABLED,
#ifdef _MONAURALSWITCH_
                       SFS_MONAURAL_ENABLED,
#endif
                       NUMBER_OF_SFS};


struct SaveFile {
    int setting[NUMBER_OF_SFS];
    int level_reached[MAX_DIFFERENT_FACTIONS];
    int level_current[2]; // faction, level (-1 for faction signifies not having one)
    int medal_received[MAX_DIFFERENT_FACTIONS][9];
    int faction_medal_received[MAX_DIFFERENT_FACTIONS];
};

struct SaveFile saveFile;

int backlightLevel = BACKLIGHT_LOW;



void createDefaultSaveFile() {
    int i, j;
    
    saveFile.setting[SFS_SCREEN_SWITCHING_METHOD] = PRESS_TRIGGER;
    saveFile.setting[SFS_GAME_SPEED] = 3; // normal game speed
    saveFile.setting[SFS_PRIMARY_HAND] = RIGHT_HANDED;
    saveFile.setting[SFS_MUSIC_VOLUME] = 100;
    saveFile.setting[SFS_SUBTITLES_ENABLED] = 1;
    #ifdef _MONAURALSWITCH_
    saveFile.setting[SFS_MONAURAL_ENABLED] = 0;
    #endif
    
    for (i=0; i<MAX_DIFFERENT_FACTIONS; i++)
        saveFile.level_reached[i] = 1;
    saveFile.level_current[0] = -1;
    for (i=0; i<MAX_DIFFERENT_FACTIONS; i++) {
        for (j=0; j<9; j++)
            saveFile.medal_received[i][j] = -1;
    }
    for (i=0; i<MAX_DIFFERENT_FACTIONS; i++)
        saveFile.faction_medal_received[i] = -1;
}

void readSaveFile() {
    FILE *fp = openSaveFile(-1, "rb");
    if (fp) {
        if (fread(&saveFile, 1, sizeof(struct SaveFile), fp) < sizeof(struct SaveFile))
            createDefaultSaveFile();
//            error("Savefile is thought to be corrupted", "");
        closeFile(fp);
    } else {
        createDefaultSaveFile();
        writeSaveFile();
    }
    applyMusicVolume(getMusicVolume());
}

void writeSaveFile() {
    FILE *fp = openSaveFile(-1, "wb");
    if (fp) {
        fwrite(&saveFile, 1, sizeof(struct SaveFile), fp);
        closeFile(fp);
    }
}

void clearProgress() {
    int i, j;
    
    for (i=0; i<MAX_DIFFERENT_FACTIONS; i++)
        saveFile.level_reached[i] = 1;
    saveFile.level_current[0] = -1;
    for (i=0; i<MAX_DIFFERENT_FACTIONS; i++) {
        for (j=0; j<9; j++)
            saveFile.medal_received[i][j] = -1;
    }
    for (i=0; i<MAX_DIFFERENT_FACTIONS; i++)
        saveFile.faction_medal_received[i] = -1;
    
    writeSaveFile();
}

void clearSaveFile() {
    createDefaultSaveFile();
    writeSaveFile();
}




enum GameType settingsGameType = SINGLEPLAYER;

inline enum GameType getGameType() {
    return settingsGameType;
}

void setGameType(enum GameType type) {
    settingsGameType = type;
}


inline enum ScreenSwitchMethod getScreenSwitchMethod() {
    return saveFile.setting[SFS_SCREEN_SWITCHING_METHOD];
}

void setScreenSwitchMethod(enum ScreenSwitchMethod method) {
    saveFile.setting[SFS_SCREEN_SWITCHING_METHOD] = method;
}


inline int getGameSpeed() {
    return saveFile.setting[SFS_GAME_SPEED];
}

void setGameSpeed(int speed) {
    saveFile.setting[SFS_GAME_SPEED] = speed;
}


inline enum PrimaryHand getPrimaryHand() {
    return saveFile.setting[SFS_PRIMARY_HAND];
}

void setPrimaryHand(enum PrimaryHand hand) {
    saveFile.setting[SFS_PRIMARY_HAND] = hand;
}


inline int getMusicVolume() {
    return saveFile.setting[SFS_MUSIC_VOLUME];
}

void setMusicVolume(int volume) {
    if (volume < 0)   volume = 0;
    if (volume > 100) volume = 100;
    saveFile.setting[SFS_MUSIC_VOLUME] = volume;
    applyMusicVolume(getMusicVolume());
}


inline int getSubtitlesEnabled() {
    return saveFile.setting[SFS_SUBTITLES_ENABLED];
}

void setSubtitlesEnabled(int enabled) {
    saveFile.setting[SFS_SUBTITLES_ENABLED] = enabled;
}

#ifdef _MONAURALSWITCH_
inline int getMonauralEnabled() {
    return saveFile.setting[SFS_MONAURAL_ENABLED];
}

void setMonauralEnabled(int enabled) {
    saveFile.setting[SFS_MONAURAL_ENABLED] = enabled;
}
#endif

int getBacklightLevel() {
    // In default ARM7 of libnds there is no way to retrieve the backlight status, only to set it.
    // Backlight is enabled in the initialization of this game, but no idea which level exactly.
    return backlightLevel;
}

void cycleBacklightLevel() {
    backlightLevel++;
    if (backlightLevel > BACKLIGHT_LOW) { // should have been BACKLIGHT_MAX
        backlightLevel = -1;
        powerOff(PM_BACKLIGHT_TOP | PM_BACKLIGHT_BOTTOM);
    }
    if (backlightLevel >= BACKLIGHT_LOW) {
        // TODO: set backlight level
        powerOn(PM_BACKLIGHT_TOP | PM_BACKLIGHT_BOTTOM);
    }
}



inline int getLevelReached(int factionInfo) {
    return saveFile.level_reached[factionInfo];
}

void setLevelReached(int factionInfo, int level) {
    saveFile.level_reached[factionInfo] = level;
}


inline int getLevelCurrentFactionInfo() {
    return saveFile.level_current[0];
}

inline int getLevelCurrent() {
    return saveFile.level_current[1];
}

void setLevelCurrent(int factionInfo, int level) {
    saveFile.level_current[0] = factionInfo;
    saveFile.level_current[1] = level;
}

inline enum Medal getMedalReceived(int factionInfo, int level) {
    if (level <= 0 || level > 9)
        return MEDAL_BRONZE;
    
    return saveFile.medal_received[factionInfo][level-1];
}

void setMedalReceived(int factionInfo, int level, enum Medal medal) {
    saveFile.medal_received[factionInfo][level-1] = medal;
}

inline enum Medal getFactionMedalReceived(int factionInfo) {
    return saveFile.faction_medal_received[factionInfo];
}

void setFactionMedalReceived(int factionInfo, enum Medal medal) {
    saveFile.faction_medal_received[factionInfo] = medal;
}
