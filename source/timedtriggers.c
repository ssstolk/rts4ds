// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "timedtriggers.h"

#include <string.h>

#include "ingame_briefing.h"
#include "objectives.h"
#include "structures.h"
#include "units.h"
#include "factions.h"
#include "info.h"
#include "soundeffects.h"
#include "cutscene_debriefing.h"
#include "settings.h"
#include "shared.h"
#include "view.h"
#include "fileio.h"


#define MAX_COUNT_BG_TIMERS              60
#define MAX_COUNT_GAME_TIMERS            30
#define MAX_COUNT_STRUCTURE_KILL_TIMERS   2
#define MAX_COUNT_UNIT_KILL_TIMERS        2
#define MAX_COUNT_VIEW_COORD_TIMERS       6


struct InfoViewCoordWav {
    int  info;
    int  timer;
    int  x, y;
    char wav[256];
};

struct InfoKillWav {
    int side;
    int info;
    int timer;
    char wav[256];
};

struct InfoWavTimedtriggers {
    int  info;
    char wav[256];
};

struct Timer {
    struct InfoWavTimedtriggers bg[MAX_COUNT_BG_TIMERS];
    struct InfoWavTimedtriggers game[MAX_COUNT_GAME_TIMERS];
    struct InfoKillWav structure_kill[MAX_COUNT_STRUCTURE_KILL_TIMERS];
    struct InfoKillWav unit_kill[MAX_COUNT_UNIT_KILL_TIMERS];
    struct InfoViewCoordWav view_coord[MAX_COUNT_VIEW_COORD_TIMERS];
    int amount_bg;
    int amount_game;
    int amount_structure_kill;
    int amount_unit_kill;
    int amount_view_coord;
};


struct Timer timer;


