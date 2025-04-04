// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "objectives.h"

#include <string.h>

#include "ingame_briefing.h"
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


#define OBJECTIVES_INGAME_DELAY  (5 * FPS)

#define MAX_OBJECTIVES_COUNT_NEED       3
#define MAX_OBJECTIVES_COUNT_KILL       3
#define MAX_OBJECTIVES_COUNT_GETTO      6
#define MAX_OBJECTIVES_COUNT_RESOURCES  2
#define MAX_OBJECTIVES_COUNT_COLLECTIBLES  4


struct InfoSideGetToWav {
    int  side;
    int  info;
    int  x1, y1, x2, y2;
    char wav[256];
};

struct InfoSideWav {
    int  side;
    int  info;
    char wav[256];
};

struct InfoCollectiblesWav {
    int  info;
    int  x, y;
    int  value;
    char wav[256];
};

struct InfoWavObjectives {
    int  info;
    char wav[256];
};


struct EntityObjectives {
    struct InfoSideWav          need[MAX_OBJECTIVES_COUNT_NEED];
    struct InfoWavObjectives    kill[MAX_OBJECTIVES_COUNT_KILL];
    struct InfoSideGetToWav     get_to[MAX_OBJECTIVES_COUNT_GETTO];
    struct InfoCollectiblesWav  collect[MAX_OBJECTIVES_COUNT_COLLECTIBLES];
    int amount_need;    int opt_amount_need;
    int amount_kill;    int opt_amount_kill;
    int amount_get_to;  int opt_amount_get_to;
    int amount_collect; int opt_amount_collect;
};

struct ResourceObjectives {
    struct InfoWavObjectives need[MAX_OBJECTIVES_COUNT_RESOURCES];    
    int amount_need;    int opt_amount_need;
};

struct KillAllObjective {
    char win_wav[256];
    char lose_wav[256];
};

struct EntityObjectives structureObjective;
struct EntityObjectives unitObjective;
struct ResourceObjectives resourceObjective;
struct KillAllObjective killAllObjective;

enum ObjectivesState objectivesState;
int objectivesStateTimer;

int specifiedObjectiveExists;



enum ObjectivesState getObjectivesState() {
    return objectivesState;
}


