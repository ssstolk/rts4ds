// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _MUSIC_H_
#define _MUSIC_H_

#include <nds/ndstypes.h>

#define MAX_MUSIC_CHANNELS  4
#define MUSIC_BUFFER_SIZE (100*1024)
#define MUSIC_ONE_SHOT 0
#define MUSIC_LOOP     1

enum MusicType { MT_NONE, MT_MENU, MT_MENU_FACTION, MT_INGAME, MT_MISC };

void playMusic(enum MusicType type, bool loop);
void playMiscMusic(char *filename, bool loop);
void stopMusic();
void applyMusicVolume(int level);
void doMusicLogic();
void initMusicWithScenario();
void initMusic();

#endif
