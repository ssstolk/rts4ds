// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "ai.h"

#include "game.h"
#include "info.h"
#include "factions.h"
#include "units.h"
#include "structures.h"
#include "music.h"
#include "fileio.h"

#define MAX_DIFFERENT_TEAM_SCRIPTED_AI 50

struct TeamScriptedAI {
    enum UnitGroupAI groupAI;
    enum UnitType unitType;
    int minAmount;
    int maxAmount;
    int staging_tile;
};

struct TeamCurrentAI {
    int currentTeam;
    int amount;
};

struct TeamAI {
    int amountOfTeamScriptedAI;
    struct TeamScriptedAI teamScriptedAI[MAX_DIFFERENT_TEAM_SCRIPTED_AI];
    struct TeamCurrentAI teamCurrentAI;
    // duration between teams and countdown
    int pauseBetweenTeams;
    int currentPause;
    // maximum duration staging (for meeting up at a rally point, before moving on to attack) and countdown
    int maximumStagingDuration;
    int currentStagingDuration;
};


static struct TeamAI teamAI[MAX_DIFFERENT_FACTIONS - 1];
static struct PriorityStructureAI priorityStructureAI;




void doTeamAILogic() {
    int i, j, k;
    int possibleUnitToBuild[ENEMY_UNITS_QUEUE];
    int possibleUnitToBuildBy[ENEMY_UNITS_QUEUE];
    int amountOfPossibleUnitsToBuild;
    enum UnitType unitTypesToBuild;
    int amountOfUnitsOnMap;
    int formingBecomesStaging;
    
    startProfilingFunction("doTeamAILogic");
    
    for (i=0; i<getAmountOfSides() - 1; i++) {
        if (teamAI[i].amountOfTeamScriptedAI == 0) // no scripted teams available
            continue;
        
        if (teamAI[i].teamCurrentAI.currentTeam != -1 && teamAI[i].currentStagingDuration == 0) {
            // send already created STAGING teams off to do their job (they are not allowed to wait any longer!)
            for (j=0; j<MAX_UNITS_ON_MAP; j++) {
                if (unit[j].side == i+1 && unit[j].enabled) {
                    if (unit[j].group & UGAI_STAGING) {
                        unit[j].group &= (~UGAI_STAGING);
                        if (unit[j].group == UGAI_HUNT) {
                            unit[j].logic = UL_HUNT;
                            unit[j].logic_aid = 0;
                        } else if (unit[j].group == UGAI_KAMIKAZE)
                            unit[j].logic = UL_KAMIKAZE;
                    }
                }
            }
            teamAI[i].currentStagingDuration = -1;
        }
        
        if (teamAI[i].teamCurrentAI.currentTeam == -1 || !getUnitsQueue(i+1)[0].enabled) {
            // send out created team (if it exists and isn't already sent out) and count how many units of this side already exist
            amountOfUnitsOnMap = 0;
            formingBecomesStaging = 0;
            for (j=0; j<MAX_UNITS_ON_MAP; j++) {
                if (unit[j].side == i+1 && unit[j].enabled) {
                    amountOfUnitsOnMap++;
                    if (unit[j].group == UGAI_FORMING)
                        formingBecomesStaging = 1;
                }
            }
            if (formingBecomesStaging) {
                teamAI[i].currentStagingDuration = FPS; // grant one second minimum
                for (j=0; j<MAX_UNITS_ON_MAP; j++) {
                    if (unit[j].side == i+1 && unit[j].enabled) {                
                        if (unit[j].group & UGAI_STAGING) {
                            // send already created STAGING teams off to do their job (only 1 STAGING team allowed)
                            unit[j].group &= (~UGAI_STAGING);
                            if (unit[j].group == UGAI_HUNT) {
                                unit[j].logic = UL_HUNT;
                                unit[j].logic_aid = 0;
                            } else if (unit[j].group == UGAI_KAMIKAZE)
                                unit[j].logic = UL_KAMIKAZE;
                        }
                        else if (unit[j].group == UGAI_FORMING) {
                            unit[j].group = UGAI_STAGING | teamAI[i].teamScriptedAI[teamAI[i].teamCurrentAI.currentTeam].groupAI;
                            unit[j].logic = UL_MOVE_LOCATION;
                            unit[j].logic_aid = teamAI[i].teamScriptedAI[teamAI[i].teamCurrentAI.currentTeam].staging_tile;
                            if (unit[j].logic_aid == -1) // let it "move" to its current location, so sending the units off will kick in 
                                unit[j].logic_aid = TILE_FROM_XY(unit[j].x, unit[j].y); 
                            teamAI[i].currentStagingDuration = (teamAI[i].maximumStagingDuration > 0) ?
                                                                teamAI[i].maximumStagingDuration :
                                                                max(teamAI[i].currentStagingDuration, 
                                                                    (abs(X_FROM_TILE(unit[j].logic_aid) - unit[j].x) +
                                                                     abs(Y_FROM_TILE(unit[j].logic_aid) - unit[j].y)) *
                                                                    unitInfo[unit[j].info].speed);
                        }
                    }
                }
            }
            
            if (teamAI[i].currentStagingDuration > 0)
                teamAI[i].currentStagingDuration--;
            
            if (teamAI[i].currentPause > 0) {
                teamAI[i].currentPause--;
                continue;
            }
            
            // set new team to create
            teamAI[i].teamCurrentAI.currentTeam = (teamAI[i].teamCurrentAI.currentTeam + 1) % teamAI[i].amountOfTeamScriptedAI;
            if (teamAI[i].teamScriptedAI[teamAI[i].teamCurrentAI.currentTeam].minAmount <= getUnitLimit(i+1)-amountOfUnitsOnMap) {
                // determine the number of units for the team and create it
                teamAI[i].teamCurrentAI.amount = MIN(
                    getUnitLimit(i+1)-amountOfUnitsOnMap, // max additional units allowed for this side
                    teamAI[i].teamScriptedAI[teamAI[i].teamCurrentAI.currentTeam].minAmount + rand() % (teamAI[i].teamScriptedAI[teamAI[i].teamCurrentAI.currentTeam].maxAmount - teamAI[i].teamScriptedAI[teamAI[i].teamCurrentAI.currentTeam].minAmount)); // ideal additional units for this side
                
                unitTypesToBuild = teamAI[i].teamScriptedAI[teamAI[i].teamCurrentAI.currentTeam].unitType;
                amountOfPossibleUnitsToBuild = 0;
                
                for (j=0; j<MAX_DIFFERENT_UNITS; j++) {
                    if (unitTypesToBuild & BIT(unitInfo[j].type)) {
                        k = availableBuilderForItem(i+1, 0, j);
                        if (k >= 0) {
                            possibleUnitToBuild[amountOfPossibleUnitsToBuild]   = j;
                            possibleUnitToBuildBy[amountOfPossibleUnitsToBuild] = k;
                            amountOfPossibleUnitsToBuild++;
                            if (amountOfPossibleUnitsToBuild == ENEMY_UNITS_QUEUE)
                                break;
                        }
                    }
                }
                if (amountOfPossibleUnitsToBuild > 0) {
                    for (j=0; j<teamAI[i].teamCurrentAI.amount; j++) {
                        k = rand() % amountOfPossibleUnitsToBuild;
                        addItemUnitsQueue(i+1, possibleUnitToBuild[k], possibleUnitToBuildBy[k]);
                    }
                }
                // do this even if no new units could be built. don't want the game to slow down to a crawl.
                teamAI[i].currentPause = teamAI[i].pauseBetweenTeams;
            }
        }
    }
    
    stopProfilingFunction();
}