void doTimedtriggersLogic() {
    int i, j;
    struct Structure *curStructure;
    struct Unit *curUnit;
    unsigned lowestTimer = ~0;
    char *fileToBuffer = 0;
    
    if (getIngameBriefingState() != IBS_INACTIVE)
        return;
    
    if (getObjectivesState() != OBJECTIVES_INCOMPLETE)
        return;
    
    // BG
    for (i=0; i<timer.amount_bg; i++) {
        if (timer.bg[i].info > 0) {
            // animation
            if (!strncmp(timer.bg[i].wav, "Ani=", strlen("Ani=")) || !strncmp(timer.bg[i].wav, "ani=", strlen("ani="))) {
                if (((--timer.bg[i].info) & 0xFFFF) == 0) {
                    startIngameBriefing(timer.bg[i].wav + strlen("Ani="));
                    return;
                }
            }
            // soundfile
            else {
                if (((--timer.bg[i].info) & 0xFFFF) == 0) {
                    playMiscSoundeffect(timer.bg[i].wav);
                    timer.bg[i].info = timer.bg[i].info | ((timer.bg[i].info >> 16) * FPS);
                    lowestTimer  = 0;
                    fileToBuffer = 0;
                } else if ((timer.bg[i].info & 0xFFFF) < lowestTimer) {
                    lowestTimer  = timer.bg[i].info & 0xFFFF;
                    fileToBuffer = timer.bg[i].wav;
                }
            }
        }
    }
    
    // Game
    for (i=0; i<timer.amount_game; i++) {
        if (timer.game[i].info > 0) {
            // animation
            if (!strncmp(timer.game[i].wav, "Ani=", strlen("Ani=")) || !strncmp(timer.game[i].wav, "ani=", strlen("ani="))) {
                if (((--timer.game[i].info) & 0xFFFF) == 0) {
                    startIngameBriefing(timer.game[i].wav + strlen("Ani="));
                    return;
                }
            }
            // soundfile
            else {
                if (((--timer.game[i].info) & 0xFFFF) == 0) {
                    playGameTimerSoundeffect(timer.game[i].wav);
                    timer.game[i].info = timer.game[i].info | ((timer.game[i].info >> 16) * FPS);
                    lowestTimer  = 0;
                    fileToBuffer = 0;
                } else if ((timer.game[i].info & 0xFFFF) < lowestTimer) {
                    lowestTimer  = timer.game[i].info & 0xFFFF;
                    fileToBuffer = timer.game[i].wav;
                }
            }
        }
    }
    
    // StructureKill
    for (i=0; i<timer.amount_structure_kill; i++) {
        if (timer.structure_kill[i].timer > 0) {
            // animation
            if (!strncmp(timer.structure_kill[i].wav, "Ani=", strlen("Ani=")) || !strncmp(timer.structure_kill[i].wav, "ani=", strlen("ani="))) {
                if (--timer.structure_kill[i].timer == 0) {
                    startIngameBriefingWithDelay(timer.structure_kill[i].wav + strlen("Ani="), 2*FPS);
                    for (j=0, curStructure=structure; j<MAX_STRUCTURES_ON_MAP; j++, curStructure++) {
                        if ((timer.structure_kill[i].side == MAX_DIFFERENT_FACTIONS ||
                             (timer.structure_kill[i].side == MAX_DIFFERENT_FACTIONS + 1 && curStructure->side != FRIENDLY) ||
                             timer.structure_kill[i].side == curStructure->side)
                            &&
                            (timer.structure_kill[i].info == MAX_DIFFERENT_STRUCTURES ||
                             timer.structure_kill[i].info == curStructure->info))
                        curStructure->armour = 0; // boom!
                    }
                    return;
                }
            }
            // soundfile
            else {
                if (--timer.structure_kill[i].timer == 0) {
                    playMiscSoundeffect(timer.structure_kill[i].wav);
                    for (j=0, curStructure=structure; j<MAX_STRUCTURES_ON_MAP; j++, curStructure++) {
                        if ((timer.structure_kill[i].side == MAX_DIFFERENT_FACTIONS ||
                             (timer.structure_kill[i].side == MAX_DIFFERENT_FACTIONS + 1 && curStructure->side != FRIENDLY) ||
                             timer.structure_kill[i].side == curStructure->side)
                            &&
                            (timer.structure_kill[i].info == MAX_DIFFERENT_STRUCTURES ||
                             timer.structure_kill[i].info == curStructure->info))
                        curStructure->armour = 0; // boom!
                    }
                    lowestTimer  = 0;
                    fileToBuffer = 0;
                } else if (timer.structure_kill[i].timer < lowestTimer) {
                    lowestTimer  = timer.structure_kill[i].timer;
                    fileToBuffer = timer.structure_kill[i].wav;
                }
            }
        }
    }
    
    // UnitKill
    for (i=0; i<timer.amount_unit_kill; i++) {
        if (timer.unit_kill[i].timer > 0) {
            // animation
            if (!strncmp(timer.unit_kill[i].wav, "Ani=", strlen("Ani=")) || !strncmp(timer.unit_kill[i].wav, "ani=", strlen("ani="))) {
                if (--timer.unit_kill[i].timer == 0) {
                    startIngameBriefingWithDelay(timer.unit_kill[i].wav + strlen("Ani="), 2*FPS);
                    for (j=0, curUnit=unit; j<MAX_UNITS_ON_MAP; j++, curUnit++) {
                        if ((timer.unit_kill[i].side == MAX_DIFFERENT_FACTIONS ||
                             (timer.unit_kill[i].side == MAX_DIFFERENT_FACTIONS + 1 && curUnit->side != FRIENDLY) ||
                             timer.unit_kill[i].side == curUnit->side)
                            &&
                            (timer.unit_kill[i].info == MAX_DIFFERENT_UNITS ||
                             timer.unit_kill[i].info == curUnit->info))
                        curUnit->armour = 0; // boom!
                    }
                    return;
                }
            }
            // soundfile
            else {
                if (--timer.unit_kill[i].timer == 0) {
                    playMiscSoundeffect(timer.unit_kill[i].wav);
                    for (j=0, curUnit=unit; j<MAX_UNITS_ON_MAP; j++, curUnit++) {
                        if ((timer.unit_kill[i].side == MAX_DIFFERENT_FACTIONS ||
                             (timer.unit_kill[i].side == MAX_DIFFERENT_FACTIONS + 1 && curUnit->side != FRIENDLY) ||
                             timer.unit_kill[i].side == curUnit->side)
                            &&
                            (timer.unit_kill[i].info == MAX_DIFFERENT_UNITS ||
                             timer.unit_kill[i].info == curUnit->info))
                        curUnit->armour = 0; // boom!
                    }
                    lowestTimer  = 0;
                    fileToBuffer = 0;
                } else if (timer.unit_kill[i].timer < lowestTimer) {
                    lowestTimer  = timer.unit_kill[i].timer;
                    fileToBuffer = timer.unit_kill[i].wav;
                }
            }
        }
    }
    
    // ViewCoord
    for (i=0; i<timer.amount_view_coord; i++) {
        if (timer.view_coord[i].timer == 0) {
            if (timer.view_coord[i].info > 0) {
                setViewCurrentXY(timer.view_coord[i].x - (HORIZONTAL_WIDTH/2), timer.view_coord[i].y - ((HORIZONTAL_HEIGHT/2) - 1));
                timer.view_coord[i].info--;
            }
        }
        if (timer.view_coord[i].timer > 0) {
            if (--timer.view_coord[i].timer == 0) {
                setViewCurrentXY(timer.view_coord[i].x - (HORIZONTAL_WIDTH/2), timer.view_coord[i].y - ((HORIZONTAL_HEIGHT/2) - 1));
                // animation
                if (!strncmp(timer.view_coord[i].wav, "Ani=", strlen("Ani=")) || !strncmp(timer.view_coord[i].wav, "ani=", strlen("ani="))) {
                    startIngameBriefing(timer.view_coord[i].wav + strlen("Ani="));
                    return;
                }
                // soundfile
                else {
                    playObjectiveSoundeffect(timer.view_coord[i].wav);
                }
            }
        }
    }
    
    if (fileToBuffer)
        requestContinueBufferingSoundeffect(fileToBuffer);
}


