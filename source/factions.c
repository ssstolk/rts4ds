// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "factions.h"

#include "game.h"
#include "info.h"
#include "ai.h"
#include "fileio.h"


struct FactionInfo factionInfo[MAX_DIFFERENT_FACTIONS];
int faction[MAX_DIFFERENT_FACTIONS];
int factionTechLevel[MAX_DIFFERENT_FACTIONS];
int rebuildDelayStructures[MAX_DIFFERENT_FACTIONS];
int rebuildDelayUnits[MAX_DIFFERENT_FACTIONS];
int repairSpeed[MAX_DIFFERENT_FACTIONS];
int unitBuildSpeed[MAX_DIFFERENT_FACTIONS];
enum Side neutralSide;


int getFaction(enum Side side) { return faction[side]; }
void setFaction(enum Side side, int nr) { faction[side] = nr; }
int getFactionTechLevel(enum Side side) { return factionTechLevel[side]; }
int getRebuildDelayStructures(enum Side side) { return rebuildDelayStructures[side]; }
int getRebuildDelayUnits(enum Side side) { return rebuildDelayUnits[side]; }
int getRepairSpeed(enum Side side) { return repairSpeed[side]; }
int getUnitBuildSpeed(enum Side side) { return unitBuildSpeed[side]; }
inline enum Side getNeutralSide() { return neutralSide; }


void initFactions() {
    int amountOfFactions = 0;
    char oneline[256];
    FILE *fp;
    int i, j, k, l;

    fp = openFile("factions.ini", FS_PROJECT_FILE);
    readstr(fp, oneline);
    sscanf(oneline, "AMOUNT-OF-FACTIONS %i", &amountOfFactions);
    if (amountOfFactions > MAX_DIFFERENT_FACTIONS)
        errorSI("Too many types of factions exist. Limit is:", MAX_DIFFERENT_FACTIONS);
    amountOfFactions %= (MAX_DIFFERENT_FACTIONS + 1);
    for (i=0; i<amountOfFactions; i++) {
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 255);
        strcpy(factionInfo[i].name, oneline);
        factionInfo[i].enabled = 1;
    }
    for (; i<MAX_DIFFERENT_FACTIONS; i++) {
        factionInfo[i].enabled = 0;
    }
    closeFile(fp);
    
    for (i=0; i<amountOfFactions; i++) {
        fp = openFile(factionInfo[i].name, FS_FACTIONS_INFO);
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[GENERAL]", strlen("[GENERAL]")));
        readstr(fp, oneline);
        readstr(fp, oneline);
        sscanf(oneline, "Colour=%i,%i,%i", &j, &k, &l);
        factionInfo[i].colour = RGB15(j, k, l);
        
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[UNLOCKED]", strlen("[UNLOCKED]")));
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 255);
        for (j=0; j<amountOfFactions; j++) {
            if (!strcmp(factionInfo[j].name, oneline + strlen("Faction="))) {
                factionInfo[i].unlock_faction_info = j;
                readstr(fp, oneline);
                sscanf(oneline, "Level=%i", &factionInfo[i].unlock_faction_level);
                break;
            }
        }
        if (j == amountOfFactions)
            error("A non-existant Faction was specified as Faction for unlocking:", factionInfo[i].name);
        closeFile(fp);
    }
    
    faction[FRIENDLY] = 0;
}