void initTeamAI() {
    FILE *fp;
    char oneline[256];
    char *positionInInput;
    enum UnitType unitType;
    int i,j,k;
    
    int gameSpeed = getGameSpeed();
    
    for (i=0; i<getAmountOfSides() - 1; i++) {
        teamAI[i].amountOfTeamScriptedAI = 0;
        teamAI[i].teamCurrentAI.currentTeam = -1;
        teamAI[i].pauseBetweenTeams = 0;
        teamAI[i].currentPause = 0;
        teamAI[i].maximumStagingDuration = 0;
        teamAI[i].currentStagingDuration = 0;
    }
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    
    // TEAMS section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[TEAMS]", strlen("[TEAMS]")));
    while (1) {
        readstr(fp, oneline);
        if (strncmp(oneline, "Enemy", strlen("Enemy")))
            break;
        sscanf(oneline, "Enemy%i,", &i);
        positionInInput = oneline + strlen("Enemy#,");
        i--;
        
        if (i >= MAX_DIFFERENT_FACTIONS-1)
            continue; // ignore this line; it mentions an enemy that does not occur in the current scenario
        if (teamAI[i].amountOfTeamScriptedAI == MAX_DIFFERENT_TEAM_SCRIPTED_AI)
            error("Scenario specified more AI Teams for the following faction than allowed:", factionInfo[getFaction(i+1)].name);
        
        if (!strncmp(positionInInput, "InitialPause", strlen("InitialPause"))) {
            sscanf(positionInInput + strlen("InitialPause"), ",%i", &teamAI[i].currentPause);
            teamAI[i].currentPause = (2*(teamAI[i].currentPause * FPS)) / gameSpeed;
            continue;
        }
        if (!strncmp(positionInInput, "Pause", strlen("Pause"))) {
            sscanf(positionInInput + strlen("Pause"), ",%i", &teamAI[i].pauseBetweenTeams);
            teamAI[i].pauseBetweenTeams = (2*(teamAI[i].pauseBetweenTeams * FPS)) / gameSpeed;
            continue;
        }
        
        if (!strncmp(positionInInput, "MaxStagingDuration", strlen("MaxStagingDuration"))) {
            sscanf(positionInInput + strlen("MaxStagingDuration"), ",%i", &teamAI[i].maximumStagingDuration);
            teamAI[i].maximumStagingDuration = (2*(teamAI[i].maximumStagingDuration * FPS)) / gameSpeed;
            continue;
        }
        
        if (!strncmp(positionInInput, "Kamikaze", strlen("Kamikaze"))) {
            teamAI[i].teamScriptedAI[teamAI[i].amountOfTeamScriptedAI].groupAI = UGAI_KAMIKAZE;
            positionInInput += strlen("Kamikaze") + 1;
/*      } else if (!strncmp(positionInInput, "Flee", strlen("Flee"))) {
            teamAI[i].teamScriptedAI[teamAI[i].amountOfTeamScriptedAI].groupAI = UGAI_FLEE;
            positionInInput += strlen("Flee,");
        } else if (!strncmp(positionInInput, "Staging", strlen("Staging"))) {
            teamAI[i].teamScriptedAI[teamAI[i].amountOfTeamScriptedAI].groupAI = UGAI_STAGING;
            positionInInput += strlen("Staging,");*/
        } else if (!strncmp(positionInInput, "Hunt", strlen("Hunt"))) {
            teamAI[i].teamScriptedAI[teamAI[i].amountOfTeamScriptedAI].groupAI = UGAI_HUNT;
            positionInInput += strlen("Hunt") + 1;
        } else if (!strncmp(positionInInput, "Normal", strlen("Normal"))) {
            teamAI[i].teamScriptedAI[teamAI[i].amountOfTeamScriptedAI].groupAI = UGAI_HUNT;
            positionInInput += strlen("Normal") + 1;
        } else
            error("Invalid team AI in scenario", oneline);
        
        unitType = 0;
        for (;;) {
            if (!strncmp(positionInInput, "Foot", strlen("Foot"))) {
                unitType |= BIT(UT_FOOT);
                positionInInput += strlen("Foot,");
            } else if (!strncmp(positionInInput, "Wheeled", strlen("Wheeled"))) {
                unitType |= BIT(UT_WHEELED);
                positionInInput += strlen("Wheeled,");
            } else if (!strncmp(positionInInput, "Tracked", strlen("Tracked"))) {
                unitType |= BIT(UT_TRACKED);
                positionInInput += strlen("Tracked,");
            } else if (!strncmp(positionInInput, "Aerial", strlen("Aerial"))) {
                unitType |= BIT(UT_AERIAL);
                positionInInput += strlen("Aerial,");
            } else
                break;
        }
        
        if (unitType == 0) // if no unitType was specified, all unitTypes are OK to use
            unitType = BIT(UT_FOOT) | BIT(UT_WHEELED) | BIT(UT_TRACKED) | BIT(UT_AERIAL);
        
        teamAI[i].teamScriptedAI[teamAI[i].amountOfTeamScriptedAI].unitType = unitType;
        
        sscanf(positionInInput, "%i,%i", &teamAI[i].teamScriptedAI[teamAI[i].amountOfTeamScriptedAI].minAmount, &teamAI[i].teamScriptedAI[teamAI[i].amountOfTeamScriptedAI].maxAmount);
        
        while (*positionInInput != 0 && *positionInInput != 'R')
            positionInInput++;
        if (sscanf(positionInInput, "Rally:%i,%i", &j, &k) == 2)
            teamAI[i].teamScriptedAI[teamAI[i].amountOfTeamScriptedAI].staging_tile = TILE_FROM_XY(j,k);
        else
            teamAI[i].teamScriptedAI[teamAI[i].amountOfTeamScriptedAI].staging_tile = -1;
        
        teamAI[i].amountOfTeamScriptedAI++;
    }
    
    closeFile(fp);
}




