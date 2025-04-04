// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

enum GameType { SINGLEPLAYER, MULTIPLAYER_SERVER, MULTIPLAYER_CLIENT };
enum ScreenSwitchMethod { PRESS_TRIGGER, HOLD_TRIGGER };
enum PrimaryHand { RIGHT_HANDED, LEFT_HANDED };


enum GameType getGameType();
void setGameType(enum GameType type);

enum ScreenSwitchMethod getScreenSwitchMethod();
void setScreenSwitchMethod(enum ScreenSwitchMethod method);

int getGameSpeed();
void setGameSpeed(int speed);

enum PrimaryHand getPrimaryHand();
void setPrimaryHand(enum PrimaryHand hand);

int getMusicVolume();
void setMusicVolume(int volume);

int getSubtitlesEnabled();
void setSubtitlesEnabled(int enabled);

#ifdef _MONAURALSWITCH_
int getMonauralEnabled();
void setMonauralEnabled(int enabled);
#endif

int getBacklightLevel();
void cycleBacklightLevel();



enum Medal { MEDAL_BRONZE=0, MEDAL_SILVER=1, MEDAL_GOLD=2, MEDAL_NONE=-1 };


int getLevelReached(int factionInfo);
void setLevelReached(int factionInfo, int level);

int getLevelCurrentFactionInfo();
int getLevelCurrent();
void setLevelCurrent(int factionInfo, int level);

enum Medal getMedalReceived(int factionInfo, int level);
void setMedalReceived(int factionInfo, int level, enum Medal medal);

enum Medal getFactionMedalReceived(int factionInfo);
void setFactionMedalReceived(int factionInfo, enum Medal medal);




void readSaveFile();
void writeSaveFile();
void clearProgress();
void clearSaveFile();

#endif
