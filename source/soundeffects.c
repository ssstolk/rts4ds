// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "soundeffects.h"

#include "factions.h"
#include "game.h"
#include "fileio.h"


#define SOUNDEFFECTS_BUFFER_SIZE        (490*1024)  /* 490 kb, ouch ;) */
#define SOUNDEFFECTS_MISC_BUFFER_SIZE    (50*1024)  /* and another 50 kb */

#define MAX_SOUNDEFFECT_BUFFERING_PER_FRAME  (11025 / FPS)  /* in bytes.   11025 Hz == 11025 bytes/sec */

//#define REMOVE_SOUND_ENGINE
#ifdef REMOVE_SOUND_ENGINE
void stopSoundeffects() {}
void updateSEafterVBlank() {}
int playSoundeffectControlled(enum Soundeffect se, unsigned int volumePercentage, unsigned int sp) { return 0; }
int playSoundeffect(enum Soundeffect se) { return 0; }
int SoundeffectControlled(char *name, unsigned int volumePercentage, unsigned int sp) { return 0; }
int playMiscSoundeffect(char *name) { return 0; }
int playGameTimerSoundeffectControlled(char *name, unsigned int volumePercentage, unsigned int sp) { return 0; }
int playGameTimerSoundeffect(char *name) { return 0; }
int playObjectiveSoundeffectControlled(char *name, unsigned int volumePercentage, unsigned int sp) { return 0; }
int playObjectiveSoundeffect(char *name) { return 0; }
void requestContinueBufferingSoundeffect(char *name) {}
void initSoundeffectsWithScenario() {}
void initSoundeffects() {}
#else

struct SoundeffectChannel {
    int hardware_channel;
    enum Soundeffect se;
    unsigned int frames_left;
};
struct SoundeffectChannel soundeffectChannel[MAX_SOUNDEFFECT_CHANNELS];


/* Assuming PCM Wave format, mono.                       */
/* It mostly uses Little Endian, which the DS also uses. */

struct WaveFmtChunk {
    char fmtId[4]; // needs to be "fmt "
    u32 chunkSize; // specified in bytes
    u16 audioFormat;
    u16 numChannels;
    u32 sampleRate;
    u32 byteRate;
    u16 blockAlign;
    u16 bitsPerSample;
};

struct WaveDataChunk {
    char dataId[4]; // needs to be "data"
    u32 chunkSize; // specified in bytes
};

struct WaveChunks {
    struct WaveFmtChunk  *fmtChunk;
    struct WaveDataChunk *dataChunk;
};



unsigned char soundeffectsMiscBuffer[SOUNDEFFECTS_MISC_BUFFER_SIZE];
unsigned char soundeffectsBuffer[SOUNDEFFECTS_BUFFER_SIZE];
struct WaveChunks *soundeffect[SE_COUNT];

unsigned int sizeSoundeffectsWithoutScenario;

unsigned int soundCreditUnusableDuration;
unsigned int soundCreditsUnusableDuration;
unsigned int soundEnemyUnitApproachingUnusableDuration;
unsigned int soundFriendlyBaseAttackedUnusableDuration;
unsigned int soundPlaceStructureUnusableDuration;

u8 channelThatShouldBeLouder=255;


struct RequestBufferingSoundeffect {
    FILE *fp;
    char filename[256];
    unsigned int sizeRead; // bytes
    int finalized;
};

struct QueuedBufferedSoundeffect {
    enum Soundeffect se; // BG/MISC, GAME, OBJECTIVE
    unsigned int vp; // volume percentage
    unsigned int sp; // sound panning
};

struct CurrentBufferedSoundeffect {
    unsigned int sizePlayed; // bytes 
    unsigned int byteRate;
};


struct RequestBufferingSoundeffect requestBufferingSoundeffect;
struct QueuedBufferedSoundeffect   queuedBufferedSoundeffect;
struct CurrentBufferedSoundeffect  currentBufferedSoundeffect;





void stopSoundeffectChannel(int chan) {
    int hwChannel = soundeffectChannel[chan].hardware_channel;
    
    soundeffectChannel[chan].hardware_channel = -1;
    soundeffectChannel[chan].frames_left = 0;
    
    if (hwChannel >= 0)
        soundKill(hwChannel);
}