void doObjectivesLogic() {
    struct Structure *curStructure;
    struct Unit *curUnit;
    int i, j, k;
    int specifiedObjectivesAccomplished = 1;
    enum Side neutralSide = getNeutralSide();
    char filename[256];
    
    if (getIngameBriefingState() != IBS_INACTIVE)
        return;
    
    if (objectivesState != OBJECTIVES_INCOMPLETE) {
        objectivesStateTimer++;
        if (objectivesStateTimer < OBJECTIVES_INGAME_DELAY) {
            // fade main screen gradually to win (white:1) or lose (black:2) for 4/5th of the delay
            if (objectivesStateTimer == ((OBJECTIVES_INGAME_DELAY*4)/5)) {
                i = (((objectivesState == OBJECTIVES_FAILED) ? 2 : 1)<<14) | 16;
                REG_MASTER_BRIGHT = i;
                REG_MASTER_BRIGHT_SUB = i;
            } else if (objectivesStateTimer < ((OBJECTIVES_INGAME_DELAY*4)/5)) {
                i = (((objectivesStateTimer-1) * 16)*5) / (OBJECTIVES_INGAME_DELAY*4); // currently displayed state
                j = (((objectivesStateTimer  ) * 16)*5) / (OBJECTIVES_INGAME_DELAY*4); // desired displayed state
                if (j != i) {
                    i = (((objectivesState == OBJECTIVES_FAILED) ? 2 : 1)<<14) | j;
                    REG_MASTER_BRIGHT = i;
                    REG_MASTER_BRIGHT_SUB = i;
                }
            }
            return;
        }

        setScreenSetUp(MAIN_TOUCH); // does not matter which as touch, as long as not swapping
        if (objectivesState == OBJECTIVES_FAILED) {
            stopSoundeffects();
            initCutsceneDebriefingFilename(0);
            setGameState(CUTSCENE_DEBRIEFING);
        } else { // OBJECTIVES_COMPLETED
            stopSoundeffects();
            initCutsceneDebriefingFilename(1);
            setLevel(getLevel() + 1);
            setLevelCurrent(getFaction(FRIENDLY), getLevel());
            if (getLevelReached(getFaction(FRIENDLY)) < getLevel())
                setLevelReached(getFaction(FRIENDLY), getLevel());
            writeSaveFile();
            setGameState(CUTSCENE_DEBRIEFING);
        }
        return;
    }
    
    // StructureNeed; can only cause a fail
    for (i=0; i<structureObjective.amount_need + structureObjective.opt_amount_need; i++) {
        if (structureObjective.need[i].info >= 0) {
//          specifiedObjectiveExists = 1;
            if (structureObjective.need[i].info == MAX_DIFFERENT_STRUCTURES) {
                if (structureObjective.need[i].side < MAX_DIFFERENT_FACTIONS)
                    j = MAX_STRUCTURES_ON_MAP * (getStructureDeaths(structureObjective.need[i].side) > 0);
                else {
                    for (j=1; j<MAX_DIFFERENT_FACTIONS && factionInfo[j].enabled; j++) {
                        if (getStructureDeaths(structureObjective.need[j].side) > 0) {
                            j = MAX_STRUCTURES_ON_MAP;
                            break;
                        }
                    }
                }
            }
            else {
                if (structureObjective.need[i].side < MAX_DIFFERENT_FACTIONS) {
                    for (j=0, curStructure=structure; j<MAX_STRUCTURES_ON_MAP; j++, curStructure++) {
                        if (curStructure->enabled && (curStructure->side == structureObjective.need[i].side || curStructure->side == neutralSide) && curStructure->info == structureObjective.need[i].info)
                            break;
                    }
                } else {
                    for (j=0, curStructure=structure; j<MAX_STRUCTURES_ON_MAP; j++, curStructure++) {
                        if (curStructure->enabled && curStructure->side != FRIENDLY && curStructure->info == structureObjective.need[i].info)
                            break;
                    }
                }
            }
            if (j == MAX_STRUCTURES_ON_MAP) {
                structureObjective.need[i].info = -1; // it was taken care of
                if (i < structureObjective.amount_need)
                    objectivesState = OBJECTIVES_FAILED;
                
                if (!strncmp(structureObjective.need[i].wav, "Ani=", strlen("Ani=")) || !strncmp(structureObjective.need[i].wav, "ani=", strlen("ani="))) {
                    startIngameBriefingWithDelay(structureObjective.need[i].wav + strlen("Ani="), 2*FPS);
                    return;
                } else
                    playObjectiveSoundeffect(structureObjective.need[i].wav);
                
                if (objectivesState == OBJECTIVES_FAILED)
                    return;
            }
        }
    }
    
    // StructureKill
    for (i=0; i<structureObjective.amount_kill + structureObjective.opt_amount_kill; i++) {
        if (structureObjective.kill[i].info >= 0) {
            if (i < structureObjective.amount_kill)
                specifiedObjectiveExists = 1;
            if (structureObjective.kill[i].info == MAX_DIFFERENT_STRUCTURES) {
                j = 0;
                for (k=1; k<getAmountOfSides(); k++)
                    j += getStructureDeaths(k);
                j = MAX_STRUCTURES_ON_MAP * (j > 0);
            } else {
                for (j=0, curStructure=structure; j<MAX_STRUCTURES_ON_MAP; j++, curStructure++) {
                    if (curStructure->enabled && curStructure->side != FRIENDLY && curStructure->info == structureObjective.kill[i].info)
                        break;
                }
            }
            if (j == MAX_STRUCTURES_ON_MAP) {
                structureObjective.kill[i].info = -1; // it was taken care of
                if (!strncmp(structureObjective.kill[i].wav, "Ani=", strlen("Ani=")) || !strncmp(structureObjective.kill[i].wav, "ani=", strlen("ani="))) {
                    startIngameBriefingWithDelay(structureObjective.kill[i].wav + strlen("Ani="), 2*FPS);
                    return;
                } else
                    playObjectiveSoundeffect(structureObjective.kill[i].wav);
            } else if (i < structureObjective.amount_kill)
                specifiedObjectivesAccomplished = 0;
        }
    }
    
    // UnitNeed; can only cause a fail
    for (i=0; i<unitObjective.amount_need + unitObjective.opt_amount_need; i++) {
        if (unitObjective.need[i].info >= 0) {
//          specifiedObjectiveExists = 1;
            if (unitObjective.need[i].info == MAX_DIFFERENT_UNITS) {
                if (unitObjective.need[i].side < MAX_DIFFERENT_FACTIONS)
                    j = MAX_UNITS_ON_MAP * (getUnitDeaths(unitObjective.need[i].side) > 0);
                else {
                    for (j=1; j<MAX_DIFFERENT_FACTIONS && factionInfo[j].enabled; j++) {
                        if (getUnitDeaths(unitObjective.need[j].side) > 0) {
                            j = MAX_UNITS_ON_MAP;
                            break;
                        }
                    }
                }
            }
            else {
                if (unitObjective.need[i].side < MAX_DIFFERENT_FACTIONS) {
                    for (j=0, curUnit=unit; j<MAX_UNITS_ON_MAP; j++, curUnit++) {
                        if (curUnit->enabled && (curUnit->side == unitObjective.need[i].side || curUnit->side == neutralSide) && curUnit->info == unitObjective.need[i].info)
                            break;
                    }
                } else {
                    for (j=0, curUnit=unit; j<MAX_UNITS_ON_MAP; j++, curUnit++) {
                        if (curUnit->enabled && curUnit->side != FRIENDLY && curUnit->info == unitObjective.need[i].info)
                            break;
                    }
                }
            }
            if (j == MAX_UNITS_ON_MAP) {
                unitObjective.need[i].info = -1; // it was taken care of
                if (i < unitObjective.amount_need)
                    objectivesState = OBJECTIVES_FAILED;
                
                if (!strncmp(unitObjective.need[i].wav, "Ani=", strlen("Ani=")) || !strncmp(unitObjective.need[i].wav, "ani=", strlen("ani="))) {
                    startIngameBriefingWithDelay(unitObjective.need[i].wav + strlen("Ani="), 2*FPS);
                    return;
                } else
                    playObjectiveSoundeffect(unitObjective.need[i].wav);
                
                if (objectivesState == OBJECTIVES_FAILED)
                    return;
            }
        }
    }
    
    // UnitKill
    for (i=0; i<unitObjective.amount_kill + unitObjective.opt_amount_kill; i++) {
        if (unitObjective.kill[i].info >= 0) {
            if (i < unitObjective.amount_kill)
                specifiedObjectiveExists = 1;
            if (unitObjective.kill[i].info == MAX_DIFFERENT_UNITS) {
                j = 0;
                for (k=1; k<getAmountOfSides(); k++)
                    j += getUnitDeaths(k);
                j = MAX_UNITS_ON_MAP * (j > 0);
            } else {
                for (j=0, curUnit=unit; j<MAX_UNITS_ON_MAP; j++, curUnit++) {
                    if (curUnit->enabled && curUnit->side != FRIENDLY && curUnit->info == unitObjective.kill[i].info)
                        break;
                }
            }
            if (j == MAX_UNITS_ON_MAP) {
                unitObjective.kill[i].info = -1; // it was taken care of
                if (!strncmp(unitObjective.kill[i].wav, "Ani=", strlen("Ani=")) || !strncmp(unitObjective.kill[i].wav, "ani=", strlen("ani="))) {
                    startIngameBriefingWithDelay(unitObjective.kill[i].wav + strlen("Ani="), 2*FPS);
                    return;
                } else
                    playObjectiveSoundeffect(unitObjective.kill[i].wav);
            } else if (i < unitObjective.amount_kill)
                specifiedObjectivesAccomplished = 0;
        }
    }
    
    // StructureTo
    for (i=0; i<structureObjective.amount_get_to + structureObjective.opt_amount_get_to; i++) {
        if (structureObjective.get_to[i].info >= 0) {
            if (i < structureObjective.amount_get_to)
                specifiedObjectiveExists = 1;
            if (structureObjective.get_to[i].info == MAX_DIFFERENT_STRUCTURES) {
                for (j=0, curStructure=structure; j<MAX_STRUCTURES_ON_MAP; j++, curStructure++) {
                    if (curStructure->enabled &&
                        (curStructure->side == structureObjective.get_to[i].side ||
                        (curStructure->side != FRIENDLY && structureObjective.get_to[i].side == MAX_DIFFERENT_FACTIONS + 1)) &&
                        curStructure->x >= structureObjective.get_to[i].x1 && curStructure->x <= structureObjective.get_to[i].x2 &&
                        curStructure->y >= structureObjective.get_to[i].y1 && curStructure->y <= structureObjective.get_to[i].y2)
                        break;
                }
            } else {
                for (j=0, curStructure=structure; j<MAX_STRUCTURES_ON_MAP; j++, curStructure++) {
                    if (curStructure->enabled && curStructure->info == structureObjective.get_to[i].info &&
                        (curStructure->side == structureObjective.get_to[i].side ||
                        (curStructure->side != FRIENDLY && structureObjective.get_to[i].side == MAX_DIFFERENT_FACTIONS + 1)) &&
                        curStructure->x >= structureObjective.get_to[i].x1 && curStructure->x <= structureObjective.get_to[i].x2 &&
                        curStructure->y >= structureObjective.get_to[i].y1 && curStructure->y <= structureObjective.get_to[i].y2)
                        break;
                }
            }
            if (j == MAX_STRUCTURES_ON_MAP) {
                if (i < structureObjective.amount_get_to)
                    specifiedObjectivesAccomplished = 0;
            } else {
                structureObjective.get_to[i].info = -1; // it was taken care of
                if (structureObjective.get_to[i].side != FRIENDLY && i < structureObjective.amount_get_to) // an enemy filled the objective and won instantly!
                    objectivesState = OBJECTIVES_FAILED;
                
                if (!strncmp(structureObjective.get_to[i].wav, "Ani=", strlen("Ani=")) || !strncmp(structureObjective.get_to[i].wav, "ani=", strlen("ani="))) {
                    startIngameBriefingWithDelay(structureObjective.get_to[i].wav + strlen("Ani="), 2*FPS);
                    return;
                } else
                    playObjectiveSoundeffect(structureObjective.get_to[i].wav);
                
                if (objectivesState == OBJECTIVES_FAILED)
                    return;
            }
        }
    }
    
    // UnitTo
    for (i=0; i<unitObjective.amount_get_to + unitObjective.opt_amount_get_to; i++) {
        if (unitObjective.get_to[i].info >= 0) {
            if (i < unitObjective.amount_get_to)
                specifiedObjectiveExists = 1;
            if (unitObjective.get_to[i].info == MAX_DIFFERENT_UNITS) {
                for (j=0, curUnit=unit; j<MAX_UNITS_ON_MAP; j++, curUnit++) {
                    if (curUnit->enabled &&
                        (curUnit->side == unitObjective.get_to[i].side ||
                        (curUnit->side != FRIENDLY && unitObjective.get_to[i].side == MAX_DIFFERENT_FACTIONS + 1)) &&
                        curUnit->x >= unitObjective.get_to[i].x1 && curUnit->x <= unitObjective.get_to[i].x2 &&
                        curUnit->y >= unitObjective.get_to[i].y1 && curUnit->y <= unitObjective.get_to[i].y2)
                        break;
                }
            } else {
                for (j=0, curUnit=unit; j<MAX_UNITS_ON_MAP; j++, curUnit++) {
                    if (curUnit->enabled && curUnit->info == unitObjective.get_to[i].info &&
                        (curUnit->side == unitObjective.get_to[i].side ||
                        (curUnit->side != FRIENDLY && unitObjective.get_to[i].side == MAX_DIFFERENT_FACTIONS + 1)) &&
                        curUnit->x >= unitObjective.get_to[i].x1 && curUnit->x <= unitObjective.get_to[i].x2 &&
                        curUnit->y >= unitObjective.get_to[i].y1 && curUnit->y <= unitObjective.get_to[i].y2)
                        break;
                }
            }
            if (j == MAX_UNITS_ON_MAP) {
                if (i < unitObjective.amount_get_to)
                    specifiedObjectivesAccomplished = 0;
            } else {
                unitObjective.get_to[i].info = -1; // it was taken care of
                if (unitObjective.get_to[i].side != FRIENDLY && i < unitObjective.amount_get_to) // an enemy filled the objective and won instantly!
                    objectivesState = OBJECTIVES_FAILED;
                
                if (!strncmp(unitObjective.get_to[i].wav, "Ani=", strlen("Ani=")) || !strncmp(unitObjective.get_to[i].wav, "ani=", strlen("ani="))) {
                    startIngameBriefingWithDelay(unitObjective.get_to[i].wav + strlen("Ani="), 2*FPS);
                    return;
                } else
                    playObjectiveSoundeffect(unitObjective.get_to[i].wav);
                
                if (objectivesState == OBJECTIVES_FAILED)
                    return;
            }
        }
    }
    
    // Collectibles
    for (i=0; i<unitObjective.amount_collect + unitObjective.opt_amount_collect; i++) {
        if (unitObjective.collect[i].info == 0) {  // check if active
            if (i < unitObjective.amount_collect)
                specifiedObjectiveExists = 1;
            
            // check if there's a unit surrounding this location
            for (j=(unitObjective.collect[i].y-1);j<=(unitObjective.collect[i].y+1);j++) {
                if ((j>=0) && (j<environment.height)) {
                    for (k=(unitObjective.collect[i].x-1);k<=(unitObjective.collect[i].x+1);k++) {
                        if ((k>=0) && (k<environment.width)) {
                            if (environment.layout[TILE_FROM_XY(k,j)].contains_unit>=0) {
                                curUnit = unit + (environment.layout[TILE_FROM_XY(k,j)].contains_unit);
                                // the unit should be NOT MOVING (it reached the centre of the tile)
                                if ((curUnit->move & UM_MASK) == UM_NONE) {
                                    if (i < unitObjective.amount_collect)
                                        specifiedObjectivesAccomplished = 0;
                                    
                                    // award the value (even if it goes OVER limit)!
                                    changeCreditsNoLimit(curUnit->side, unitObjective.collect[i].value);
                                    
                                    // now DESTROY the structure at that location (no, really!)
                                    if (environment.layout[TILE_FROM_XY(unitObjective.collect[i].x, unitObjective.collect[i].y)].contains_structure>=0) {
                                        // this should be always true, otherwise it means the structure has been
                                        // forgotten OR it's bigger than 1x1 tile, which shouldn't be possible
                                        curStructure = structure + (environment.layout[TILE_FROM_XY(unitObjective.collect[i].x, unitObjective.collect[i].y)].contains_structure);
                                        curStructure->armour = -1; // destroy this structure
                                    }
                                    
                                    // it was taken care of
                                    unitObjective.collect[i].info = -1;
                                    
                                    // if an enemy filled a req. objective you lose instantly!
                                    if (curUnit->side != FRIENDLY && i < unitObjective.amount_collect)
                                        objectivesState = OBJECTIVES_FAILED;
                                    
                                    if (!strncmp(unitObjective.collect[i].wav, "Ani=", strlen("Ani=")) || !strncmp(unitObjective.collect[i].wav, "ani=", strlen("ani="))) {
                                        startIngameBriefingWithDelay(unitObjective.collect[i].wav + strlen("Ani="), 2*FPS);
                                        return;
                                    } else
                                        playObjectiveSoundeffect(unitObjective.collect[i].wav);
                                    
                                    // if an enemy filled a req. objective you lose instantly!
                                    if (objectivesState == OBJECTIVES_FAILED)
                                        return;
                                }
                            }
                        }
                        
                        // check if we should go on searching
                        if (unitObjective.collect[i].info!=0)
                            break;
                    }
                }
                
                // check if we should go on searching
                if (unitObjective.collect[i].info!=0)
                    break;
            }
        }
    }
    
    // Resources
    for (i=0; i<resourceObjective.amount_need + resourceObjective.opt_amount_need; i++) {
        if (resourceObjective.need[i].info > 0) {
            if (i >= resourceObjective.amount_need) {
                if (getCredits(FRIENDLY) >= resourceObjective.need[i].info) {
                    resourceObjective.need[i].info = 0; // it was taken care of
                    if (!strncmp(resourceObjective.need[i].wav, "Ani=", strlen("Ani=")) || !strncmp(resourceObjective.need[i].wav, "ani=", strlen("ani="))) {
                        startIngameBriefingWithDelay(resourceObjective.need[i].wav + strlen("Ani="), 2*FPS);
                        return;
                    } else
                        playObjectiveSoundeffect(resourceObjective.need[i].wav);
                }
            } else {
                specifiedObjectiveExists = 1;
                if (getCredits(FRIENDLY) < resourceObjective.need[i].info)
                    specifiedObjectivesAccomplished = 0;
                else {
                    resourceObjective.need[i].info = 0; // it was taken care of
                    if (!strncmp(resourceObjective.need[i].wav, "Ani=", strlen("Ani=")) || !strncmp(resourceObjective.need[i].wav, "ani=", strlen("ani="))) {
                        startIngameBriefingWithDelay(resourceObjective.need[i].wav + strlen("Ani="), 2*FPS);
                        return;
                    } else
                        playObjectiveSoundeffect(resourceObjective.need[i].wav);
                }
            }
        }
    }
    
    // were all objectives accomplished?
    
    if (specifiedObjectiveExists && specifiedObjectivesAccomplished) {
        objectivesState = OBJECTIVES_COMPLETED;
        return;
    }
    
    // player can always lose by having been completely destroyed
    
    specifiedObjectivesAccomplished = 1;
    for (i=0, curStructure=structure; i<MAX_STRUCTURES_ON_MAP; i++, curStructure++) {
        if (curStructure->enabled && curStructure->side == FRIENDLY && !structureInfo[curStructure->info].foundation && !structureInfo[curStructure->info].barrier && curStructure->x < environment.width && curStructure->y < environment.height)
            break;
    }
    if (i == MAX_STRUCTURES_ON_MAP) {
        for (i=0, curUnit=unit; i<MAX_UNITS_ON_MAP; i++, curUnit++) {
            if (curUnit->enabled && curUnit->side == FRIENDLY)
                break;
        }
        if (i == MAX_UNITS_ON_MAP)
            specifiedObjectivesAccomplished = 0;
    }
    
    if (!specifiedObjectivesAccomplished) {
        objectivesState = OBJECTIVES_FAILED;
        if (*killAllObjective.lose_wav) {
            if (!strncmp(killAllObjective.lose_wav, "Ani=", strlen("Ani=")) || !strncmp(killAllObjective.lose_wav, "ani=", strlen("ani=")))
                startIngameBriefingWithDelay(killAllObjective.lose_wav + strlen("Ani="), 2*FPS);
            else
                playObjectiveSoundeffect(killAllObjective.lose_wav);
        } else {
            sprintf(filename, "%s_lose", factionInfo[getFaction(ENEMY1)].name);
            playObjectiveSoundeffect(filename);
        }
        return;
    }
    
    // if no objectives were specified apart from Opts, the player needs to win by destroying the enemy
    
    if (!specifiedObjectiveExists) {
        for (i=0, curStructure=structure; i<MAX_STRUCTURES_ON_MAP; i++, curStructure++) {
            if (curStructure->enabled && curStructure->side != FRIENDLY && curStructure->side != getNeutralSide() && !structureInfo[curStructure->info].foundation && !structureInfo[curStructure->info].barrier && curStructure->x < environment.width && curStructure->y < environment.height)
                return;
        }
        for (i=0, curUnit=unit; i<MAX_UNITS_ON_MAP; i++, curUnit++) {
            if (curUnit->enabled && curUnit->side != FRIENDLY && curUnit->side != getNeutralSide())
                return;
        }
        objectivesState = OBJECTIVES_COMPLETED;
        if (*killAllObjective.win_wav) {
            if (!strncmp(killAllObjective.win_wav, "Ani=", strlen("Ani=")) || !strncmp(killAllObjective.win_wav, "ani=", strlen("ani=")))
                startIngameBriefingWithDelay(killAllObjective.win_wav + strlen("Ani="), 2*FPS);
            else
                playObjectiveSoundeffect(killAllObjective.win_wav);
        } else {
            sprintf(filename, "%s_win", factionInfo[getFaction(FRIENDLY)].name);
            playObjectiveSoundeffect(filename);
        }
        return;
    }
}