struct RebuildItem {
    int enabled;
    int nr;
};

struct RebuildQueue {
    struct RebuildItem structure[ENEMY_STRUCTURES_QUEUE];
    struct RebuildItem unit[ENEMY_UNITS_QUEUE];
    int structure_delay;
    int unit_delay;
};

struct RebuildQueue rebuildQueue[MAX_DIFFERENT_FACTIONS];



void doRebuildAILogic() {
    int freeQueueSlots[MAX_DIFFERENT_FACTIONS];
    int foundationQueueSlot[MAX_DIFFERENT_FACTIONS];
    enum Side curSide;
    int okToPlaceStructure;
    struct EnvironmentLayout *envLayout;
    int foundationInfo = 0;
    int structureNr;
    int i, j, k;
    
    startProfilingFunction("doRebuildAILogic");
    
    // STRUCTURES TIME
    startProfilingFunction("doRebuildAILogic (structures)");
    
    // get foundation info
    for (i=0; i<MAX_DIFFERENT_STRUCTURES && structureInfo[i].enabled; i++) {
        if (structureInfo[i].foundation && structureInfo[i].width == 1 && structureInfo[i].height == 1) {
            foundationInfo = i;
            break;
        }
    }
    
    // get foundation queue slots
    for (curSide=1; curSide<getAmountOfSides(); curSide++) {
        foundationQueueSlot[curSide] = -1;
        for (i=0; i<ENEMY_STRUCTURES_QUEUE && getStructuresQueue(curSide)[i].enabled; i++) {
            if (structureInfo[getStructuresQueue(curSide)[i].itemInfo].foundation) {
                foundationQueueSlot[curSide] = i;
                break;
            }
        }
    }
    
    // place foundation and/or structures
    for (curSide=1; curSide<getAmountOfSides(); curSide++) {
        for (i=0; i<ENEMY_STRUCTURES_QUEUE; i++) {
            if (rebuildQueue[curSide].structure[i].enabled) {
                structureNr = rebuildQueue[curSide].structure[i].nr;
                if (foundationQueueSlot[curSide] >= 0 && getStructuresQueue(curSide)[foundationQueueSlot[curSide]].status == DONE) {
                    // lay down foundation tile if possible
                    okToPlaceStructure = 1;
                    for (j=0; j<structureInfo[structure[structureNr].info].height; j++) {
                        for (k=0; k<structureInfo[structure[structureNr].info].width; k++) {
                            envLayout = environment.layout + TILE_FROM_XY(structure[structureNr].x + k, structure[structureNr].y + j);
                            if (envLayout->contains_structure == curSide) { // found foundation tile here, but is a unit obscuring it for placement?
                                if (envLayout->contains_unit != -1)
                                    okToPlaceStructure = 0;
                            } else { // perhaps we can place the foundation tile here then
                                if (rebuildQueue[curSide].structure_delay == 0 && foundationQueueSlot[curSide] >= 0 && addStructure(curSide, structure[structureNr].x + k, structure[structureNr].y + j, getStructuresQueue(curSide)[foundationQueueSlot[curSide]].itemInfo, 0) >= 0) {
                                    removeItemStructuresQueueNr(curSide, foundationQueueSlot[curSide], 0, 0);
                                    foundationQueueSlot[curSide] = -1;
                                    rebuildQueue[curSide].structure_delay = REBUILD_DELAY_CONCRETE;
                                }// else    // should wait with placing this structure anyway, even if it now can be placed
                                    okToPlaceStructure = 0;
                            }
                        }
                    }
                    // place structure if possible
                    if (okToPlaceStructure) {
                        if (rebuildQueue[curSide].structure_delay == 0 && getStructuresQueue(curSide)[i].status == DONE) {
                            j = addStructure(curSide, structure[structureNr].x, structure[structureNr].y, structure[structureNr].info, 0);
                            if (j >= 0) {
                                structure[structureNr].group = 0; // taken care of
                                removeItemStructuresQueueNr(curSide, i, 0, 0);
                                rebuildQueue[curSide].structure_delay = getRebuildDelayStructures(curSide);
                                for (k=MAX_DIFFERENT_FACTIONS; k<MAX_STRUCTURES_ON_MAP; k++) {
                                    if (structure[k].side == curSide)
                                        structure[k].group &= (~TECHTREE_INSUFFICIENT);
                                }
                                for (k=0; k<MAX_UNITS_ON_MAP; k++) {
                                    if (unit[k].side == curSide)
                                        unit[k].group &= (~TECHTREE_INSUFFICIENT);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // build foundation tile if required
    for (i=1; i<getAmountOfSides(); i++) {
        if (foundationQueueSlot[i] == -1) {
            k = availableBuilderForItem(i /*side*/, 1 /*isStructure*/, foundationInfo /*itemInfo*/);
            if (k >= 0) {
                addItemStructuresQueue(i, foundationInfo, k);
                foundationQueueSlot[i] = 0; // set it to 0 temporarily, to be able to determine if one is being built 
            }
        }
    }
    
    // get amount of free queue slots per side
    for (i=1; i<getAmountOfSides(); i++) {
        for (j=0; j<ENEMY_STRUCTURES_QUEUE && getStructuresQueue(i)[j].enabled; j++);
        freeQueueSlots[i] = (ENEMY_STRUCTURES_QUEUE - (foundationQueueSlot[i] == -1)) - j;
    }
    // place structures in queue when possible
    for (i=MAX_DIFFERENT_FACTIONS; i<MAX_STRUCTURES_ON_MAP; i++) {
// TO CHECK: DO I EVER SET ALL DISABLED STRUCTURES' GROUPS TO 0 AT SCENARIO INIT TO PREVENT GETTING INTO THIS TOO MUCH?
// TODO:     ADD if !enabled ???
        if ((structure[i].group & AWAITING_REPLACEMENT) && !(structure[i].group & (TECHTREE_INSUFFICIENT | QUEUED_FOR_REPLACEMENT))) {
            if (freeQueueSlots[structure[i].side]) {
                for (j=0; j<structureInfo[structure[i].info].height; j++) {
                    for (k=0; k<structureInfo[structure[i].info].width; k++) {
                        if (!isEnvironmentTileBetween(TILE_FROM_XY(structure[i].x + k, structure[i].y + j), ROCK, ROCKCUSTOM32))
                            structure[i].group = 0; // cannot ever replace it at its current location, thus is dropped
                    }    
                }
                
                if (structure[i].group != 0) { // has not been dropped yet for replacement by code above, rebuild
                    j = availableBuilderForItem(structure[i].side, 1, structure[i].info);
                    if (j >= 0) {
                        if (addItemStructuresQueue(structure[i].side, structure[i].info, j)) {
                            structure[i].group |= QUEUED_FOR_REPLACEMENT;
                            rebuildQueue[structure[i].side].structure[ENEMY_STRUCTURES_QUEUE - freeQueueSlots[structure[i].side]].enabled = 1;
                            rebuildQueue[structure[i].side].structure[ENEMY_STRUCTURES_QUEUE - freeQueueSlots[structure[i].side]].nr = i;
                            freeQueueSlots[structure[i].side]--;
                        }
                    } else
                        structure[i].group |= TECHTREE_INSUFFICIENT;
                }
            }
        }
    }
    
    stopProfilingFunction(); // of structures
    
    // UNITS TIME
    startProfilingFunction("doRebuildAILogic (units)");
    
    for (i=1; i<getAmountOfSides(); i++) {
        for (j=0; j<ENEMY_UNITS_QUEUE && getUnitsQueue(i)[j].enabled; j++);
        freeQueueSlots[i] = ENEMY_UNITS_QUEUE - j;
    }
    for (i=0; i<MAX_UNITS_ON_MAP; i++) {
        if ((unit[i].group & AWAITING_REPLACEMENT) && !(unit[i].group & (TECHTREE_INSUFFICIENT | QUEUED_FOR_REPLACEMENT))) {
            curSide = unit[i].side;
            if (rebuildQueue[curSide].unit_delay == 0 && freeQueueSlots[curSide]) {
                j = availableBuilderForItem(curSide, 0, unit[i].info);
                if (j >= 0) {
                    if (addItemUnitsQueue(curSide, unit[i].info, j)) {
                        freeQueueSlots[curSide]--;
                        unit[i].group |= QUEUED_FOR_REPLACEMENT;
                        
                        for (k=0; k<ENEMY_UNITS_QUEUE - freeQueueSlots[curSide]; k++) {
                            if (!rebuildQueue[curSide].unit[k].enabled && getUnitsQueue(curSide)[k].itemInfo == unit[i].info) {
                                rebuildQueue[curSide].unit[k].enabled = 1;
                                rebuildQueue[curSide].unit[k].nr = i;
                            }
                        }
                        
                        rebuildQueue[curSide].unit_delay = getRebuildDelayUnits(curSide);
                    }
                } else
                    unit[i].group |= TECHTREE_INSUFFICIENT;
            }
        }
    }
    
    for (i=1; i<getAmountOfSides(); i++) {
        if (rebuildQueue[i].structure_delay > 0)
            rebuildQueue[i].structure_delay--;
        
        if (rebuildQueue[i].unit_delay > 0)
            rebuildQueue[i].unit_delay--;
    }
    
    stopProfilingFunction(); // of units
    
    stopProfilingFunction(); // of entire function
}


void adjustAIUnitAdded(enum Side side, int itemNr, int unitNr) {
    struct Unit *srcUnit;
    struct Unit *dstUnit = unit + unitNr;
    
    if (rebuildQueue[side].unit[itemNr].enabled) {
        srcUnit = unit + rebuildQueue[side].unit[itemNr].nr;
        
        dstUnit->group = srcUnit->group & ~(AWAITING_REPLACEMENT | TECHTREE_INSUFFICIENT | QUEUED_FOR_REPLACEMENT);
        dstUnit->guard_tile = srcUnit->guard_tile;
        dstUnit->retreat_tile = srcUnit->retreat_tile;
        dstUnit->logic = UL_GUARD_RETREAT;
        srcUnit->group = 0; // taken care of
        
        return; // no need to set enabled to 0, because that'll be done right after this anyway by removeAIRebuildUnitNr();
    }
    
    dstUnit->group = UGAI_FORMING;
}


void removeAIRebuildUnitNr(enum Side side, int nr) {
    struct RebuildItem *ri = &rebuildQueue[side].unit[nr];
    int i;
        
    if (ri->enabled)
        unit[ri->nr].group &= (~QUEUED_FOR_REPLACEMENT);
    
    for (i=nr; i<ENEMY_UNITS_QUEUE-1; i++)
        memcpy(&rebuildQueue[side].unit[i], &rebuildQueue[side].unit[i+1], sizeof(struct RebuildItem));
    rebuildQueue[side].unit[ENEMY_UNITS_QUEUE-1].enabled = 0;
}


void removeAIRebuildStructureNr(enum Side side, int nr) {
    int i;
    int structureNr = rebuildQueue[side].structure[nr].nr;
    
    for (i=nr; i<ENEMY_STRUCTURES_QUEUE-1; i++)
        memcpy(&rebuildQueue[side].structure[i], &rebuildQueue[side].structure[i+1], sizeof(struct RebuildItem));
    rebuildQueue[side].structure[ENEMY_STRUCTURES_QUEUE-1].enabled = 0;
    
    structure[structureNr].group &= (~QUEUED_FOR_REPLACEMENT);
}


void initRebuildAI() {
    int i, j;
    
    for (i=1; i<getAmountOfSides(); i++) {
        for (j=0; j<ENEMY_STRUCTURES_QUEUE; j++)
            rebuildQueue[i].structure[j].enabled = 0;
        for (j=0; j<ENEMY_UNITS_QUEUE; j++)
            rebuildQueue[i].unit[j].enabled = 0;
        rebuildQueue[i].structure_delay = getRebuildDelayStructures(i);
        rebuildQueue[i].unit_delay = getRebuildDelayUnits(i);
    }
}




void doAILogic() {
    startProfilingFunction("doAILogic");
    
    doTeamAILogic();
    doRebuildAILogic();
    
    stopProfilingFunction();
}


void initAI() {
    initTeamAI();
    initRebuildAI();
}




struct PriorityStructureAI *getPriorityStructureAI() {
    return &priorityStructureAI;
}



void initPriorityStructureAI() {
    char oneline[256];
    FILE *fp;
    int i, j;

    fp = openFile("priority-structures", FS_AI_FILE);
    readstr(fp, oneline);
    sscanf(oneline, "AMOUNT-OF-PRIORITY-STRUCTURES %i", &priorityStructureAI.amountOfItems);

    for (i=0; i<priorityStructureAI.amountOfItems; i++) {
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 255);
        for (j=0; j<MAX_DIFFERENT_STRUCTURES && structureInfo[j].enabled; j++) {
            if (!strncmp(oneline, structureInfo[j].name, strlen(structureInfo[j].name)) && oneline[strlen(structureInfo[j].name)] == 0) {
                priorityStructureAI.item[i] = j;
                break;
            }
        }
        if (j == MAX_DIFFERENT_STRUCTURES)
            error("In the AI an unknown structure was mentioned", oneline);
    }
    closeFile(fp);
}


inline struct TeamAI *getTeamAI() {
    return teamAI;
}

int getTeamAISaveSize(void) {
    return sizeof(teamAI);
}

int getTeamAISaveData(void *dest, int max_size) {
    int size=sizeof(teamAI);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&teamAI,size);
    return (size);
}

struct RebuildQueue *getRebuildQueue() {
    return rebuildQueue;
}

int getRebuildQueueSaveSize(void) {
    return sizeof(rebuildQueue);
}

int getRebuildQueueSaveData(void *dest, int max_size) {
    int size=sizeof(rebuildQueue);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&rebuildQueue,size);
    return (size);
}