void stopSoundeffects() {
    int i;
    
    for (i=0; i<MAX_SOUNDEFFECT_CHANNELS; i++)
        stopSoundeffectChannel(i);
    
    queuedBufferedSoundeffect.se = 0;
    if (requestBufferingSoundeffect.fp) {
        fclose(requestBufferingSoundeffect.fp);
        requestBufferingSoundeffect.fp = 0;
        requestBufferingSoundeffect.filename[0] = 0; // it may not have succesfully loaded in yet, so get rid of any mention of it
    }

    channelThatShouldBeLouder = 255;
}


void toggleSignBit(unsigned char *sample, unsigned int len) {
    for(; len > 0; --len, ++sample) {
        *sample ^= 0x80;
    }
}

void *searchWaveId(void *searchpos, char *id) {
    if (!strncmp(searchpos, "RIFF", strlen("RIFF")))
        searchpos += 12;
    for (;;) {
        if (!strncmp(searchpos, id, strlen(id)))
            return searchpos;
        searchpos += sizeof(struct WaveDataChunk) + (((struct WaveDataChunk *) searchpos)->chunkSize & 0xFFFF) + (((struct WaveDataChunk *) searchpos)->chunkSize & 1);
    }
}

void initWaveChunks(void *dest, enum Soundeffect se) {
    soundeffect[se] = (struct WaveChunks *) dest;
    // making sure the WaveChunks struct points to the right parts of the read in file
    soundeffect[se]->fmtChunk  = searchWaveId(dest + sizeof(struct WaveChunks), "fmt ");
    soundeffect[se]->dataChunk = searchWaveId(dest + sizeof(struct WaveChunks), "data");
    // making sure that the 8-bit wave files are converted to be signed samples instead of unsigned
    if (soundeffect[se]->fmtChunk->bitsPerSample == 8)
        toggleSignBit(((unsigned char *) soundeffect[se]->dataChunk) + sizeof(struct WaveDataChunk), soundeffect[se]->dataChunk->chunkSize & 0xFFFF);
}

unsigned int readInSoundeffect(void *dest, enum Soundeffect se, char *name) {
    unsigned int size = copyFile(dest + sizeof(struct WaveChunks), name, FS_SOUNDEFFECT);
    
    if (size == 0)
        soundeffect[se] = 0;
    else {
        initWaveChunks(dest, se);
        
        size += sizeof(struct WaveChunks);
        // making sure the next stored wav is aligned on a 4-byte boundary
        if ((size % 4) > 0)
            size += (4- (size % 4));
    }
    
    return size;
}


