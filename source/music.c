// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "music.h"

//#define REMOVE_MUSIC_ENGINE
#ifdef REMOVE_MUSIC_ENGINE
void playMusic(enum MusicType type, bool loop) {}
void playMiscMusic(char *filename, bool loop) {}
void stopMusic() {}
void applyMusicVolume(int level) {}
void doMusicLogic() {}
void initMusic() {}
void initMusicWithScenario() {}
#else

#include <nds.h>
#include <maxmod9.h>
#include "factions.h"
#include "fileio.h"
#include "settings.h"
#include "shared.h"

bool musicLoaded = false;
unsigned char musicBuffer[MUSIC_BUFFER_SIZE];
enum MusicType musicType = MT_NONE;
// bool musicLoop;
unsigned int musicCounter;
unsigned int maxIngameMusic[MAX_DIFFERENT_FACTIONS];
uint8* allowMusic = 0;

int getFactionForMusic() {
    int i;
    for (i=getFaction(FRIENDLY); i>=0 && maxIngameMusic[i]==0; i--); // if current faction has no music to play, use a previous faction's music
    return i;
}

void playMaxmodMusic(bool loop) {
    uint16 musicUnlockChannelMask = 0;
    int i;
    
    for (i=0; i<MAX_MUSIC_CHANNELS; i++)
        musicUnlockChannelMask |= BIT(15-i);

    // loading in the single song (id: 0) from the musicBuffer
    mmInitDefaultMem((mm_addr) musicBuffer);
    mmLockChannels((~(musicUnlockChannelMask<<16))>>16);
    mmUnlockChannels(musicUnlockChannelMask);
    mmLoad(0);
    musicLoaded = true;
    applyMusicVolume(getMusicVolume());
    mmStart(0, loop ? MM_PLAY_LOOP : MM_PLAY_ONCE);
}

void stopMaxmodMusic() {
    if (musicLoaded) {    
        mmStop();
        mmUnload(0);
        musicLoaded = false;
    }
}

void playMusic(enum MusicType type, bool loop) {
    unsigned int size;
    char  filename[256];
    char* factionName = factionInfo[getFaction(FRIENDLY)].name;
    int factionForMusic;
    
    startProfilingFunction("playMusic");
    
    if (type == musicType && type != MT_NONE && mmActive()) {
        stopProfilingFunction();
        return;
    }
    
    musicType = type;
    
    stopMaxmodMusic();
    
    if (type == MT_NONE) {
        stopProfilingFunction();
        return;
    }
    
    if (type == MT_MENU)
        strcpy(filename, "menu");
    else if (type == MT_MENU_FACTION)
        sprintf(filename, "menu_%s", factionName);
    else if (type == MT_INGAME) {
        factionForMusic = getFactionForMusic();
        do {
            musicCounter++;
            musicCounter %= maxIngameMusic[factionForMusic];
        } while (!allowMusic[musicCounter]);
        sprintf(filename, "%s_ingame%i", factionInfo[factionForMusic].name, musicCounter+1); // files range from 1 to x while in src they range from 0 to x-1
    }

    if ((size = copyFile(musicBuffer, filename, FS_MUSIC))) { // read a new MOD file soundbank in
        if (size > MUSIC_BUFFER_SIZE)
            error("The following music exceeded the 100KB limit", filename);
        playMaxmodMusic(loop);
        stopProfilingFunction();
        return;
    }
    
    musicType = MT_NONE;
    
    // could not find music to play
    if (type == MT_INGAME && musicCounter != 0)
        playMusic(type, loop);
    
    stopProfilingFunction();
}

void playMiscMusic(char *filename, bool loop) {
    unsigned int size;
    
    startProfilingFunction("playMiscMusic");
    
    musicType = MT_MISC;
    stopMaxmodMusic();
    if ((size = copyFile(musicBuffer, filename, FS_MUSIC))) { // read a new MOD file in
        if (size > MUSIC_BUFFER_SIZE)
            error("The following music exceeded the 100KB limit", filename);
        playMaxmodMusic(loop);
    } else
        musicType = MT_NONE;
    
    stopProfilingFunction();
}

void applyMusicVolume(int level) {
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    mmSetModuleVolume((1024 * getMusicVolume()) / 100);
}

void stopMusic() {
    playMusic(MT_NONE, MUSIC_ONE_SHOT);
}

void doMusicLogic() {
    startProfilingFunction("doMusicLogic");
    if (!mmActive() && musicType == MT_INGAME)
        playMusic(musicType, MUSIC_ONE_SHOT);
    stopProfilingFunction();
}

void initMusicWithScenario() {
    // optional line section [MUSIC] with
    // Play=
    // DontPlay=
    
    FILE *fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    char oneline[256];
    
    int i;
    int max = maxIngameMusic[getFactionForMusic()];
    char *cur;
    
    int nr;
    int count = 0;
    
    uint8 setVal;
    
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[MUSIC]", strlen("[MUSIC]")) && strncmp(oneline, "[TIMERS]", strlen("[TIMERS]")));

    if (oneline[1] == 'M') { // found (optional) [MUSIC] section
        readstr(fp, oneline); // read next line which contains either Play=#,#,# or DontPlay=#,#,#
    }
    
    if (oneline[0] == 'P') {
        setVal = 1;
        cur = oneline + strlen("Play=");
    } else if (oneline[0] == 'D') {
        setVal = 0;
        cur = oneline + strlen("DontPlay=");
    } else {
        closeFile(fp);
        
        // allow all, randomize play, and return
        for (i=0; i<max; i++)
            allowMusic[i] = 1;
        musicCounter = rand() % max;
        return;
    }
    
    for (i=0; i<max; i++)
        allowMusic[i] = !setVal;
    for (;;) {
        if (sscanf(cur, "%i", &nr) < 1)
            break;
        allowMusic[nr-1] = setVal;
        count++;
        while (nr >= 10) {
            cur++;
            nr /= 10;
        }
        cur++;
        if (cur[0] != ',')
            break;
        cur++;
    }
    
    closeFile(fp);
    
    musicCounter = rand() % max;
    
/*char debugline[256];
for (i=0; i<20; i++) {
    debugline[i*2]   = '0' + allowMusic[i];
    debugline[i*2+1] = ' ';
}
debugline[20*2] = 0;
errorSI(debugline, musicCounter);*/
}

void initMusic() {
    FILE *fp;
    char oneline[256];
    unsigned count, maxCount;
    int i;
    
    musicType = MT_NONE;
//    musicLoop = false;
    musicCounter = 0;
    stopMusic();
    
    for (i=0; i<MAX_DIFFERENT_FACTIONS; i++)
        maxIngameMusic[i] = 1;
    maxCount = 1;
    
    fp = openFile("count", FS_MUSIC_INFO);
    if (!fp)
        return;
    
    for (;;) {
        readstr(fp, oneline);
        for (i=0; i<MAX_DIFFERENT_FACTIONS && factionInfo[i].enabled; i++) {
            if (!strncmp(factionInfo[i].name, oneline, strlen(factionInfo[i].name))) {
                sscanf(oneline + strlen(factionInfo[i].name), " = %u", &count);
                maxIngameMusic[i] = count;
                if (count > maxCount)
                    maxCount = count;
                break;
            }
        }
        if (i == MAX_DIFFERENT_FACTIONS)
            break;
            //error("Could not identify faction in music/count.ini\n"
            //      "Please use format 'factionname = count' instead of:\n", oneline);
    }
    closeFile(fp);
    
    if (!allowMusic)
        allowMusic = (uint8*) malloc(sizeof(int) * maxCount);
}

#endif
