// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _INFO_H_
#define _INFO_H_

#include "shared.h"

#include "structures.h"
#include "units.h"


int getAmountOfSides();
void setAmountOfSides(int amount);

int getMatchTime();
void setMatchTime(int time);
void updateMatchTime();
void initMatchTime();

int getUnitCount(enum Side side);
void setUnitCount(enum Side side, int amount);

int getUnitLimit(enum Side side);
void setUnitLimit(enum Side side, int amount);

int getUnitDeaths(enum Side side);
void setUnitDeaths(enum Side side, int amount);

int getStructureCount(enum Side side);
void setStructureCount(enum Side side, int amount);

int getStructureDeaths(enum Side side);
void setStructureDeaths(enum Side side, int amount);



#define MAX_REQUIREDINFO_COUNT 2

struct TechtreeItem { /* level not needed, since too high levels will be disregarded when reading in a techtree */
    int buildingInfo;                           // the structure which actually builds
    int requiredInfo[MAX_REQUIREDINFO_COUNT];   // the structure also required to build
    int buildStructure;                         // 1: buildable is structure, 0: buildable is unit
    int buildableInfo;                          // the buildable
};

struct Techtree {
    int amountOfItems;
    struct TechtreeItem item[MAX_DIFFERENT_STRUCTURES + MAX_DIFFERENT_UNITS];
};

struct Techtree *getTechtree(enum Side side);
int addItemTechtree(enum Side side, struct TechtreeItem *item);
void initTechtree(enum Side side);


enum QueueStatus { QUEUED, PAUSED, BUILDING, DONE, FROZEN };

struct BuildQueueItem {
    int enabled;
    int itemInfo;
    int builderInfo;
    enum QueueStatus status;
    int completion; /* from 0 to amount-of-frames it should last */
};

#define FRIENDLY_UNITS_QUEUE    10
#define ENEMY_UNITS_QUEUE       30
#define MAX_UNITS_QUEUE         ((FRIENDLY_UNITS_QUEUE > ENEMY_UNITS_QUEUE) ? FRIENDLY_UNITS_QUEUE : ENEMY_UNITS_QUEUE)
/*  this queue holds 10 units max for friendly, and can build units of different types at once */
int addItemUnitsQueue(enum Side side, int itemInfo, int builderInfo);
int removeItemUnitsQueueWithInfo(enum Side side, int itemInfo, int withRefund, int frozen);
int removeItemUnitsQueueNr(enum Side side, int nr, int withRefund, int frozen);
int togglePauseItemUnitsQueueNr(enum Side side, int nr);
void freezeItemUnitsQueueNr(enum Side side, int nr);
void unfreezeItemUnitsQueueWithInfo(enum Side side, int info);
int verifyUnitCoBuilderExists(int builderInfo, enum Side side);
void updateUnitsQueue(enum Side side);
void initUnitsQueue(enum Side side);
struct BuildQueueItem *getUnitsQueue(enum Side side);

#define FRIENDLY_STRUCTURES_QUEUE   5
#define ENEMY_STRUCTURES_QUEUE      10
#define MAX_STRUCTURES_QUEUE        ((FRIENDLY_STRUCTURES_QUEUE > ENEMY_STRUCTURES_QUEUE) ? FRIENDLY_STRUCTURES_QUEUE : ENEMY_STRUCTURES_QUEUE)
/* this queue holds 5 structures max for friendly, and can only build one at a time */
int addItemStructuresQueue(enum Side side, int itemInfo, int builderInfo);
int removeItemStructuresQueueWithInfo(enum Side side, int itemInfo, int withRefund, int frozen);
int removeItemStructuresQueueNr(enum Side side, int nr, int withRefund, int frozen);
int togglePauseItemStructuresQueueNr(enum Side side, int nr);
void freezeItemStructuresQueueNr(enum Side side, int nr);
void unfreezeItemStructuresQueueWithInfo(enum Side side, int itemInfo);
void updateStructuresQueue(enum Side side);
void initStructuresQueue(enum Side side);
struct BuildQueueItem *getStructuresQueue(enum Side side);

int availableBuilderForItem(enum Side side, int isStructure, int itemInfo);

int getOreStorage(enum Side side);
void setOreStorage(enum Side side, int amount);
int getCredits(enum Side side);
int changeCredits(enum Side side, int amount);
void changeCreditsNoLimit(enum Side side, int amount);
void initCredits(enum Side side, int amount);

int getPowerConsumation(enum Side side);
void setPowerConsumation(enum Side side, int amount);
int getPowerGeneration(enum Side side);
void setPowerGeneration(enum Side side, int amount);

struct Stats *getStats();
int getStatsSaveSize(void);
int getStatsSaveData(void *dest, int max_size);

#endif