void updateSEafterVBlank() {
    int i;
    int result;
    struct SoundeffectChannel *curChan;
    
    for (i=0; i<MAX_SOUNDEFFECT_CHANNELS; i++) {
        curChan = &soundeffectChannel[i];
        if (curChan->frames_left && curChan->frames_left != ~0) {
            curChan->frames_left--;
            if (curChan->se >= SE_MISC) {
                if (!curChan->frames_left)
                    currentBufferedSoundeffect.sizePlayed = SOUNDEFFECTS_MISC_BUFFER_SIZE; // played all, free to overwrite everything
                else {
                    if (currentBufferedSoundeffect.sizePlayed < 2)
                        currentBufferedSoundeffect.sizePlayed++;
                    else
                        currentBufferedSoundeffect.sizePlayed += min(MAX_SOUNDEFFECT_BUFFERING_PER_FRAME, (currentBufferedSoundeffect.byteRate / FPS));
                }
            }
        }
        
        // if this is over and was the dialogues channel, reset normal volumes
        if ( ((curChan->frames_left==0) || (curChan->frames_left==~0))
              && (curChan->se >= SE_GAMETIMER) && (i==channelThatShouldBeLouder) )
            channelThatShouldBeLouder = 255;
    }
    
    if (soundCreditUnusableDuration)
        soundCreditUnusableDuration--;
    if (soundCreditsUnusableDuration)
        soundCreditsUnusableDuration--;
    if (soundEnemyUnitApproachingUnusableDuration)
        soundEnemyUnitApproachingUnusableDuration--;
    if (soundFriendlyBaseAttackedUnusableDuration)
        soundFriendlyBaseAttackedUnusableDuration--;
    if (soundPlaceStructureUnusableDuration)
        soundPlaceStructureUnusableDuration--;

// TODO: shouldn't be an ARM7 instruction but an ARM9 bookkkeeping thing. For shame! Reimplement.
//    ipcSignals->halfvolume_avoidingchannel=channelThatShouldBeLouder;
    
    if (queuedBufferedSoundeffect.se >= SE_MISC) {
        
        if (requestBufferingSoundeffect.fp) {
            result = fread(soundeffectsMiscBuffer + sizeof(struct WaveChunks) + requestBufferingSoundeffect.sizeRead,
                     1, SOUNDEFFECTS_MISC_BUFFER_SIZE - (sizeof(struct WaveChunks) + requestBufferingSoundeffect.sizeRead),
                     requestBufferingSoundeffect.fp);
            if (result > 0)
                requestBufferingSoundeffect.sizeRead += result;
            closeFile(requestBufferingSoundeffect.fp);
            requestBufferingSoundeffect.fp = 0;
        }
        
        if (requestBufferingSoundeffect.sizeRead) {
            if (!requestBufferingSoundeffect.finalized) {
                initWaveChunks(soundeffectsMiscBuffer, queuedBufferedSoundeffect.se);
                requestBufferingSoundeffect.finalized = 1;
            }
            playSoundeffectControlled(queuedBufferedSoundeffect.se, queuedBufferedSoundeffect.vp, queuedBufferedSoundeffect.sp);
            currentBufferedSoundeffect.byteRate   = soundeffect[queuedBufferedSoundeffect.se]->fmtChunk->byteRate;
            currentBufferedSoundeffect.sizePlayed = 0; // bytes 
            queuedBufferedSoundeffect.se = 0;
        }
    }
}



int soundeffectAllowedToPlay(enum Soundeffect se) {
    int i, count;
    
    if (se == SE_CREDIT) {
        if (soundCreditUnusableDuration)
            return 0;
        soundCreditUnusableDuration = ((soundeffect[se]->dataChunk->chunkSize * FPS) / soundeffect[se]->fmtChunk->byteRate) + 1;
    } else if (se == SE_CREDITS_MAXED || se == SE_CREDITS_NONE) {
        if (soundCreditsUnusableDuration)
            return 0;
        soundCreditsUnusableDuration = FPS + ((FPS * 6) / 10);
    } else if (se == SE_ENEMY_UNIT_APPROACHING) {
        if (soundEnemyUnitApproachingUnusableDuration)
            return 0;
        soundEnemyUnitApproachingUnusableDuration = 30 * FPS;
    } else if (se == SE_FRIENDLY_BASE_ATTACKED) {
        if (soundFriendlyBaseAttackedUnusableDuration)
            return 0;
        soundFriendlyBaseAttackedUnusableDuration = 30 * FPS;
    } else if (se <= SE_IMPACT_STRUCTURE) {
        count = 0;
        for (i=0; i<MAX_SOUNDEFFECT_CHANNELS; i++) {
            if (soundeffectChannel[i].se <= SE_LAST_PROJECTILE && soundeffectChannel[i].frames_left)
                count++;
        }
        if (count >= 6)
            return 0;
    } else if (se == SE_PLACE_STRUCTURE) {
        if (soundPlaceStructureUnusableDuration)
            return 0;
        soundPlaceStructureUnusableDuration = 1;
    }
    return 1;
}

