// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "info.h"

#include "game.h"
#include "factions.h"
#include "structures.h"
#include "ai.h"
#include "settings.h"
#include "soundeffects.h"
#include "fileio.h"


struct Stats {
    int ore_holding; /* currently stored a.k.a. credits */
    int ore_showing; /* showing to the player, as to prevent instantaneous money changes */
    int ore_storage; /* total storing capacity */
    int power_consuming;
    int power_generating;
    int unitCount;
    int unitDeaths;
    int structureCount;
    int structureDeaths;
    struct BuildQueueItem structuresQueue[MAX_STRUCTURES_QUEUE];
    struct BuildQueueItem unitsQueue[MAX_UNITS_QUEUE];
};


struct Stats stats[MAX_DIFFERENT_FACTIONS]; // each side one
struct Techtree techtree[MAX_DIFFERENT_FACTIONS]; // each side one
int matchTime;
int unitLimit[MAX_DIFFERENT_FACTIONS]; // each side one

int amountOfSides;


int getAmountOfSides() {
    return amountOfSides;
}

void setAmountOfSides(int amount) {
    amountOfSides = amount;
}


int getMatchTime() {
    return (matchTime / FPS);
}

void setMatchTime(int time) {
    matchTime=time*FPS;
}

void updateMatchTime() {
    if (matchTime < (9 * 60 * 60 + 59 * 60 + 59) * FPS)
        matchTime++;
}

void initMatchTime() {
    matchTime = 0;
}


int getUnitCount(enum Side side) {
    return stats[side].unitCount;
}

void setUnitCount(enum Side side, int amount) {
    stats[side].unitCount = amount;
}


int getUnitLimit(enum Side side) {
    return unitLimit[side];
}

void setUnitLimit(enum Side side, int amount) {
    unitLimit[side] = amount;
}


int getUnitDeaths(enum Side side) {
    return stats[side].unitDeaths;
}

void setUnitDeaths(enum Side side, int amount) {
    if (amount > 9999) amount = 9999;
    stats[side].unitDeaths = amount;
}


int getStructureCount(enum Side side) {
    return stats[side].structureCount;
}

void setStructureCount(enum Side side, int amount) {
    if (amount > 9999) amount = 9999;
    stats[side].structureCount = amount;
}


int getStructureDeaths(enum Side side) {
    return stats[side].structureDeaths;
}

void setStructureDeaths(enum Side side, int amount) {
    stats[side].structureDeaths = amount;
}


inline struct Techtree *getTechtree(enum Side side) {
    return &techtree[side];
}

int addItemTechtree(enum Side side, struct TechtreeItem *item) {
    if (techtree[side].amountOfItems == MAX_DIFFERENT_STRUCTURES + MAX_DIFFERENT_UNITS)
        return 0;
    techtree[side].item[techtree[side].amountOfItems] = *item;
    techtree[side].amountOfItems++;
    return 1;
}


// note that I assume no 'None' is used in the techtree, otherwise I'd have to insert multiple checks
// I also assume ti->buildStructure is set to true/1 or false/0 already
int readTechtreeItem(struct TechtreeItem *ti, char *oneline) {
    int i;
    int offset=0;
    
    ti->buildingInfo = -1;
    ti->buildableInfo = -1;
    
    for (i=0; i<MAX_REQUIREDINFO_COUNT; i++) 
        ti->requiredInfo[i] = -1;
    
    // structure that builds the item
    ti->buildingInfo = getStructureNameInfo(oneline);
    offset += strlen(structureInfo[ti->buildingInfo].name);
    offset += 1; // +1 for the comma that follows it
    
    // item that it builds
    if (ti->buildStructure) { // it should have a structure as buildable
        ti->buildableInfo = getStructureNameInfo(oneline + offset);
        offset += strlen(structureInfo[ti->buildableInfo].name);
    } else {
        ti->buildableInfo = getUnitNameInfo(oneline + offset);
        offset += strlen(unitInfo[ti->buildableInfo].name);
    }
    if (oneline[offset] != ',') return 1;
    offset += 1; // +1 for the comma that follows it
    
    // required additional structures in order to build the item
    for (i=0; i<MAX_REQUIREDINFO_COUNT; i++) {
        ti->requiredInfo[i] = getStructureNameInfo(oneline + offset);
        offset += strlen(structureInfo[ti->requiredInfo[i]].name);
        if (oneline[offset] != ',') return 1;
        offset += 1; // +1 for the comma that follows it
    }
    
    // this should never occur: one of the above returns should take place
    error("Exceeded the maximum in a techtree of the amount of additional required structures to build:", oneline);
    return 0;
}