void initObjectives() {
    char oneline[256];
    char *cur;
    //char *charPosition;
    int len;
    int i; //, j;
    FILE *fp;
    
    specifiedObjectiveExists = 0;
    
    structureObjective.amount_need = 0;    structureObjective.opt_amount_need = 0;
    structureObjective.amount_kill = 0;    structureObjective.opt_amount_kill = 0;
    structureObjective.amount_get_to = 0;  structureObjective.opt_amount_get_to = 0;
    unitObjective.amount_need = 0;         unitObjective.opt_amount_need = 0;
    unitObjective.amount_kill = 0;         unitObjective.opt_amount_kill = 0;
    unitObjective.amount_get_to = 0;       unitObjective.opt_amount_get_to = 0;
    unitObjective.amount_collect = 0;      unitObjective.opt_amount_collect = 0;
    resourceObjective.amount_need = 0;     resourceObjective.opt_amount_need = 0;
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
//fp = fopen("/testscenario.ini", "rb");
    
    // OBJECTIVES section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[OBJECTIVES]", strlen("[OBJECTIVES]")));
    
    readstr(fp, oneline);
    replaceEOLwithEOF(oneline, 256);
    
    // Timer; if encountered, error out. sorry LDAsh.
    if (!strncmp(oneline, "Timer", strlen("Timer")))
        error("timers now belong in [TIMERS] section", oneline);
    
    cur = (strncmp(oneline, "Opt.", strlen("Opt."))) ? (oneline) : (oneline + strlen("Opt."));

    // StructureNeed
    for (i=0; i<MAX_OBJECTIVES_COUNT_NEED; i++) {
        if (strncmp(cur, "StructureNeed", strlen("StructureNeed")))
            break;
        len = strlen("StructureNeed") + nrstrlen(i + 1) + 1;
        structureObjective.need[i].side = FRIENDLY;
        structureObjective.need[i].wav[0] = 0;
        
        if (!strncmp(cur + len, "Friendly", strlen("Friendly"))) {
            len += strlen("Friendly") + 1;
        } else if (!strncmp(cur + len, "Enemy", strlen("Enemy"))) {
            if (!sscanf(cur + len, "Enemy%i,", &structureObjective.need[i].side))
                structureObjective.need[i].side = MAX_DIFFERENT_FACTIONS + 1;
            else
                len += 1;
            len += strlen("Enemy") + 1;
        }
        
        if (!strncmp(cur + len, "Any", strlen("Any"))) {
            structureObjective.need[i].info = MAX_DIFFERENT_STRUCTURES;
            len += strlen("Any");
        } else {
            structureObjective.need[i].info = getStructureNameInfo(cur + len);
            if (structureObjective.need[i].info >= 0)
                len += strlen(structureInfo[structureObjective.need[i].info].name);
        }
        if (structureObjective.need[i].info >= 0) {
            if (cur[len] == ',') // a wav was mentioned
                strcpy(structureObjective.need[i].wav, cur + len + 2); // 2 because we're going to skip ", "
        }
        if (cur != oneline)
            structureObjective.opt_amount_need++;
        else
            structureObjective.amount_need++;
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
        cur = (strncmp(oneline, "Opt.", strlen("Opt."))) ? (oneline) : (oneline + strlen("Opt."));
    }
    
    // StructureKill
    for (i=0; i<MAX_OBJECTIVES_COUNT_KILL; i++) {
        if (strncmp(cur, "StructureKill", strlen("StructureKill")))
            break;
        len = strlen("StructureKill") + nrstrlen(i + 1) + 1;
        structureObjective.kill[i].wav[0] = 0;
        
        if (!strncmp(cur + len, "Any,", strlen("Any,"))) {
            structureObjective.kill[i].info = MAX_DIFFERENT_STRUCTURES;
            len += strlen("Any");
        } else {
            structureObjective.kill[i].info = getStructureNameInfo(cur + len);
            if (structureObjective.kill[i].info >= 0)
                len += strlen(structureInfo[structureObjective.kill[i].info].name);
        }
        if (structureObjective.kill[i].info >= 0) {
            if (cur[len] == ',') // a wav was mentioned
                strcpy(structureObjective.kill[i].wav, cur + len + 2); // 2 because we're going to skip ", "
        }
        if (cur != oneline)
            structureObjective.opt_amount_kill++;
        else
            structureObjective.amount_kill++;
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
        cur = (strncmp(oneline, "Opt.", strlen("Opt."))) ? (oneline) : (oneline + strlen("Opt."));
    }
    
    // UnitNeed
    for (i=0; i<MAX_OBJECTIVES_COUNT_NEED; i++) {
        if (strncmp(cur, "UnitNeed", strlen("UnitNeed")))
            break;
        len = strlen("UnitNeed") + nrstrlen(i + 1) + 1;
        
        unitObjective.need[i].side = FRIENDLY;
        unitObjective.need[i].wav[0] = 0;
        
        if (!strncmp(cur + len, "Friendly", strlen("Friendly"))) {
            len += strlen("Friendly") + 1;
        } else if (!strncmp(cur + len, "Enemy", strlen("Enemy"))) {
            if (!sscanf(cur + len, "Enemy%i,", &unitObjective.need[i].side))
                unitObjective.need[i].side = MAX_DIFFERENT_FACTIONS + 1;
            else
                len += 1;
            len += strlen("Enemy") + 1;
        }
        
        if (!strncmp(cur + len, "Any", strlen("Any"))) {
            unitObjective.need[i].info = MAX_DIFFERENT_UNITS;
            len += strlen("Any");
        } else {
            unitObjective.need[i].info = getUnitNameInfo(cur + len);
            if (unitObjective.need[i].info >= 0)
                len += strlen(unitInfo[unitObjective.need[i].info].name);
        }
        if (unitObjective.need[i].info >= 0) {
            if (cur[len] == ',') // a wav was mentioned
                strcpy(unitObjective.need[i].wav, cur + len + 2); // 2 because we're going to skip ", "
        }
        if (cur != oneline)
            unitObjective.opt_amount_need++;
        else
            unitObjective.amount_need++;
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
        cur = (strncmp(oneline, "Opt.", strlen("Opt."))) ? (oneline) : (oneline + strlen("Opt."));
    }
    
    // UnitKill
    for (i=0; i<MAX_OBJECTIVES_COUNT_KILL; i++) {
        if (strncmp(cur, "UnitKill", strlen("UnitKill")))
            break;
        len = strlen("UnitKill") + nrstrlen(i + 1) + 1;
        unitObjective.kill[i].wav[0] = 0;
        
        if (!strncmp(cur + len, "Any,", strlen("Any,"))) {
            unitObjective.kill[i].info = MAX_DIFFERENT_UNITS;
            len += strlen("Any");
        } else {
            unitObjective.kill[i].info = getUnitNameInfo(cur + len);
            if (unitObjective.kill[i].info >= 0)
                len += strlen(unitInfo[unitObjective.kill[i].info].name);
        }
        if (unitObjective.kill[i].info >= 0) {
            if (cur[len] == ',') // a wav was mentioned
                strcpy(unitObjective.kill[i].wav, cur + len + 2); // 2 because we're going to skip ", "
        }
        if (cur != oneline)
            unitObjective.opt_amount_kill++;
        else
            unitObjective.amount_kill++;
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
        cur = (strncmp(oneline, "Opt.", strlen("Opt."))) ? (oneline) : (oneline + strlen("Opt."));
    }
    
    // StructureTo
    for (i=0; i<MAX_OBJECTIVES_COUNT_GETTO; i++) {
        if (strncmp(cur, "StructureTo", strlen("StructureTo")))
            break;
        len = strlen("StructureTo") + nrstrlen(i + 1) + 1;
        structureObjective.get_to[i].wav[0] = 0;
        structureObjective.get_to[i].side = FRIENDLY; // default
        if (!strncmp(cur + len, "Friendly", strlen("Friendly")))
            len += strlen("Friendly") + 1;
        else
        if (!strncmp(cur + len, "Enemy", strlen("Enemy"))) {
            len += strlen("Enemy");
            structureObjective.get_to[i].side = MAX_DIFFERENT_FACTIONS + 1; // default w. Enemy
            if (cur[len] != ',') {
                sscanf(cur + len, "%i,", &structureObjective.get_to[i].side);
                len += nrstrlen(structureObjective.get_to[i].side);
            }
            len += 1;
        }
        if (!strncmp(cur + len, "Any,", strlen("Any,"))) {
            structureObjective.get_to[i].info = MAX_DIFFERENT_STRUCTURES;
            len += strlen("Any");
        } else {
            structureObjective.get_to[i].info = getStructureNameInfo(cur + len);
            len += strlen(structureInfo[structureObjective.get_to[i].info].name);
        }
        if (structureObjective.get_to[i].info >= 0)
            sscanf(cur + len, ",%i,%i,%i,%i, %s", &structureObjective.get_to[i].x1, &structureObjective.get_to[i].y1, &structureObjective.get_to[i].x2, &structureObjective.get_to[i].y2, structureObjective.get_to[i].wav);
        if (cur != oneline)
            structureObjective.opt_amount_get_to++;
        else
            structureObjective.amount_get_to++;
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
        cur = (strncmp(oneline, "Opt.", strlen("Opt."))) ? (oneline) : (oneline + strlen("Opt."));
    }
    
    // UnitTo
    for (i=0; i<MAX_OBJECTIVES_COUNT_GETTO; i++) {
        if (strncmp(cur, "UnitTo", strlen("UnitTo")))
            break;
        len = strlen("UnitTo") + nrstrlen(i + 1) + 1;
        unitObjective.get_to[i].wav[0] = 0;
        unitObjective.get_to[i].side = FRIENDLY; // default
        if (!strncmp(cur + len, "Friendly", strlen("Friendly")))
            len += strlen("Friendly") + 1;
        else
        if (!strncmp(cur + len, "Enemy", strlen("Enemy"))) {
            len += strlen("Enemy");
            unitObjective.get_to[i].side = MAX_DIFFERENT_FACTIONS + 1; // default w. Enemy
            if (cur[len] != ',') {
                sscanf(cur + len, "%i,", &unitObjective.get_to[i].side);
                len += nrstrlen(unitObjective.get_to[i].side);
            }
            len += 1;
        }
        if (!strncmp(cur + len, "Any,", strlen("Any,"))) {
            unitObjective.get_to[i].info = MAX_DIFFERENT_UNITS;
            len += strlen("Any");
        } else {
            unitObjective.get_to[i].info = getUnitNameInfo(cur + len);
            len += strlen(unitInfo[unitObjective.get_to[i].info].name);
        }
        if (unitObjective.get_to[i].info >= 0)
            sscanf(cur + len, ",%i,%i,%i,%i, %s", &unitObjective.get_to[i].x1, &unitObjective.get_to[i].y1, &unitObjective.get_to[i].x2, &unitObjective.get_to[i].y2, unitObjective.get_to[i].wav);
        if (cur != oneline)
            unitObjective.opt_amount_get_to++;
        else
            unitObjective.amount_get_to++;
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
        cur = (strncmp(oneline, "Opt.", strlen("Opt."))) ? (oneline) : (oneline + strlen("Opt."));
    }
    
    // Collectibles
    for (i=0; i<MAX_OBJECTIVES_COUNT_COLLECTIBLES; i++) {
        if (strncmp(cur, "Collectible", strlen("Collectible")))
            break;
        len = strlen("Collectible") + nrstrlen(i + 1) + 1;
        unitObjective.collect[i].wav[0] = 0;
        unitObjective.collect[i].info = 0;  // this works with any unit!
        sscanf(cur + len, "%i,%i,%i, %s", &unitObjective.collect[i].x, &unitObjective.collect[i].y, &unitObjective.collect[i].value, unitObjective.collect[i].wav);
        if (cur != oneline)
            unitObjective.opt_amount_collect++;
        else
            unitObjective.amount_collect++;
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
        cur = (strncmp(oneline, "Opt.", strlen("Opt."))) ? (oneline) : (oneline + strlen("Opt."));
    }
    
    // Resources
    for (i=0; i<MAX_OBJECTIVES_COUNT_RESOURCES; i++) {
        if (strncmp(cur, "Resources", strlen("Resources")))
            break;
        len = strlen("Resources=");
        resourceObjective.need[i].wav[0] = 0;
        sscanf(cur + len, "%i, %s", &resourceObjective.need[i].info, resourceObjective.need[i].wav);
        if (cur != oneline)
            resourceObjective.opt_amount_need++;
        else
            resourceObjective.amount_need++;
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
        cur = (strncmp(oneline, "Opt.", strlen("Opt."))) ? (oneline) : (oneline + strlen("Opt."));
    }
    
    // KillAllObjective
    killAllObjective.win_wav[0]  = 0;
    killAllObjective.lose_wav[0] = 0;
    if (!strncmp(oneline, "KillAll", strlen("KillAll"))) {
        len = strlen("KillAll") + 1;
        for (i=0; i<strlen(oneline + len); i++) {
            if (oneline[len + i] == ',')
                break;
        }
        if (i < strlen(oneline + len)) {
            oneline[len + i] = 0;
            strcpy(killAllObjective.win_wav,  oneline + len);
            strcpy(killAllObjective.lose_wav, oneline + len + i + 1);
        }
    }
    
    objectivesState = OBJECTIVES_INCOMPLETE;
    objectivesStateTimer = 0;
    
    closeFile(fp);
}

inline struct EntityObjectives *getStructureObjectives() {
    return &structureObjective;
}

inline struct EntityObjectives *getUnitObjectives() {
    return &unitObjective;
}

inline struct ResourceObjectives *getResourceObjectives() {
    return &resourceObjective;
}

int getStructureObjectivesSaveSize(void) {
    return sizeof(structureObjective);
}

int getUnitObjectivesSaveSize(void) {
    return sizeof(unitObjective);
}

int getResourceObjectivesSaveSize(void) {
    return sizeof(resourceObjective);
}


int getStructureObjectivesSaveData(void *dest, int max_size) {
    int size=sizeof(structureObjective);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&structureObjective,size);
    return (size);
}

int getUnitObjectivesSaveData(void *dest, int max_size) {
    int size=sizeof(unitObjective);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&unitObjective,size);
    return (size);
}

int getResourceObjectivesSaveData(void *dest, int max_size) {
    int size=sizeof(resourceObjective);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&resourceObjective,size);
    return (size);
}