int playSoundeffectControlled(enum Soundeffect se, unsigned int volumePercentage, unsigned int sp) {
    int i;
    int bestFound = -1;
    int rangeChannels =
        (se >= SE_MISC) ?      (0 | (0 << 16)) :
        ((se >= SE_POWER_ON) ? (1 | (2 << 16)) :
        ((se >= SE_EXPLOSION_STRUCTURE) ? (3 | (4 << 16)) :
                                          (5 | ((MAX_SOUNDEFFECT_CHANNELS-1) << 16))));
    
    if (soundeffect[se] == 0)
        return 0;
    
    // find the best option and make sure it's good enough
    bestFound = (rangeChannels<<16)>>16; // lowest possible channel to use for current 'se'
    for (i=bestFound; i<(rangeChannels>>16); i++) {
        if (soundeffectChannel[i].frames_left == 0) {
            bestFound = i;
            break;
        }
        if (soundeffectChannel[i].se < soundeffectChannel[bestFound].se || 
            (soundeffectChannel[i].se == soundeffectChannel[bestFound].se && soundeffectChannel[i].frames_left < soundeffectChannel[bestFound].frames_left))
            bestFound = i;
    }
    
    // it could play, but is it allowed? some sounds require a delay before they can be played again
    if (!soundeffectAllowedToPlay(se))
        return 0;

    if (soundeffectChannel[bestFound].frames_left) {
/*      if ((soundeffectChannel[bestFound].se <= SE_LAST_PROJECTILE && se <= SE_LAST_PROJECTILE) ||
            (soundeffectChannel[bestFound].se <= SE_CREDIT && se <= SE_CREDIT) ||
            (soundeffectChannel[bestFound].se <= SE_IMPACT_STRUCTURE && se <= SE_IMPACT_STRUCTURE) ||
            (soundeffectChannel[bestFound].se <= SE_FOOT_HURT && se <= SE_FOOT_HURT) ||
            (soundeffectChannel[bestFound].se <= SE_CREDITS_NONE && se <= SE_CREDITS_NONE) ||
            (soundeffectChannel[bestFound].se <= SE_STRUCTURE_SELECT && se <= SE_STRUCTURE_SELECT) ||
            (soundeffectChannel[bestFound].se <= SE_MENU_OK && se <= SE_MENU_OK) ||
            (soundeffectChannel[bestFound].se <= SE_UNIT_REPAIRED && se <= SE_UNIT_REPAIRED))
            return 0;*/
        
        // stopping sound channels will sporadically crash the game, unfortunately; must be a bug in ARM7-ARM9 communication
        // so we will only continue here to do it for the most important ingame sounds, 
        // having already reduced the risk through reserving channel ranges for specific ingame sounds
        if (getGameState() == INGAME && (se < SE_MISC || soundeffectChannel[bestFound].se > se))
            return 0;
        stopSoundeffectChannel(bestFound);
    }

    if (se >= SE_GAMETIMER)
        channelThatShouldBeLouder = bestFound;
    
    soundeffectChannel[bestFound].se = se;
    soundeffectChannel[bestFound].frames_left = (((soundeffect[se]->dataChunk->chunkSize & 0xFFFF) * (getGameState() == INGAME ? FPS : SCREEN_REFRESH_RATE)) / soundeffect[se]->fmtChunk->byteRate) + 1;
    soundeffectChannel[bestFound].hardware_channel =
        soundPlaySample(((void *) soundeffect[se]->dataChunk) + sizeof(struct WaveDataChunk), // data
                        (soundeffect[se]->fmtChunk->bitsPerSample == 16)
                           ? SoundFormat_16Bit : SoundFormat_8Bit,      // format
                        soundeffect[se]->dataChunk->chunkSize & 0xFFFF, // dataSize
                        soundeffect[se]->fmtChunk->sampleRate,          // freq
                        (volumePercentage * 127) / 100,                 // volume
                        sp,    // pan
                        false, // loop
                        0);    // loopPoint
    return 1;
}

int playSoundeffect(enum Soundeffect se) {
    return playSoundeffectControlled(se, 100, 64);
}