void initTechtree(enum Side side) {
    FILE *fp;
    char oneline[256];
    struct TechtreeItem ti;
    int i;
    
    techtree[side].amountOfItems = 0;
    
    // going to read structures techtree
    fp = openFile(factionInfo[getFaction(side)].name, FS_STRUCTURES_TECHTREE_FILE);
    ti.buildStructure = 1;
    
    do {
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[LEVEL", strlen("[LEVEL")));
        sscanf(oneline, "[LEVEL%i]", &i);
    } while (i < getFactionTechLevel(side));
    if (i != getFactionTechLevel(side))
        error("Structure techtree level doesn't exist for", factionInfo[getFaction(side)].name);
    
    readstr(fp, oneline);
    replaceEOLwithEOF(oneline, 256);
    while (oneline[0] != '[') {
        if (readTechtreeItem(&ti, oneline)) { // proper techtree item read in (for current level)
            if (!addItemTechtree(side, &ti))
                error("Too many techtree rules for faction:", factionInfo[getFaction(side)].name);
        }
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
    }
    closeFile(fp);
    
    // going to read units techtree
    fp = openFile(factionInfo[getFaction(side)].name, FS_UNITS_TECHTREE_FILE);
    ti.buildStructure = 0;
    do {
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[LEVEL", strlen("[LEVEL")));
        sscanf(oneline, "[LEVEL%i]", &i);
    } while (i < getFactionTechLevel(side));
    if (i != getFactionTechLevel(side))
        error("Unit techtree level doesn't exist for", factionInfo[getFaction(side)].name);
    
    readstr(fp, oneline);
    replaceEOLwithEOF(oneline, 256);
    while (oneline[0] != '[') {
        if (readTechtreeItem(&ti, oneline)) { // proper techtree item read in (for current level)
            if (!addItemTechtree(side, &ti))
                error("Too many techtree rules for faction:", factionInfo[getFaction(side)].name);
        }
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
    }
    closeFile(fp);
}


int addItemUnitsQueue(enum Side side, int itemInfo, int builderInfo) { // returns 0 on failure
    struct BuildQueueItem *queue = stats[(int) side].unitsQueue;
    int encounteredBuildingSame = -1; // can only build one unit of the same structure at a time
    int i;
    
    int maxItems = (side == FRIENDLY) ? FRIENDLY_UNITS_QUEUE : ENEMY_UNITS_QUEUE;
    
    for (i=0; i<maxItems; i++) {
        if (queue[i].enabled && queue[i].builderInfo == builderInfo && queue[i].status == BUILDING)
            encounteredBuildingSame = i;
        if (!queue[i].enabled) {
            queue[i].itemInfo = itemInfo;
            queue[i].builderInfo = builderInfo;
            if (encounteredBuildingSame != -1)
                queue[i].status = QUEUED;
            else
                queue[i].status = BUILDING;
            queue[i].completion = 0;
            queue[i].enabled = 1;
            return 1;
        }
    }
    return 0;
}

int removeItemUnitsQueueWithInfo(enum Side side, int itemInfo, int withRefund, int frozen) {
    struct BuildQueueItem *queue = stats[(int) side].unitsQueue;
    int i;
    int bestCandidate = -1;
    
    int maxItems = (side == FRIENDLY) ? FRIENDLY_UNITS_QUEUE : ENEMY_UNITS_QUEUE;
    
    // first try to remove a queued one, otherwise a paused one, otherwise a building one, otherwise one done
    for (i=0; i<maxItems; i++) {
        if (queue[i].enabled && queue[i].itemInfo == itemInfo && (!frozen || queue[i].status == FROZEN)) {
            if (bestCandidate == -1)
                bestCandidate = i;
            else if (queue[i].status < queue[bestCandidate].status)
                bestCandidate = i;
            else if ((queue[i].status == queue[bestCandidate].status) && (queue[i].completion < queue[bestCandidate].completion))
                bestCandidate = i;
        }
    }
    if (bestCandidate != -1) // remove bestCandidate
        return removeItemUnitsQueueNr(side, bestCandidate, withRefund, frozen);
    return 0;
}


int removeItemUnitsQueueNr(enum Side side, int nr, int withRefund, int frozen) {
    struct BuildQueueItem *queue = stats[(int) side].unitsQueue;
    int oldBuilderInfo;
    int i;
    
    if (!queue[nr].enabled || (queue[nr].status == FROZEN && !frozen))
        return 0;
    
    int maxItems = (side == FRIENDLY) ? FRIENDLY_UNITS_QUEUE : ENEMY_UNITS_QUEUE;
    
    oldBuilderInfo = -1;
    if (queue[nr].status == BUILDING || queue[nr].status == DONE)
        oldBuilderInfo = queue[nr].builderInfo;
    if (withRefund)
        changeCredits(side, (queue[nr].completion * unitInfo[queue[nr].itemInfo].build_cost) / unitInfo[queue[nr].itemInfo].build_time);
    for (i=nr+1; i<maxItems; i++) {
        if (queue[i].enabled && queue[i].builderInfo == oldBuilderInfo && queue[i].status == QUEUED) {
            queue[i].status = BUILDING;
            oldBuilderInfo = -1;
        }
        queue[i-1].enabled     = queue[i].enabled;
        queue[i-1].itemInfo    = queue[i].itemInfo;
        queue[i-1].builderInfo = queue[i].builderInfo;
        queue[i-1].status      = queue[i].status;
        queue[i-1].completion  = queue[i].completion;
    }
    queue[maxItems-1].enabled = 0;
    
    if (side != FRIENDLY)
        removeAIRebuildUnitNr(side, nr);
    
    return 1;
}


int togglePauseItemUnitsQueueNr(enum Side side, int nr) {
    struct BuildQueueItem *queue = stats[(int) side].unitsQueue;
    int encounteredBuildingSame = -1; // can only build one unit of the same type at a time
    int encounteredQueuedSame = -1;
    int i;
    
    if (!queue[nr].enabled || queue[nr].status == FROZEN)
        return 0;
    
    if (queue[nr].status == QUEUED) {
        queue[nr].status = PAUSED;
        return 1;
    }
    
    int maxItems = (side == FRIENDLY) ? FRIENDLY_UNITS_QUEUE : ENEMY_UNITS_QUEUE;
    
    for (i=0; i<maxItems; i++) {
        if (queue[i].enabled && i != nr && queue[i].builderInfo == queue[nr].builderInfo) {
            if (encounteredBuildingSame == -1 && queue[i].status == BUILDING)
                encounteredBuildingSame = i;
            else if (encounteredQueuedSame == -1 && queue[i].status == QUEUED)
                encounteredQueuedSame = i;
        }
    }
    if (queue[nr].status == BUILDING) {
        queue[nr].status = PAUSED;
        if (encounteredQueuedSame != -1)
            queue[encounteredQueuedSame].status = BUILDING;
    } else if (queue[nr].status == PAUSED) {
        if (encounteredBuildingSame == -1)
            queue[nr].status = BUILDING;
        else if (encounteredBuildingSame < nr)
            queue[nr].status = QUEUED;
        else {
            queue[encounteredBuildingSame].status = QUEUED;
            queue[nr].status = BUILDING;
        }
    }
    return 1;
}

void freezeItemUnitsQueueNr(enum Side side, int nr) {
    struct BuildQueueItem *queue = stats[(int) side].unitsQueue;
    if (queue[nr].status == DONE)
        queue[nr].status = FROZEN;
}

void unfreezeItemUnitsQueueWithInfo(enum Side side, int itemInfo) {
    struct BuildQueueItem *queue = stats[(int) side].unitsQueue;
    int i;
    
    int maxItems = (side == FRIENDLY) ? FRIENDLY_UNITS_QUEUE : ENEMY_UNITS_QUEUE;
    
    for (i=0; i<maxItems; i++) {
        if (queue[i].status == FROZEN && queue[i].itemInfo == itemInfo) {
            queue[i].status = DONE;
            return;
        }
    }
}

// verify if there are at least TWO builders of this type
// on the requested side (we're building units, not structures)
int verifyUnitCoBuilderExists(int builderInfo, enum Side side) {
  int j,count=0;
  for (j=0; j<MAX_STRUCTURES_ON_MAP; j++) {
    if (structure[j].enabled && structure[j].side == side && structure[j].info == builderInfo) {
	  count++;
	  if (count==2)
	    return 1;
	}
  }
  return 0;  
}

void updateUnitsQueue(enum Side side) {
    struct BuildQueueItem *queue = stats[(int) side].unitsQueue;
    int creditsRequired;
    int i, j;
    int unitNr;
    unsigned int speed;
    int steps;
    
	static unsigned int gameFrame[MAX_DIFFERENT_FACTIONS];
	gameFrame[side]++;
    
    int maxItems = (side == FRIENDLY) ? FRIENDLY_UNITS_QUEUE : ENEMY_UNITS_QUEUE;
    
    for (i=0; i<maxItems && queue[i].enabled; i++) {
        for (j=0; j<MAX_STRUCTURES_ON_MAP; j++) {
            if (structure[j].enabled && structure[j].side == side && structure[j].info == queue[i].builderInfo && structure[j].primary)
                break;
        }
        if (j == MAX_STRUCTURES_ON_MAP) { // the structure that could build this item was destroyed and has to be removed
            removeItemUnitsQueueNr(side, i, 0, 0);
            i--;
        }
    }
    
    for (i=0; i<maxItems; i++) {
        if (queue[i].enabled) {
            if (queue[i].status == BUILDING) {
                steps = 1;
                
                speed = getUnitBuildSpeed(side); // speed measured in percentage, 100% being normal speed
                
                // increase speed by 50% if two or more building structures exist for this unit
                if (verifyUnitCoBuilderExists(queue[i].builderInfo, side))
                    speed += (speed/2);
                if (speed < 100) {
                    j = (100*1000000) / (100-speed); // j == decrease steps once every X/1000000 frames
                    if ((gameFrame > gameFrame-1) && ((gameFrame[side]*1000000)/j > ((gameFrame[side]-1)*1000000)/j))
                        steps--;
                } else if (speed > 100) {
                    for (; speed>100; speed-=100)
                        steps++;
                    j = (100*1000000) / speed; // j == increase steps once every X/1000000 frames
                                                // with +20%, j == 5(000000)
                                                // with +80%, j == 1(250000)
                    if ((gameFrame > gameFrame-1) && ((gameFrame[side]*1000000)/j > ((gameFrame[side]-1)*1000000)/j))
                        steps++;
                }
                
                for (j=0; j<steps && queue[i].completion<unitInfo[queue[i].itemInfo].build_time; j++) {
                    creditsRequired = ((unitInfo[queue[i].itemInfo].build_cost * (queue[i].completion + 1)) / unitInfo[queue[i].itemInfo].build_time) -
                                      ((unitInfo[queue[i].itemInfo].build_cost * (queue[i].completion)) / unitInfo[queue[i].itemInfo].build_time);
                    if (changeCredits(side, -creditsRequired))
                        queue[i].completion++;
                    else if (j==0)
                        break;
                }
            }
            if (queue[i].status == BUILDING || queue[i].status == DONE) {
                if (queue[i].completion >= unitInfo[queue[i].itemInfo].build_time) {
                    /* queued unit is done */
                    /* add it as a new unit on the playfield */
                    unitNr = -1;
                    for (j=0; j<MAX_STRUCTURES_ON_MAP; j++) {
                        if (structure[j].enabled && structure[j].side == side && structure[j].info == queue[i].builderInfo && structure[j].primary) {
                            if (getGameType() == SINGLEPLAYER) { // immediately place, be nice
                                queue[i].status = DONE;
                                unitNr = addUnit(side, structure[j].x + structureInfo[structure[j].info].release_tile, structure[j].y + structureInfo[structure[j].info].height - 1, queue[i].itemInfo, 0);
                                if (unitNr >= 0) {
                                    if (side == FRIENDLY) {
                                        if (unitInfo[queue[i].itemInfo].can_collect_ore)
                                            playSoundeffect(SE_MINER_DEPLOYED);
                                        else
                                            playSoundeffect(SE_UNIT_DEPLOYED);
                                    }
                                    else /*if (side != FRIENDLY)*/ {
                                        adjustAIUnitAdded(side, i, unitNr);
                                    }
                                    removeItemUnitsQueueNr(side, i, 0, 0);
                                    i--;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}

void initUnitsQueue(enum Side side) {
    struct BuildQueueItem *queue = stats[(int) side].unitsQueue;
    int i;
    
    int maxItems = (side == FRIENDLY) ? FRIENDLY_UNITS_QUEUE : ENEMY_UNITS_QUEUE;
    
    for (i=0; i<maxItems; i++)
        queue[i].enabled = 0;
}

inline struct BuildQueueItem *getUnitsQueue(enum Side side) {
    return stats[(int) side].unitsQueue;
}



int addItemStructuresQueue(enum Side side, int itemInfo, int builderInfo) { // returns 0 on failure
    struct BuildQueueItem *queue = stats[(int) side].structuresQueue;
    int build_class_mask=0;
    int first_free_slot=-1;
    int i;
    
    int maxItems = (side == FRIENDLY) ? FRIENDLY_STRUCTURES_QUEUE : ENEMY_STRUCTURES_QUEUE;
    
    for (i=0; i<maxItems; i++) {
        if (queue[i].enabled && queue[i].status == BUILDING)
            build_class_mask |= structureInfo[queue[i].itemInfo].class_mask;
        else if (!queue[i].enabled && (first_free_slot<0))
            first_free_slot = i;
    }
    
    if (first_free_slot>=0) {
        queue[first_free_slot].itemInfo = itemInfo;
        queue[first_free_slot].builderInfo = builderInfo;
        queue[first_free_slot].status = ((build_class_mask & structureInfo[itemInfo].class_mask)==0) ?
                                         BUILDING :
                                         QUEUED;
        queue[first_free_slot].completion = 0;
        queue[first_free_slot].enabled = 1;
        return 1;
    }
    
    return 0;
}

int removeItemStructuresQueueWithInfo(enum Side side, int itemInfo, int withRefund, int frozen) {
    struct BuildQueueItem *queue = stats[(int) side].structuresQueue;
    int i;
    int bestCandidate = -1;
    
    int maxItems = (side == FRIENDLY) ? FRIENDLY_STRUCTURES_QUEUE : ENEMY_STRUCTURES_QUEUE;
    
    // first try to remove a queued one, otherwise a paused one, otherwise a building one, otherwise one done
    for (i=0; i<maxItems; i++) {
        if (queue[i].enabled && queue[i].itemInfo == itemInfo && (!frozen || queue[i].status == FROZEN)) {
            if (bestCandidate == -1)
                bestCandidate = i;
            else if (queue[i].status < queue[bestCandidate].status)
                bestCandidate = i;
            else if ((queue[i].status == queue[bestCandidate].status) && (queue[i].completion < queue[bestCandidate].completion))
                bestCandidate = i;
        }
    }
    if (bestCandidate != -1) // remove bestCandidate
        return removeItemStructuresQueueNr(side, bestCandidate, withRefund, frozen);
    return 0;
}


int removeItemStructuresQueueNr(enum Side side, int nr, int withRefund, int frozen) {
    struct BuildQueueItem *queue = stats[(int) side].structuresQueue;
    enum QueueStatus oldStatus;
    int oldClassMask;
    int i;
    
    if (!queue[nr].enabled || (queue[nr].status == FROZEN && !frozen))
        return 0;
    
    int maxItems = (side == FRIENDLY) ? FRIENDLY_STRUCTURES_QUEUE : ENEMY_STRUCTURES_QUEUE;
    
    if (withRefund)
        changeCredits(side, (queue[nr].completion * structureInfo[queue[nr].itemInfo].build_cost) / structureInfo[queue[nr].itemInfo].build_time);
    oldStatus = queue[nr].status;
    oldClassMask = structureInfo[queue[nr].itemInfo].class_mask;
    for (i=nr+1; i<maxItems; i++) {
        if (oldStatus == BUILDING && queue[i].enabled && queue[i].status == QUEUED &&
            structureInfo[queue[i].itemInfo].class_mask == oldClassMask) {
            queue[i].status = BUILDING;
            oldStatus = PAUSED;
        }
        queue[i-1].enabled = queue[i].enabled;
        queue[i-1].itemInfo = queue[i].itemInfo;
        queue[i-1].status = queue[i].status;
        queue[i-1].completion = queue[i].completion;
    }
    queue[maxItems-1].enabled = 0;
    
    if (side != FRIENDLY)
        removeAIRebuildStructureNr(side, nr);
    
    return 1;
}

int togglePauseItemStructuresQueueNr(enum Side side, int nr) {
    struct BuildQueueItem *queue = stats[(int) side].structuresQueue;
    int i;
    int build_class_mask=0;
    
    if (!queue[nr].enabled || queue[nr].status == FROZEN)
        return 0;
    
    int maxItems = (side == FRIENDLY) ? FRIENDLY_STRUCTURES_QUEUE : ENEMY_STRUCTURES_QUEUE;
    
    if (queue[nr].status == QUEUED)
        queue[nr].status = PAUSED;
    else if (queue[nr].status == BUILDING) {
        queue[nr].status = PAUSED;
        
        // pausing this: check if another of the same class can be started
        for (i=0; i<maxItems; i++) {
            if (queue[i].enabled && queue[i].status == QUEUED && (structureInfo[queue[i].itemInfo].class_mask==structureInfo[queue[nr].itemInfo].class_mask)) {
                queue[i].status = BUILDING;
                break;  // only one other can be started
            }
        }
        
    } else if (queue[nr].status == PAUSED) {
      
        // unpausing this: check if it can be started immediately
        // it can be started if it's the LEFTmost of this class.
        // could cause 're-queuing' of a RIGHTmost item that was in process
        
        for (i=0; i<nr; i++) {
            if (queue[i].enabled && queue[i].status == BUILDING)
                build_class_mask |= structureInfo[queue[i].itemInfo].class_mask;
        }
        
        queue[nr].status = ((build_class_mask & structureInfo[queue[nr].itemInfo].class_mask)==0) ?
                            BUILDING :
                            QUEUED;
       
        // should 're-queue' something at its RIGHT?
        for (i=(nr+1); i<maxItems; i++) {
            if (queue[i].enabled && queue[i].status == BUILDING) {
                if (structureInfo[queue[nr].itemInfo].class_mask == structureInfo[queue[i].itemInfo].class_mask) {
                    queue[i].status = QUEUED;
                    break;
                }
            }
        }
    }
    return 1;
}

void freezeItemStructuresQueueNr(enum Side side, int nr) {
    struct BuildQueueItem *queue = stats[(int) side].structuresQueue;
    if (queue[nr].status == DONE)
        queue[nr].status = FROZEN;
}

void unfreezeItemStructuresQueueWithInfo(enum Side side, int itemInfo) {
    struct BuildQueueItem *queue = stats[(int) side].structuresQueue;
    int i;
    
    int maxItems = (side == FRIENDLY) ? FRIENDLY_STRUCTURES_QUEUE : ENEMY_STRUCTURES_QUEUE;
    
    for (i=0; i<maxItems; i++) {
        if (queue[i].status == FROZEN && queue[i].itemInfo == itemInfo) {
            queue[i].status = DONE;
            return;
        }
    }
}

void updateStructuresQueue(enum Side side) {
    struct BuildQueueItem *queue = stats[(int) side].structuresQueue;
    int creditsRequired;
    int i, j;
    int build_class_mask=0;
    bool building_completed=false;
    
    int maxItems = (side == FRIENDLY) ? FRIENDLY_STRUCTURES_QUEUE : ENEMY_STRUCTURES_QUEUE;
    
    for (i=0; i<maxItems && queue[i].enabled; i++) {
        for (j=0; j<MAX_STRUCTURES_ON_MAP; j++) {
            if (structure[j].enabled && structure[j].side == side && structure[j].info == queue[i].builderInfo && structure[j].primary)
                break;
        }
        if (j == MAX_STRUCTURES_ON_MAP) { // the structure that could build this item was destroyed and has to be removed
            removeItemStructuresQueueNr(side, i, 0, 0);
            i--;
        }
    }
    
    for (i=0; i<maxItems; i++) {
        if (queue[i].enabled && queue[i].status == BUILDING) {
            creditsRequired = ((structureInfo[queue[i].itemInfo].build_cost * (queue[i].completion + 1)) / structureInfo[queue[i].itemInfo].build_time) -
                              ((structureInfo[queue[i].itemInfo].build_cost * (queue[i].completion)) / structureInfo[queue[i].itemInfo].build_time);
            if (changeCredits(side, -creditsRequired)) {
                queue[i].completion++;
                if (queue[i].completion >= structureInfo[queue[i].itemInfo].build_time) {
                    queue[i].status = DONE;
                    building_completed = true;
                } 
                else {  
                    build_class_mask |= structureInfo[queue[i].itemInfo].class_mask;
                }
                
            }
            // break; /* no need to check the other items in the queue because they're definitely not set to BUILDING */
            // above line removed (there can be more than one BUILDING now!)
        }
    }
    
    if (building_completed) {  // we might have a/some structure(s) queued waiting
        // play SFX only once even for two completed constructions at the same time
        if (side == FRIENDLY)
            playSoundeffect(SE_CONSTRUCTION_COMPLETE);
            
        // switch to BUILDING if QUEUED and belonging to a 'free' class
        for (i=0; i<maxItems; i++) {
            if (queue[i].enabled && (queue[i].status == QUEUED) && ((build_class_mask & structureInfo[queue[i].itemInfo].class_mask)==0)) {
                queue[i].status = BUILDING;
                build_class_mask |= structureInfo[queue[i].itemInfo].class_mask;
            }
        }
    }
}

void initStructuresQueue(enum Side side) {
    struct BuildQueueItem *queue = stats[(int) side].structuresQueue;
    int i;
    
    int maxItems = (side == FRIENDLY) ? FRIENDLY_STRUCTURES_QUEUE : ENEMY_STRUCTURES_QUEUE;
    
    for (i=0; i<maxItems; i++)
        queue[i].enabled = 0;
}

inline struct BuildQueueItem *getStructuresQueue(enum Side side) {
    return stats[(int) side].structuresQueue;
}


int availableBuilderForItem(enum Side side, int isStructure, int itemInfo) {
    int amountOfItems;
    struct TechtreeItem *curItem;
    int info;
    int i, j, k;
    
    amountOfItems = techtree[side].amountOfItems;
    curItem = techtree[side].item;
    for (i=0; i<amountOfItems; i++, curItem++) {
        if (curItem->buildableInfo == itemInfo && curItem->buildStructure == isStructure) {
            // check for the existance of its requiredInfos and buildingInfo
            k=0;
            for (j=0; j<MAX_REQUIREDINFO_COUNT; j++) {
                info = curItem->requiredInfo[j];
                if (info >= 0) {
                    for (k=0; k<MAX_STRUCTURES_ON_MAP; k++) {
                        if (structure[k].info == info && structure[k].enabled && structure[k].side == side)
                            break;
                    }
                    if (k == MAX_STRUCTURES_ON_MAP) // not possible with the current techtree item/rule
                        break;
                }
            }
            if (k == MAX_STRUCTURES_ON_MAP) // not possible with the current techtree item/rule
                continue;
            
            info = curItem->buildingInfo;
            for (j=0; j<MAX_STRUCTURES_ON_MAP; j++) {
                if (structure[j].info == info && structure[j].enabled && structure[j].side == side)
                    return info;
            }
        }
    }
    
    return -1;
}


int getCredits(enum Side side) { // only used for visuals in the case of Friendly
    if (side != FRIENDLY)
        return stats[side].ore_holding;
    
    // side == FRIENDLY
    if (stats[side].ore_showing < stats[side].ore_holding) {
        stats[side].ore_showing++;
        if (stats[side].ore_showing >= stats[side].ore_storage)
            playSoundeffect(SE_CREDITS_MAXED);
        else
            playSoundeffect(SE_CREDIT);
    } else if (stats[side].ore_showing > stats[side].ore_holding) {
        stats[side].ore_showing--;
        if (stats[side].ore_showing == 0)
            playSoundeffect(SE_CREDITS_NONE);
        else
            playSoundeffect(SE_CREDIT);
    }
    return stats[side].ore_showing;
}


void initCredits(enum Side side, int amount) {
    stats[(int) side].ore_holding = amount;
    stats[(int) side].ore_showing = amount;
}


int changeCredits(enum Side side, int amount) {
    int i, j;
    struct Stats *info = &stats[(int) side];
    struct Structure *curStructure;

    if (getGameType() == SINGLEPLAYER) {
        // CPU cheats when it comes to money
        if (side == FRIENDLY && amount > 0) {
            for (i=1; i<amountOfSides; i++) {
                curStructure = structure;
                for (j=0; j<MAX_STRUCTURES_ON_MAP; j++, curStructure++) {
                    if (curStructure->side == i && curStructure->enabled && structureInfo[curStructure->info].can_extract_ore) {
                        stats[i].ore_holding += amount;
                        if (stats[i].ore_holding > stats[i].ore_storage)
                            stats[i].ore_holding = stats[i].ore_storage;
                        if (stats[i].ore_holding > 99999)
                            stats[i].ore_holding = 99999;
                        break;
                    }
                }
            }
        }
    }

    if (info->ore_holding + amount < 0)
        return 0;
    if (info->ore_holding >= info->ore_storage && amount >= 0) /* no need to add credits if it's already over the top */
        return 1;
    info->ore_holding += amount;
    if (info->ore_holding > info->ore_storage && amount >= 0) /* some credits are lost ;p */
        info->ore_holding = info->ore_storage;
    if (info->ore_holding > 99999)
        info->ore_holding = 99999;
    
    return 1;
}

void changeCreditsNoLimit(enum Side side, int amount) {
    struct Stats *info = &stats[(int) side];
    info->ore_holding += amount;
    info->ore_showing += amount;
}

int getOreStorage(enum Side side) {
    return stats[(int) side].ore_storage;
}

void setOreStorage(enum Side side, int amount) {
    if (amount < 0) amount = 0;
    stats[(int) side].ore_storage = amount;
}

int getPowerConsumation(enum Side side) {
    return stats[(int) side].power_consuming;
}

void setPowerConsumation(enum Side side, int amount) {
    if (amount < 0) amount = 0;
    if (side == FRIENDLY) {
        if (amount > stats[(int) side].power_consuming) { // it increased
            if (stats[(int) side].power_consuming <= stats[(int) side].power_generating && amount > stats[(int) side].power_generating)
                playSoundeffect(SE_POWER_OFF);
        } else if (amount < stats[(int) side].power_consuming) { // it decreased
            if (stats[(int) side].power_consuming > stats[(int) side].power_generating && amount <= stats[(int) side].power_generating)
                playSoundeffect(SE_POWER_ON);
        }
    }
    stats[(int) side].power_consuming = amount;
}

int getPowerGeneration(enum Side side) {
    return stats[(int) side].power_generating;
}

void setPowerGeneration(enum Side side, int amount) {
    if (amount < 0) amount = 0;
    if (side == FRIENDLY) {
        if (amount > stats[(int) side].power_generating) { // it increased
            if (stats[(int) side].power_consuming > stats[(int) side].power_generating && stats[(int) side].power_consuming <= amount)
                playSoundeffect(SE_POWER_ON);
        } else if (amount < stats[(int) side].power_generating) { // it decreased
            if (stats[(int) side].power_consuming <= stats[(int) side].power_generating && stats[(int) side].power_consuming > amount)
                playSoundeffect(SE_POWER_OFF);
        }
    }
    stats[(int) side].power_generating = amount;
}

inline struct Stats *getStats() {
    return stats;
}

int getStatsSaveSize(void) {
    return sizeof(stats);
}

int getStatsSaveData(void *dest, int max_size) {
    int size=sizeof(stats);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&stats,size);
    return (size);
}
