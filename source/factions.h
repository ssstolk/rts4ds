// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _FACTIONS_H_
#define _FACTIONS_H_

#include "shared.h"

#include <nds.h>

#define MAX_DIFFERENT_FACTIONS    5
#define MAX_FACTION_NAME_LENGTH   30


struct FactionInfo {
    int enabled;
    char name[MAX_FACTION_NAME_LENGTH+1];
//  char description[MAX_DESCRIPTION_LENGTH+1];
    uint16 colour;
    
    int unlock_faction_info;
    int unlock_faction_level;
};


extern struct FactionInfo factionInfo[];

int getFaction(enum Side side);
void setFaction(enum Side side, int nr);
int getFactionTechLevel(enum Side side);
int getRebuildDelayStructures(enum Side side);
int getRebuildDelayUnits(enum Side side);
int getRepairSpeed(enum Side side);
int getUnitBuildSpeed(enum Side side);
enum Side getNeutralSide();

void initFactions();
void initFactionsWithScenario();

#endif