int playBufferedSoundeffectControlled(char *name, unsigned int volumePercentage, unsigned int sp, enum Soundeffect se) {
    int i;
    
    if (queuedBufferedSoundeffect.se > se)
        return 0;
    
    for (i=0; i<MAX_SOUNDEFFECT_CHANNELS; i++) {
        if (soundeffectChannel[i].se > se && soundeffectChannel[i].frames_left)
            return 0;
    }
    
    queuedBufferedSoundeffect.se = 0;
    
    if (strcmp(name, requestBufferingSoundeffect.filename)) { // different sound buffered, open desired one
        if (requestBufferingSoundeffect.fp)
            closeFile(requestBufferingSoundeffect.fp);
        startProfilingFunction("playBuffSEContr's oF");
        requestBufferingSoundeffect.fp = openFile(name, FS_SOUNDEFFECT);
        stopProfilingFunction();
        requestBufferingSoundeffect.sizeRead  = 0;
        requestBufferingSoundeffect.finalized = 0;
        strcpy(requestBufferingSoundeffect.filename, name);
    }
    
    if (!requestBufferingSoundeffect.fp && requestBufferingSoundeffect.sizeRead == 0)
        return 0;
    
/*    for (i=0; i<MAX_SOUNDEFFECT_CHANNELS; i++) {
        if (soundeffectChannel[i].se >= SE_MISC) {
//            stopSoundeffectChannel(i);
            soundeffectChannel[i].se = SE_MISC;
            soundeffectChannel[i].frames_left = ~0; // making sure no other sound tries to sneak in and snatch the chan
            
            channelThatShouldBeLouder = 255;
        }
    }*/
    
    queuedBufferedSoundeffect.se = se;
    queuedBufferedSoundeffect.vp = volumePercentage;
    queuedBufferedSoundeffect.sp = sp;
    return 1;
}

int playMiscSoundeffectControlled(char *name, unsigned int volumePercentage, unsigned int sp) {
    return playBufferedSoundeffectControlled(name, volumePercentage, sp, SE_MISC);
}

int playMiscSoundeffect(char *name) {
    return playMiscSoundeffectControlled(name, 100, 64);
}


int playGameTimerSoundeffectControlled(char *name, unsigned int volumePercentage, unsigned int sp) {
    return playBufferedSoundeffectControlled(name, volumePercentage, sp, SE_GAMETIMER);
}

int playGameTimerSoundeffect(char *name) {
    return playGameTimerSoundeffectControlled(name, 100, 64);
}


int playObjectiveSoundeffectControlled(char *name, unsigned int volumePercentage, unsigned int sp) {
    return playBufferedSoundeffectControlled(name, volumePercentage, sp, SE_OBJECTIVE);
}

int playObjectiveSoundeffect(char *name) {
    return playObjectiveSoundeffectControlled(name, 100, 64);
}

void requestContinueBufferingSoundeffect(char *name) {
    int result;

    if (queuedBufferedSoundeffect.se >= SE_MISC)
        return;
    
    // is the soundeffect-file the same as the one attempted to buffer
    if (!strcmp(name, requestBufferingSoundeffect.filename)) {
        // check if file isn't open currently
        if (!requestBufferingSoundeffect.fp) {
            // check if the file isn't done already loading in yet
            if (requestBufferingSoundeffect.sizeRead == 0) {
                startProfilingFunction("requestContBuffSE's oF 1");
                requestBufferingSoundeffect.fp = openFile(name, FS_SOUNDEFFECT);
                stopProfilingFunction();
            }
            return;
        }
        // continue buffering
        if (requestBufferingSoundeffect.sizeRead >= currentBufferedSoundeffect.sizePlayed)
            return;
        
        result = fread(soundeffectsMiscBuffer + sizeof(struct WaveChunks) + requestBufferingSoundeffect.sizeRead,
                       1, min(currentBufferedSoundeffect.sizePlayed - requestBufferingSoundeffect.sizeRead, MAX_SOUNDEFFECT_BUFFERING_PER_FRAME),
                       requestBufferingSoundeffect.fp);
        if (result == 0) {
            closeFile(requestBufferingSoundeffect.fp);
            requestBufferingSoundeffect.fp = 0;
        }
        if (result > 0)
            requestBufferingSoundeffect.sizeRead += result;
        return;
    }
    
    // a new soundeffect-file needs buffering. just open it for now.
    if (requestBufferingSoundeffect.fp)
        closeFile(requestBufferingSoundeffect.fp);
    startProfilingFunction("requestContBuffSE's oF 2");
    requestBufferingSoundeffect.fp = openFile(name, FS_SOUNDEFFECT);
    stopProfilingFunction();
    requestBufferingSoundeffect.sizeRead  = 0;
    requestBufferingSoundeffect.finalized = 0;
    strcpy(requestBufferingSoundeffect.filename, name);
}