void initTimedtriggers() {
    char oneline[256];
    char *charPosition;
    int i, j;
    int len;
    int gameSpeed = getGameSpeed();
    FILE *fp;

    timer.amount_bg = 0;
    timer.amount_game = 0;
    timer.amount_structure_kill = 0;
    timer.amount_unit_kill = 0;
    timer.amount_view_coord = 0;
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
//fp = fopen("/testscenario.ini", "rb");
    
    // TIMERS section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[TIMERS]", strlen("[TIMERS]")) && strncmp(oneline, "[OBJECTIVES]", strlen("[OBJECTIVES]")));
    
    if (!strncmp(oneline, "[OBJECTIVES]", strlen("[OBJECTIVES]"))) {
        closeFile(fp);
        return;
    }
    
    readstr(fp, oneline);
    replaceEOLwithEOF(oneline, 256);
    
    // BG
    for (i=0; i<MAX_COUNT_BG_TIMERS; i++) {
        if (strncmp(oneline, "BG", strlen("BG")))
            break;
        timer.bg[i].info = 0;
        timer.bg[i].wav[0] = 0;
        sscanf(oneline, "BG=%i, %s", &timer.bg[i].info, timer.bg[i].wav);
        timer.bg[i].info *= FPS;
        charPosition = strchr(timer.bg[i].wav, ',');
        if (charPosition != NULL) {
            sscanf(charPosition, ", %i", &j);
            timer.bg[i].info |= (j << 16);
            *charPosition = 0;
        }
        timer.amount_bg++;
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
    }
    
//    if (strncmp(oneline, "BG", strlen("BG")) == 0)
//        errorSI("The number of BG timers exceeded the max limit of", MAX_COUNT_BG_TIMERS);
    
    // Game
    for (i=0; i<MAX_COUNT_GAME_TIMERS; i++) {        
        if (strncmp(oneline, "Game", strlen("Game")))
            break;
        timer.game[i].info = 0;
        timer.game[i].wav[0] = 0;
        sscanf(oneline, "Game=%i, %s", &timer.game[i].info, timer.game[i].wav);
        timer.game[i].info = (2*(timer.game[i].info * FPS)) / gameSpeed;
        charPosition = strchr(timer.game[i].wav, ',');
        if (charPosition != NULL) {
            sscanf(charPosition, ", %i", &j);
            timer.game[i].info |= (j << 16);
            *charPosition = 0;
        }
        timer.amount_game++;
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
    }
    
    // StructureKill
    for (i=0; i<MAX_COUNT_STRUCTURE_KILL_TIMERS; i++) {
        if (strncmp(oneline, "StructureKill", strlen("StructureKill")))
            break;
        len = strlen("StructureKill") + nrstrlen(i + 1) + 1;
        timer.structure_kill[i].wav[0] = 0;
        sscanf(oneline + len, "%i,", &timer.structure_kill[i].timer);
        len += nrstrlen(timer.structure_kill[i].timer) + 1;
        timer.structure_kill[i].timer = (2*(timer.structure_kill[i].timer * FPS)) / gameSpeed;
        if (!strncmp(oneline + len, "Any", strlen("Any"))) {
            timer.structure_kill[i].side = MAX_DIFFERENT_FACTIONS;
            len += strlen("Any") + 1;
        } else if (!strncmp(oneline + len, "Friendly", strlen("Friendly"))) {
            timer.structure_kill[i].side = FRIENDLY;
            len += strlen("Friendly") + 1;
        } else {
            if (!sscanf(oneline + len, "Enemy%i,", &timer.structure_kill[i].side))
                timer.structure_kill[i].side = MAX_DIFFERENT_FACTIONS + 1;
            else
                len += 1;
            len += strlen("Enemy") + 1;
        }
        if (!strncmp(oneline + len, "Any", strlen("Any"))) {
            timer.structure_kill[i].info = MAX_DIFFERENT_STRUCTURES;
            len += strlen("Any");
        } else {
            timer.structure_kill[i].info = getStructureNameInfo(oneline + len);
            if (timer.structure_kill[i].info >= 0)
                len += strlen(structureInfo[timer.structure_kill[i].info].name);
        }
        if (oneline[len] == ',') // a wav was mentioned
            strcpy(timer.structure_kill[i].wav, oneline + len + 2); // 2 because we're going to skip ", "
        
        timer.amount_structure_kill++;
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
    }
    
    // UnitKill
    for (i=0; i<MAX_COUNT_UNIT_KILL_TIMERS; i++) {
        if (strncmp(oneline, "UnitKill", strlen("UnitKill")))
            break;
        len = strlen("UnitKill") + nrstrlen(i + 1) + 1;
        timer.unit_kill[i].wav[0] = 0;
        sscanf(oneline + len, "%i,", &timer.unit_kill[i].timer);
        len += nrstrlen(timer.unit_kill[i].timer) + 1;
        timer.unit_kill[i].timer = (2*(timer.unit_kill[i].timer * FPS)) / gameSpeed;
        if (!strncmp(oneline + len, "Any", strlen("Any"))) {
            timer.unit_kill[i].side = MAX_DIFFERENT_FACTIONS;
            len += strlen("Any") + 1;
        } else if (!strncmp(oneline + len, "Friendly", strlen("Friendly"))) {
            timer.unit_kill[i].side = FRIENDLY;
            len += strlen("Friendly") + 1;
        } else {
            if (!sscanf(oneline + len, "Enemy%i,", &timer.unit_kill[i].side))
                timer.unit_kill[i].side = MAX_DIFFERENT_FACTIONS + 1;
            else
                len += 1;
            len += strlen("Enemy") + 1;
        }
        if (!strncmp(oneline + len, "Any", strlen("Any"))) {
            timer.unit_kill[i].info = MAX_DIFFERENT_UNITS;
            len += strlen("Any");
        } else {
            timer.unit_kill[i].info = getUnitNameInfo(oneline + len);
            if (timer.unit_kill[i].info >= 0)
                len += strlen(unitInfo[timer.unit_kill[i].info].name);
        }
        if (oneline[len] == ',') // a wav was mentioned
            strcpy(timer.unit_kill[i].wav, oneline + len + 2); // 2 because we're going to skip ", "
        
        timer.amount_unit_kill++;
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
    }
    
    // ViewCoord
    for (i=0; i<MAX_COUNT_VIEW_COORD_TIMERS; i++) {
        if (strncmp(oneline, "ViewCoord", strlen("ViewCoord")))
            break;
        len = strlen("ViewCoord") + nrstrlen(i + 1) + 1;
        timer.view_coord[i].wav[0] = 0;
        sscanf(oneline + len, "%i,", &timer.view_coord[i].timer);
        len += nrstrlen(timer.view_coord[i].timer) + 1;
        timer.view_coord[i].timer = (2*(timer.view_coord[i].timer * FPS)) / gameSpeed;
        sscanf(oneline + len, "%i,%i,%i, %s", &timer.view_coord[i].x, &timer.view_coord[i].y, &timer.view_coord[i].info, timer.view_coord[i].wav);
        timer.view_coord[i].info = (2*(timer.view_coord[i].info * FPS)) / gameSpeed;
        timer.amount_view_coord++;
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
    }
    
    closeFile(fp);
}

inline struct Timer *getTimedTriggers() {
    return &timer;
}

int getTimedTriggersSaveSize(void) {
    return sizeof(timer);
}

int getTimedTriggersSaveData(void *dest, int max_size) {
    int size=sizeof(timer);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&timer,size);
    return (size);
}
