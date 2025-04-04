// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _AI_H_
#define _AI_H_

#include "structures.h"

#define MAX_DIFFERENT_PRIORITY_STRUCTURES MAX_DIFFERENT_STRUCTURES

#define DEFAULT_REBUILD_DELAY_STRUCTURES  (((20 * FPS) * 2) / getGameSpeed())
#define DEFAULT_REBUILD_DELAY_UNITS       (((20 * FPS) * 2) / getGameSpeed())
#define REBUILD_DELAY_CONCRETE            ((( 2 * FPS) * 2) / getGameSpeed())

struct PriorityStructureAI {
    int amountOfItems;
    int item[MAX_DIFFERENT_PRIORITY_STRUCTURES];
};

struct PriorityStructureAI *getPriorityStructureAI();
void initPriorityStructureAI();


void adjustAIUnitAdded(enum Side side, int itemNr, int unitNr);

void removeAIRebuildUnitNr(enum Side side, int nr);
void removeAIRebuildStructureNr(enum Side side, int nr);

void doAILogic();
void initAI();

struct TeamAI *getTeamAI();
int getTeamAISaveSize(void);
int getTeamAISaveData(void *dest, int max_size);

struct RebuildQueue *getRebuildQueue();
int getRebuildQueueSaveSize(void);
int getRebuildQueueSaveData(void *dest, int max_size);

#endif