void initSoundeffectsWithScenario() {
    char filename[256];
    char *secondpart;
    unsigned int size;
    int i;
    
    for (i=0; i<MAX_SOUNDEFFECT_CHANNELS; i++) {
        soundeffectChannel[i].frames_left = 0;
        soundeffectChannel[i].hardware_channel = -1;
    }
    
    soundCreditUnusableDuration = 0;
    soundCreditsUnusableDuration = 0;
    soundEnemyUnitApproachingUnusableDuration = 0;
    soundFriendlyBaseAttackedUnusableDuration = 0;
    soundPlaceStructureUnusableDuration = 0;
    
    size = sizeSoundeffectsWithoutScenario;

    strcpy(filename, factionInfo[getFaction(FRIENDLY)].name);
    secondpart = filename + strlen(factionInfo[getFaction(FRIENDLY)].name);
    
    strcpy(secondpart, "_constructionComplete");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_CONSTRUCTION_COMPLETE, filename);
//    strcpy(secondpart, "_enemyUnitDestroyed");
//    size += readInSoundeffect(soundeffectsBuffer + size, SE_ENEMY_UNIT_DESTROYED, filename);
    strcpy(secondpart, "_enemyUnitApproaching");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_ENEMY_UNIT_APPROACHING, filename);
    strcpy(secondpart, "_friendlyBaseAttacked");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_FRIENDLY_BASE_ATTACKED, filename);
    strcpy(secondpart, "_minerDeployed");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_MINER_DEPLOYED, filename);
    strcpy(secondpart, "_unitDeployed");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_UNIT_DEPLOYED, filename);
    strcpy(secondpart, "_unitRepaired");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_UNIT_REPAIRED, filename);
    
    if (size > SOUNDEFFECTS_BUFFER_SIZE)
        errorSI("The regular soundeffects buffer is too small.\nIt currently tried to load (in bytes):", size);
}

int addProjectileSoundeffect(void *dest, char *file, int projectileNr, int shot) {
    int i;
    
    // check if "None" was specified
    if (!strncmp(file, "None", strlen("None"))) {
        soundeffect[SE_PROJECTILE + 2*projectileNr + shot] = 0;
        return 0;
    }
    
    // check if the file is already stored somewhere    
    for (i=0; i<soundeffectsMiscBuffer[0]; i++) {
        if (!strcmp((char *)(soundeffectsMiscBuffer + 4 + (256+4)*i), file)) {
            soundeffect[SE_PROJECTILE + 2*projectileNr + shot] =  *((struct WaveChunks **) (soundeffectsMiscBuffer + 4 + (256+4)*i + 256));
            return 0;
        }
    }
    // otherwise read it in
    strcpy((char *)(soundeffectsMiscBuffer + 4 + (256+4)*soundeffectsMiscBuffer[0]), file);
    *((struct WaveChunks **) (soundeffectsMiscBuffer + 4 + (256+4)*soundeffectsMiscBuffer[0] + 256)) = dest;
    soundeffectsMiscBuffer[0]++;
    return readInSoundeffect(dest, SE_PROJECTILE + 2*projectileNr + shot, file);
}