void initFactionsWithScenario() {
    int i, j;
    char oneline[256];
    FILE *fp;
    
    neutralSide = MAX_SIDES; // ensuring neutral side is set to none at first
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    
    for (i=0; i<MAX_DIFFERENT_FACTIONS; i++)
        faction[i] = -1;
    
    // FRIENDLY section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[FRIENDLY]", strlen("[FRIENDLY]")));
    readstr(fp, oneline);
    i=0;
    // check which faction is friendly
    while (factionInfo[i].enabled && i<MAX_DIFFERENT_FACTIONS) {
        if (!strncmp(oneline + strlen("Faction="), factionInfo[i].name, strlen(factionInfo[i].name))) {
            faction[FRIENDLY] = i;
            break;
        }
        i++;
    }
    if (faction[FRIENDLY] == -1) {
        replaceEOLwithEOF(oneline, 255);
        error("Scenario's friendly faction does not exist:", oneline + strlen("Faction="));
    }
    readstr(fp, oneline);
    sscanf(oneline, "TechLevel=%i", &factionTechLevel[FRIENDLY]);
    repairSpeed[FRIENDLY] = 100;
    unitBuildSpeed[FRIENDLY] = 100;
    
    setAmountOfSides(1);
    
    // ENEMY section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[ENEMY", strlen("[ENEMY")));
    for (i=1; i<MAX_DIFFERENT_FACTIONS; i++) {
        readstr(fp, oneline);
        // check which faction is enemy
        for (j=0; j<MAX_DIFFERENT_FACTIONS && factionInfo[j].enabled; j++) {
            if (!strncmp(oneline + strlen("Faction="), factionInfo[j].name, strlen(factionInfo[j].name))) {
                faction[i] = j;
//              if (j == NEUTRAL_FACTION && neutralSide == MAX_SIDES) // plebs is neutral, but any ForceNeutrality sides can overrule this
//                  neutralSide = i;
                break;
            }
        }
        if (faction[i] == -1) {
            replaceEOLwithEOF(oneline, 255);
            error("Scenario's enemy faction does not exist:", oneline + strlen("Faction="));
        }
        readstr(fp, oneline);
        sscanf(oneline, "TechLevel=%i", &factionTechLevel[i]);
        readstr(fp, oneline);
        if (oneline[0] != 'R' || sscanf(oneline, "RepairSpeed=%i", &repairSpeed[i]) < 1)
            repairSpeed[i] = 100; // default value
        
        setAmountOfSides(getAmountOfSides() + 1);
        
        // we still have some optional lines to read in, but also to skip the preceding lines Credits and MaxUnit
        
        rebuildDelayStructures[i] = DEFAULT_REBUILD_DELAY_STRUCTURES;
        rebuildDelayUnits[i] = DEFAULT_REBUILD_DELAY_UNITS;
        unitBuildSpeed[i] = 100; /* 100%, normal speed */
        
        while (oneline[0] != '[') {
            if (!strncmp(oneline, "RebuildDelayStructures=", strlen("RebuildDelayStructures="))) {
                oneline[0] = oneline[strlen("RebuildDelayStructures=")];
                if (oneline[0] < '0' || oneline[0] > '9')
                    rebuildDelayStructures[i] = -1;
                else {
                    sscanf(oneline + strlen("RebuildDelayStructures="), "%i", &rebuildDelayStructures[i]);
                    rebuildDelayStructures[i] = ((rebuildDelayStructures[i] * FPS) * 2) / getGameSpeed();
                }
            } else if (!strncmp(oneline, "RebuildDelayUnits=", strlen("RebuildDelayUnits="))) {
                oneline[0] = oneline[strlen("RebuildDelayUnits=")];
                if (oneline[0] < '0' || oneline[0] > '9')
                    rebuildDelayUnits[i] = -1;
                else {
                    sscanf(oneline + strlen("RebuildDelayUnits="), "%i", &rebuildDelayUnits[i]);
                    rebuildDelayUnits[i] = ((rebuildDelayUnits[i] * FPS) * 2) / getGameSpeed();
                }
            } else if (!strncmp(oneline, "UnitBuildSpeed=", strlen("UnitBuildSpeed="))) {
                sscanf(oneline, "UnitBuildSpeed=%u%%", &unitBuildSpeed[i]); //example: UnitBuildSpeed=30% (or 160%, etc)
            } else if (!strncmp(oneline, "ForceNeutrality", strlen("ForceNeutrality"))) {
                neutralSide = i; // ForceNeutrality automatically overwrites any default neutral side
            }
            readstr(fp, oneline);
        }
        if (strncmp(oneline, "[ENEMY", strlen("[ENEMY")))
            break;
    }
    
    if (getGameType() == MULTIPLAYER_CLIENT) {
        i = faction[FRIENDLY];
        faction[FRIENDLY] = faction[ENEMY1];
        faction[ENEMY1] = i;
    }
    
    closeFile(fp);
}