void initSoundeffects() {
    unsigned int size = 0;
    FILE *fp;
    int i;
    char filename[256];
    char oneline[256];
    
    requestBufferingSoundeffect.fp = 0;
    requestBufferingSoundeffect.filename[0] = 0;
    queuedBufferedSoundeffect.se   = 0;
    
    for (i=0; i<MAX_SOUNDEFFECT_CHANNELS; i++) {
        soundeffectChannel[i].frames_left = 0;
        soundeffectChannel[i].hardware_channel = -1;
    }
    
    size += readInSoundeffect(soundeffectsBuffer + size, SE_BUTTONCLICK, "Sound_buttonClick");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_CANNOT, "Sound_cannot");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_CREDIT, "Sound_credit");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_PLACE_STRUCTURE, "Sound_placeStructure");
    
    size += readInSoundeffect(soundeffectsBuffer + size, SE_POWER_ON, "Sound_powerOn");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_POWER_OFF, "Sound_powerOff");
    
    size += readInSoundeffect(soundeffectsBuffer + size, SE_CREDITS_MAXED, "Sound_creditsMaxed");
    
    size += readInSoundeffect(soundeffectsBuffer + size, SE_IMPACT_SAND, "Sound_impactSand");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_IMPACT_UNIT, "Sound_impactUnit");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_IMPACT_STRUCTURE, "Sound_impactStructure");
    
    size += readInSoundeffect(soundeffectsBuffer + size, SE_EXPLOSION_STRUCTURE, "Sound_explosionStructure");
//    size += readInSoundeffect(soundeffectsBuffer + size, SE_EXPLOSION_UNIT_WHEELED, "Sound_explosionUnitWheeled");
//    size += readInSoundeffect(soundeffectsBuffer + size, SE_EXPLOSION_UNIT_TRACKED, "Sound_explosionUnitTracked");
    
    size += readInSoundeffect(soundeffectsBuffer + size, SE_FOOT_SELECT, "Sound_footSelect");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_FOOT_ATTACK, "Sound_footAttack");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_FOOT_MOVE, "Sound_footMove");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_FOOT_CRUSH, "Sound_footCrush");
//    size += readInSoundeffect(soundeffectsBuffer + size, SE_FOOT_SCREAM, "Sound_footScream");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_FOOT_HURT, "Sound_footHurt");
    
    size += readInSoundeffect(soundeffectsBuffer + size, SE_WHEELED_SELECT, "Sound_wheeledSelect");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_WHEELED_ATTACK, "Sound_wheeledAttack");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_WHEELED_MOVE, "Sound_wheeledMove");
    
    size += readInSoundeffect(soundeffectsBuffer + size, SE_TRACKED_SELECT, "Sound_trackedSelect");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_TRACKED_ATTACK, "Sound_trackedAttack");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_TRACKED_MOVE, "Sound_trackedMove");
    
    for (i=0; i<MAX_SE_UNIT_DEATHS; i++) {
        sprintf(filename, "Sound_unitDeath%i", i+1);
        size += readInSoundeffect(soundeffectsBuffer + size, SE_UNIT_DEATH + i, filename);
    }
    
    size += readInSoundeffect(soundeffectsBuffer + size, SE_STRUCTURE_SELECT, "Sound_structureSelect");
    
    size += readInSoundeffect(soundeffectsBuffer + size, SE_MINING, "Sound_mining");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_MINER_FULL, "Sound_minerFull");
    
    size += readInSoundeffect(soundeffectsBuffer + size, SE_MENU_SELECT, "Sound_menuSelect");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_MENU_BACK, "Sound_menuBack");
    size += readInSoundeffect(soundeffectsBuffer + size, SE_MENU_REGION, "Sound_menuRegion");
    
    soundeffect[SE_CREDITS_NONE] = soundeffect[SE_CANNOT];
    soundeffect[SE_MENU_OK] = soundeffect[SE_BUTTONCLICK];
    
    // projectile soundeffects reading in
    // temporarily storing their names in Misc soundeffects buffer to save space
    soundeffectsMiscBuffer[0] = 0; // temporarily mentions the amount of names stored in it
    for (i=0; i<MAX_DIFFERENT_PROJECTILES && projectileInfo[i].enabled; i++) {
        fp = openFile(projectileInfo[i].name, FS_PROJECTILES_INFO);
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[SOUND]", strlen("[SOUND]")));
        
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 255);
        size += addProjectileSoundeffect(soundeffectsBuffer + size, oneline + strlen("Shot1="), i, 0);
        
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 255);
        size += addProjectileSoundeffect(soundeffectsBuffer + size, oneline + strlen("Shot2="), i, 1);
        
        closeFile(fp);
    }
    
    // making sure that initSoundeffectsWithScenario knows where to pick up
    sizeSoundeffectsWithoutScenario = size;
}
#endif
