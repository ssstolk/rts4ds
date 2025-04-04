// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "units.h"

#include "fileio.h"
#include "sprites.h"
#include "view.h"
#include "game.h"
#include "info.h"
#include "ai.h"
#include "factions.h"
#include "environment.h"
#include "overlay.h"
#include "explosions.h"
#include "structures.h"
#include "radar.h"
#include "shared.h"
#include "playscreen.h"
#include "soundeffects.h"
#include "pathfinding.h"

#define USE_REDUCED_UNIT_SELECTED_GFX
#define USE_REDUCED_UNIT_COLLECTED_GFX


struct UnitReinforcement {
    int enabled;
    int delay;
    enum Side side;
    int info;
    int x;
    int y;
    int armour;
    int logic_aid;
};
struct UnitReinforcement unitReinforcement[MAX_REINFORCEMENTS];


struct UnitInfo unitInfo[MAX_DIFFERENT_UNITS];
struct Unit unit[MAX_UNITS_ON_MAP];
int focusOnUnitNr;

static int unit_smoke_graphics_offset = 0;
static int unit_bleed_graphics_offset;
static int unit_selected_graphics_offset;
static int unit_collected_graphics_offset;
static int base_unit_flag;

static int unitMultiplayerIdCounter;




inline int getFocusOnUnitNr() {
    return focusOnUnitNr;
}

inline void setFocusOnUnitNr(int nr) {
    focusOnUnitNr = nr;
}


void initUnits() {
    char oneline[256];
    int amountOfUnits = 0;
    int i, j;
    FILE *fp;
    
    // get the names of all available units
    fp = openFile("units.ini", FS_PROJECT_FILE);
    readstr(fp, oneline);
    sscanf(oneline, "AMOUNT-OF-UNITS %i", &amountOfUnits);
    if (amountOfUnits > MAX_DIFFERENT_UNITS)
        errorSI("Too many types of units exist. Limit is:", MAX_DIFFERENT_UNITS);
    for (i=0; i<amountOfUnits; i++) {
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 255);
        strcpy(unitInfo[i].name, oneline);
        unitInfo[i].enabled = 1;
    }
    closeFile(fp);
    
    // get information on each and every available unit
    for (i=0; i<amountOfUnits; i++) {
        fp = openFile(unitInfo[i].name, FS_UNITS_INFO);
        
        // GENERAL section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[GENERAL]", strlen("[GENERAL]")));
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 255);
        strcpy(unitInfo[i].description, oneline + strlen("Description="));
        readstr(fp, oneline);
        if (!strncmp(oneline + strlen("Type="), "Foot", strlen("Foot")))
            unitInfo[i].type = UT_FOOT;
        else if (!strncmp(oneline + strlen("Type="), "Wheeled", strlen("Wheeled")))
            unitInfo[i].type = UT_WHEELED;
        else if (!strncmp(oneline + strlen("Type="), "Tracked", strlen("Tracked")))
            unitInfo[i].type = UT_TRACKED;
        else if (!strncmp(oneline + strlen("Type="), "Aerial", strlen("Aerial")))
            unitInfo[i].type = UT_AERIAL;
        else if (!strncmp(oneline + strlen("Type="), "Creature", strlen("Creature")))
            unitInfo[i].type = UT_CREATURE;
        else {
            replaceEOLwithEOF(oneline, 255);
            error("Unit had unrecognised type called:", oneline + strlen("Type="));
        }
        
        // CONSTRUCTING section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[CONSTRUCTING]", strlen("[CONSTRUCTING]")));
        readstr(fp, oneline);
        sscanf(oneline, "Cost=%i", &unitInfo[i].build_cost);
        readstr(fp, oneline);
        sscanf(oneline, "Speed=%i", &unitInfo[i].original_build_time);
        unitInfo[i].original_build_time *= FPS;
        
        // REPAIRING section
        if (unitInfo[i].type >= UT_WHEELED) {
            do {
                readstr(fp, oneline);
            } while (strncmp(oneline, "[REPAIRING]", strlen("[REPAIRING]")));
            readstr(fp, oneline);
            sscanf(oneline, "Cost=%i", &unitInfo[i].repair_cost);
            readstr(fp, oneline);
            sscanf(oneline, "Speed=%i", &unitInfo[i].original_repair_time);
            unitInfo[i].original_repair_time *= FPS;
        }
        
        // MEASUREMENTS section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[MEASUREMENTS]", strlen("[MEASUREMENTS]")));
        readstr(fp, oneline);
        sscanf(oneline, "Size=%i", &j);
        unitInfo[i].graphics_size = (unsigned int) j;
        
        // ANIMATION section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[ANIMATION]", strlen("[ANIMATION]")));
        readstr(fp, oneline);
        if (oneline[0] == 'M') {
            sscanf(oneline, "Mine=%i", &unitInfo[i].shoot_ani);
            unitInfo[i].shoot_ani++;
        } else
            sscanf(oneline, "Shoot=%i", &unitInfo[i].shoot_ani);
        readstr(fp, oneline);
        sscanf(oneline, "Move=%i", &unitInfo[i].move_ani);
        readstr(fp, oneline);
        sscanf(oneline, "Rotation=%i", &unitInfo[i].rotation_ani);
        readstr(fp, oneline);
        if (oneline[0] == '[')
            unitInfo[i].idle_ani = 0;
        else {
            sscanf(oneline, "LookAround=%u", &unitInfo[i].idle_ani);
            unitInfo[i].idle_ani *= FPS;
        
        // FLAGS section
            do {
                readstr(fp, oneline);
            } while (strncmp(oneline, "[FLAGS]", strlen("[FLAGS]")));
        }
        readstr(fp, oneline);
        sscanf(oneline, "Type=%i", &unitInfo[i].flag_type);
        
        // VIEW section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[VIEW]", strlen("[VIEW]")));
        readstr(fp, oneline);
        sscanf(oneline, "Range=%i", &unitInfo[i].view_range);
        
        // WEAPON section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[WEAPON]", strlen("[WEAPON]")));
        readstr(fp, oneline);
        sscanf(oneline, "Range=%i", &unitInfo[i].shoot_range);
        if (unitInfo[i].shoot_range != 0) { // reload time and projectile needs checking
            readstr(fp, oneline);
            sscanf(oneline, "Reload-Time=%i", &unitInfo[i].original_reload_time);
            unitInfo[i].original_reload_time = (unitInfo[i].original_reload_time * FPS) / 4;
            readstr(fp, oneline);
            sscanf(oneline, "Double-Shot=%i", &unitInfo[i].double_shot);
            readstr(fp, oneline);
            
            if (oneline[0] == 'T') {
                sscanf(oneline, "Tank-Shot=%i", &unitInfo[i].tank_shot);
                readstr(fp, oneline);
            }
            
            unitInfo[i].projectile_info = -1;
            for (j=0; j<MAX_DIFFERENT_PROJECTILES && projectileInfo[j].enabled; j++) {
                if (!strncmp(oneline + strlen("Projectile="), projectileInfo[j].name, strlen(projectileInfo[j].name))) {
                    unitInfo[i].projectile_info = j;
                    unitInfo[i].can_heal_foot = (projectileInfo[j].power < 0);
                    break;
                }
            }
        } else {
            unitInfo[i].projectile_info = -1;
            unitInfo[i].original_reload_time = 0;
            unitInfo[i].double_shot = 0;
            unitInfo[i].tank_shot = 0;
        }
        
        // ARMOUR section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[ARMOUR]", strlen("[ARMOUR]")));
        readstr(fp, oneline);
        sscanf(oneline, "Armour=%i", &unitInfo[i].max_armour);
        if (unitInfo[i].max_armour < 0)
            unitInfo[i].max_armour = INFINITE_ARMOUR;
        
        // EXPLOSION section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[EXPLOSION]", strlen("[EXPLOSION]")));
        readstr(fp, oneline);
        unitInfo[i].explosion_info = -1;
        for (j=0; j<MAX_DIFFERENT_EXPLOSIONS && explosionInfo[j].enabled; j++) {
            if (!strncmp(oneline + strlen("Explosion="), explosionInfo[j].name, strlen(explosionInfo[j].name))) {
                unitInfo[i].explosion_info = j;
                break;
            }
        }
        readstr(fp, oneline);
        if (oneline[0] == '[')
            unitInfo[i].death_soundeffect = -1;
        else {
            sscanf(oneline, "DeathSound=%i", &unitInfo[i].death_soundeffect);
            unitInfo[i].death_soundeffect--;
        
        // MOVEMENT section
            do {
                readstr(fp, oneline);
            } while (strncmp(oneline, "[MOVEMENT]", strlen("[MOVEMENT]")));
        }
        readstr(fp, oneline);
        sscanf(oneline, "Speed=%i", &unitInfo[i].original_speed);
        unitInfo[i].original_speed = (unitInfo[i].original_speed * FPS) / 4;
        readstr(fp, oneline);
        sscanf(oneline, "RotateSpeed=%i", &unitInfo[i].original_rotate_speed);
        unitInfo[i].original_rotate_speed = (unitInfo[i].original_rotate_speed * FPS) / 4;
        readstr(fp, oneline);
        if (oneline[0] == '[')
            unitInfo[i].tracks_type = -1;
        else {
            sscanf(oneline, "Tracks=%i", &unitInfo[i].tracks_type);
            unitInfo[i].tracks_type--;
        
        // MISC section
            do {
                readstr(fp, oneline);
            } while (strncmp(oneline, "[MISC]", strlen("[MISC]")));
        }
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 256);
        unitInfo[i].contains_structure = getStructureNameInfo(oneline + strlen("Contains-Structure="));
        readstr(fp, oneline);
        sscanf(oneline, "Can-Rotate-Turret=%i", &unitInfo[i].can_rotate_turret);
        readstr(fp, oneline);
//        sscanf(oneline, "Can-Contain-Unit=%i", &unitInfo[i].can_contain_unit);
        readstr(fp, oneline);
//        sscanf(oneline, "Can-Selfdestruct=%i", &unitInfo[i].can_selfdestruct);
        readstr(fp, oneline);
        sscanf(oneline, "Can-Collect-Ore=%i", &unitInfo[i].can_collect_ore);
        closeFile(fp);
    }
    for (i=amountOfUnits; i<MAX_DIFFERENT_UNITS; i++) // setting all unused ones to disabled
        unitInfo[i].enabled = 0;
}

void initUnitsSpeed() {
    int i;
    int gameSpeed = getGameSpeed();
    
    for (i=0; i<MAX_DIFFERENT_UNITS && unitInfo[i].enabled; i++) {
        unitInfo[i].build_time = (2*unitInfo[i].original_build_time) / gameSpeed;
        unitInfo[i].repair_time = (2*unitInfo[i].original_repair_time) / gameSpeed;
        unitInfo[i].speed = (2*unitInfo[i].original_speed) / gameSpeed;
        unitInfo[i].rotate_speed = (2*unitInfo[i].original_rotate_speed) / gameSpeed;
        unitInfo[i].reload_time = (2*unitInfo[i].original_reload_time) / gameSpeed;
    }
}

void initUnitsWithScenario() {
    char oneline[256];
    char *positionBehindSide;
    char lastCharacter;
    int amountOfUnits = 0;
    int amountOfReinforcementUnits = 0;
    FILE *fp;
    int i,j,k;
    
    unitMultiplayerIdCounter = 0;
    focusOnUnitNr = -1;
    
    j = environment.width*environment.height;
    for (i=0; i<j; i++)
        environment.layout[i].contains_unit = -1;
    
    for (i=0; i<MAX_DIFFERENT_FACTIONS; i++) {
        setUnitCount(i, 0);
        setUnitDeaths(i, 0);
    }
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);

    // UNITS section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[UNITS]", strlen("[UNITS]")));
    while (1) {
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 255);
        if (!strncmp(oneline, "Friendly,", strlen("Friendly,"))) {
            unit[amountOfUnits].side = FRIENDLY;
            positionBehindSide = oneline + strlen("Friendly,");
        } else if (!strncmp(oneline, "Enemy", strlen("Enemy"))) {
            sscanf(oneline, "Enemy%i,", &i);
            unit[amountOfUnits].side = i;
            positionBehindSide = oneline + strlen("Enemy#,");
        } else
            break;
        if (getGameType() == MULTIPLAYER_CLIENT)
            unit[amountOfUnits].side = !unit[amountOfUnits].side;
        
        for (i=0; i<MAX_DIFFERENT_UNITS && unitInfo[i].enabled; i++) {
            if (!strncmp(positionBehindSide, unitInfo[i].name, strlen(unitInfo[i].name)) && positionBehindSide[strlen(unitInfo[i].name)] == ',') {
                unit[amountOfUnits].info = i;
                break;
            }
        }
        if (i == MAX_DIFFERENT_UNITS || !unitInfo[i].enabled)
            error("In the scenario an unknown unit was mentioned", oneline);
            
        sscanf(positionBehindSide + strlen(unitInfo[i].name), ",%i,%i,%i,%i,", &j, &unit[amountOfUnits].x, &unit[amountOfUnits].y, &k);
        unit[amountOfUnits].armour = (j < 0) ? (INFINITE_ARMOUR) : (unitInfo[i].max_armour * j / 100);
        
        if (unit[amountOfUnits].x >= environment.width || unit[amountOfUnits].y >= environment.height)
            errorSI("One of the units in the scenario\nhas an out-of-map x, y.\n\nIt's unit nr", amountOfUnits + 1);
        
// TEMP POSITIONED STUFF
        unit[amountOfUnits].logic = UL_GUARD;
        unit[amountOfUnits].group = 0;
        if (getGameType() == SINGLEPLAYER) {
            if (unit[amountOfUnits].side != FRIENDLY) {
                lastCharacter = positionBehindSide[strlen(positionBehindSide) - 1];
                if (lastCharacter == 'g') { // nothing
                    unit[amountOfUnits].logic = UL_NOTHING;
                    unit[amountOfUnits].group = UGAI_NOTHING;
                } else if (lastCharacter == 'h') { // ambush
                    unit[amountOfUnits].logic = UL_AMBUSH;
                    unit[amountOfUnits].group = UGAI_AMBUSH;
                } else if (lastCharacter == 'e') { // kamikaze
                    unit[amountOfUnits].logic = UL_KAMIKAZE;
                    unit[amountOfUnits].group = UGAI_KAMIKAZE;
                } else if (lastCharacter == 'a') { // guard area
                    unit[amountOfUnits].logic = UL_GUARD_AREA;
                    unit[amountOfUnits].group = UGAI_GUARD_AREA;
                } else if (lastCharacter == 'l') { // patrol
                    unit[amountOfUnits].logic = UL_GUARD_AREA;
                    unit[amountOfUnits].group = UGAI_PATROL;
                } else if (lastCharacter == 't') { // Hunt, no delay specified
                    unit[amountOfUnits].logic = UL_HUNT;
                    unit[amountOfUnits].logic_aid = 0;
                    unit[amountOfUnits].group = UGAI_HUNT;
                } else if (lastCharacter == 'd') { // guard
                    // variables already set, so nothing to do here
                } else { // there are behaviours (Hunt|Attack) with parameters
                    char firstCharacter = positionBehindSide[strlen(unitInfo[i].name) + 5 + nrstrlen(j) + nrstrlen(unit[amountOfUnits].x) + nrstrlen(unit[amountOfUnits].y) + nrstrlen(k)];
                    if (firstCharacter == 'H') { // Hunt
                        unit[amountOfUnits].logic = UL_HUNT;
                        sscanf(positionBehindSide + strlen(unitInfo[i].name) + 6 + nrstrlen(j) + nrstrlen(unit[amountOfUnits].x) + nrstrlen(unit[amountOfUnits].y) + nrstrlen(k) + strlen("Hunt"), "%i", &unit[amountOfUnits].logic_aid);
                        unit[amountOfUnits].logic_aid = (2*(unit[amountOfUnits].logic_aid*FPS)) / getGameSpeed();
                        unit[amountOfUnits].group = UGAI_HUNT;
                        if (unit[amountOfUnits].x == 0 || unit[amountOfUnits].x == environment.width - 1 || unit[amountOfUnits].y == 0 || unit[amountOfUnits].y == environment.height - 1) {
                            unitReinforcement[amountOfReinforcementUnits].delay  = unit[amountOfUnits].logic_aid;
                            unitReinforcement[amountOfReinforcementUnits].side   = unit[amountOfUnits].side;
                            unitReinforcement[amountOfReinforcementUnits].info   = unit[amountOfUnits].info;
                            unitReinforcement[amountOfReinforcementUnits].x      = unit[amountOfUnits].x;
                            unitReinforcement[amountOfReinforcementUnits].y      = unit[amountOfUnits].y;
                            unitReinforcement[amountOfReinforcementUnits].armour = unit[amountOfUnits].armour;
                            unitReinforcement[amountOfReinforcementUnits].enabled = 1;
                            amountOfReinforcementUnits++;
                            continue;
                        }
                    } else if (firstCharacter == 'A') {  // Attack
                        int tgt_x, tgt_y;
                        sscanf(positionBehindSide + strlen(unitInfo[i].name) + 6 + nrstrlen(j) + nrstrlen(unit[amountOfUnits].x) + nrstrlen(unit[amountOfUnits].y) + nrstrlen(k) + strlen("Attack"), "%i,%i", &tgt_x, &tgt_y);
                        unit[amountOfUnits].logic = UL_ATTACK_AREA;
                        unit[amountOfUnits].logic_aid = TILE_FROM_XY(tgt_x, tgt_y);
                        unit[amountOfUnits].group = UGAI_HUNT; // retaliate if attacked!
                    } else {
                        // unknown behaviour (!!!)
                        error("An unknown behaviour has been specified", oneline);
                    }
                }
            } else { // unit[amountOfUnits].side == FRIENDLY
                char firstCharacter = positionBehindSide[strlen(unitInfo[i].name) + 5 + nrstrlen(j) + nrstrlen(unit[amountOfUnits].x) + nrstrlen(unit[amountOfUnits].y) + nrstrlen(k)];
                if (firstCharacter == 'R') {  // Reinforcement (behaviour for Friendly unit only!)
                    int tgt_x, tgt_y;
                    sscanf(positionBehindSide + strlen(unitInfo[i].name) + 6 + nrstrlen(j) + nrstrlen(unit[amountOfUnits].x) + nrstrlen(unit[amountOfUnits].y) + nrstrlen(k) + strlen("Reinforcement"), "%i,%i,%i", &unit[amountOfUnits].logic_aid, &tgt_x, &tgt_y);
                    unit[amountOfUnits].logic_aid = (2*(unit[amountOfUnits].logic_aid*FPS)) / getGameSpeed();
                    if (unit[amountOfUnits].x == 0 || unit[amountOfUnits].x == environment.width - 1 || unit[amountOfUnits].y == 0 || unit[amountOfUnits].y == environment.height - 1) {
                        unitReinforcement[amountOfReinforcementUnits].delay     = unit[amountOfUnits].logic_aid;
                        unitReinforcement[amountOfReinforcementUnits].side      = unit[amountOfUnits].side;
                        unitReinforcement[amountOfReinforcementUnits].info      = unit[amountOfUnits].info;
                        unitReinforcement[amountOfReinforcementUnits].x         = unit[amountOfUnits].x;
                        unitReinforcement[amountOfReinforcementUnits].y         = unit[amountOfUnits].y;
                        unitReinforcement[amountOfReinforcementUnits].armour    = unit[amountOfUnits].armour;
                        unitReinforcement[amountOfReinforcementUnits].logic_aid = TILE_FROM_XY(tgt_x, tgt_y);
                        unitReinforcement[amountOfReinforcementUnits].enabled = 1;
                        #ifdef DEBUG_BUILD
                        if (unitReinforcement[amountOfReinforcementUnits].logic_aid < 0 || unitReinforcement[amountOfReinforcementUnits].logic_aid >= environment.width * environment.height) errorSI("unitReinforcement-logicaid", unitReinforcement[amountOfReinforcementUnits].logic_aid);
                        #endif
                        amountOfReinforcementUnits++;
                        continue;
                    } else {
                        error("Reinforcement units should be placed on map borders, this one is not:", oneline);
                    }
                }
            }
        }
        
        unit[amountOfUnits].turret_positioned = unit[amountOfUnits].unit_positioned = (enum Positioned) (k%(((int)LEFT_UP)+1));
        
        unit[amountOfUnits].enabled = 1;
        unit[amountOfUnits].selected = 0;
        unit[amountOfUnits].move = UM_NONE;
        unit[amountOfUnits].guard_tile = TILE_FROM_XY(unit[amountOfUnits].x, unit[amountOfUnits].y);
        unit[amountOfUnits].retreat_tile = unit[amountOfUnits].guard_tile;
        unit[amountOfUnits].multiplayer_id = unitMultiplayerIdCounter++;
        unit[amountOfUnits].reload_time = unitInfo[unit[amountOfUnits].info].reload_time;
        unit[amountOfUnits].misfire_time = 0;
        #ifdef REMOVE_ASTAR_PATHFINDING
        unit[amountOfUnits].hugging_obstacle = 0;
        #endif
        unit[amountOfUnits].smoke_time = 0;
        // if (unitInfo[unit[amountOfUnits].info].double_shot && (unit[amountOfUnits].reload_time - (SHOOT_ANIMATION_DURATION_FAKED + 1) >= DOUBLE_SHOT_AT)) // try to make sure it doesn't look like it's shooting from the get-go
            unit[amountOfUnits].reload_time = max((unitInfo->double_shot?DOUBLE_SHOT_AT:0), unitInfo->reload_time - (SHOOT_ANIMATION_DURATION_FAKED + 1));
        environment.layout[unit[amountOfUnits].retreat_tile].contains_unit = amountOfUnits;
        
        if (unit[amountOfUnits].side == FRIENDLY)
            activateEntityView(unit[amountOfUnits].x, unit[amountOfUnits].y, 1, 1, unitInfo[unit[amountOfUnits].info].view_range);
        
        setUnitCount(unit[amountOfUnits].side, getUnitCount(unit[amountOfUnits].side) + 1);
        amountOfUnits++;
    }
    closeFile(fp);
    
    for (i=amountOfUnits; i<MAX_UNITS_ON_MAP; i++) {
        unit[i].enabled = 0;
        unit[i].group = 0;
    }
    
    for (i=amountOfReinforcementUnits; i<MAX_REINFORCEMENTS; i++)
        unitReinforcement[i].enabled = 0;
    
    // C28 - begin - check if UL_ATTACK_AREA is attacking a unit on that location instead of a structure or environment
    for (i=0; i<MAX_UNITS_ON_MAP; i++) {
        if (unit[i].enabled && unit[i].logic==UL_ATTACK_AREA) {
            j = environment.layout[unit[i].logic_aid].contains_unit;
            if (j >= 0) {
                unit[i].logic     = UL_ATTACK_UNIT;  // attack the unit
                unit[i].logic_aid = j;
            }
        }
    }
    // C28 - end
    
    initUnitsSpeed();
}


int getUnitNameInfo(char *buffer) {
    int i, len;
    
    len = strlen("None");
    if (!strncmp(buffer, "None", len) && (buffer[len]==0 || buffer[len]==','))
        return -1;
    
    for (i=0; i<MAX_DIFFERENT_UNITS && unitInfo[i].enabled; i++) {
        len = strlen(unitInfo[i].name);
        if (!strncmp(buffer, unitInfo[i].name, len) && (buffer[len]==0 || buffer[len]==','))
            return i;
    }
    error("Non existant unit name encountered:", buffer);
    return -1;
}


void reserveTileForUnit(int unit_nr, int x, int y, enum UnitMove move) {
    struct EnvironmentLayout *envLayout;
    
    if (move != UM_MOVE_UP && move != UM_MOVE_DOWN) {
        if (move >= UM_MOVE_RIGHT_UP && move <= UM_MOVE_RIGHT_DOWN)
            x += 1;
        else
            x -= 1;
    }
    if (move != UM_MOVE_LEFT && move != UM_MOVE_RIGHT) {
        if (move >= UM_MOVE_RIGHT_DOWN && move <= UM_MOVE_LEFT_DOWN)
            y += 1;
        else
            y -= 1;
    }
    
    envLayout = &environment.layout[TILE_FROM_XY(x,y)];
    if (envLayout->contains_unit <= -1)
        envLayout->contains_unit = -unit_nr - 2;
}

int freeToMoveUnitViaTile(struct Unit *current, int tile) {
    struct EnvironmentLayout *envLayout = &environment.layout[tile];
    struct EnvironmentLayout *envLayoutStructure;
    struct StructureInfo *structInfo;
    
    if (tile == -1)
        return 0;
    
    #ifdef DEBUG_BUILD
    if (tile < 0 || tile >= environment.width * environment.height) errorSI("tile@freeToMoveUnitViaTile", tile);
    #endif
    
    // checking for a structure on that spot
    envLayoutStructure = envLayout;
    if (envLayoutStructure->contains_structure < -1) {
        #ifdef DEBUG_BUILD
        if ((tile + envLayoutStructure->contains_structure + 1) < 0 || (tile + envLayoutStructure->contains_structure + 1) >= environment.width * environment.height) errorSI("envLayoutStructure->contains_structure@freeToMoveUnitViaTile", envLayoutStructure->contains_structure);
        #endif
        envLayoutStructure += (envLayoutStructure->contains_structure + 1);
    }
    if (envLayoutStructure->contains_structure >= 0) {
        structInfo = &structureInfo[structure[envLayoutStructure->contains_structure].info];
        if (!structInfo->foundation) {
            if (structure[envLayoutStructure->contains_structure].side != current->side)
                return 0;
            if (!(((structInfo->can_extract_ore && unitInfo[current->info].can_collect_ore) || 
                (structInfo->can_repair_unit && unitInfo[current->info].type >= UT_WHEELED)) &&
                structure[envLayoutStructure->contains_structure].contains_unit == -1))
                return 0;
            
            // apparently unit is allowed to move in this particular structure, but does it want to?
            // only if its destination is the same structure as the structure on this tile
            if (current->logic == UL_RETREAT)
                envLayout = &environment.layout[current->retreat_tile];
            else
                envLayout = &environment.layout[current->logic_aid];
            if (envLayout->contains_structure < -1)
                envLayout += (envLayout->contains_structure + 1);
            if (envLayout == envLayoutStructure)
                return 1;
            
            return 0;
        }
    }
    
    // check for movement limited by the environment on this tile
    if (unitInfo[current->info].type == UT_AERIAL)
        return 1;
    
    if (envLayout->graphics >= SANDCHASM && envLayout->graphics <= SANDCHASM16)
        return 0;
    if (envLayout->graphics >= ROCKCHASM && envLayout->graphics <= ROCKCHASM16)
        return 0;
    if (envLayout->graphics >= CHASMCUSTOM)
        return 0;
    if (envLayout->graphics >= MOUNTAIN && envLayout->graphics <= MOUNTAIN16 && unitInfo[current->info].type == UT_TRACKED /*!= UT_FOOT*/)
        return 0;
    
    return 1;
}



int getNextTile(int curX, int curY, enum UnitMove move) {
    if (move != UM_MOVE_UP && move != UM_MOVE_DOWN) {
        if (move >= UM_MOVE_RIGHT_UP && move <= UM_MOVE_RIGHT_DOWN) {
            if (curX >= environment.width - 1)
                return -1;
            curX++;
        } else {
            if (curX <= 0)
                return -1;
            curX--;
        }
    }
    if (move != UM_MOVE_LEFT && move != UM_MOVE_RIGHT) {
        if (move >= UM_MOVE_RIGHT_DOWN && move <= UM_MOVE_LEFT_DOWN) {
            if (curY >= environment.height - 1)
                return -1;
            curY++;
        } else {
            if (curY <= 0)
                return -1;
            curY--;
        }
    }
    return TILE_FROM_XY(curX, curY);
}

inline int unitMovingOntoTileBesides(int tile, int exceptionUnitNr) {
    int x = X_FROM_TILE(tile);
    int y = Y_FROM_TILE(tile);
    int tileNr;
    int unitNr;
    enum UnitMove um;
    
    for (um=UM_MOVE_UP; um<=UM_MOVE_LEFT_UP; um++) {
        tileNr = getNextTile(x, y, um);
        if (tileNr >= 0) {
            unitNr = environment.layout[tileNr].contains_unit;
            if (unitNr >= 0) {
                if (unit[unitNr].move == (((um-UM_MOVE_UP) + 4) % UM_MOVE_LEFT_UP) + UM_MOVE_UP) {
                    if (unitNr != exceptionUnitNr)
                        return unitNr;
                }
            }
        }
    }
    return -1;
}

int unitMovingOntoTile(int tile) {
    return unitMovingOntoTileBesides(tile, -1);
}

inline int isTileBlockedByOtherUnit(struct Unit *current, int tile) {
    int otherUnit = environment.layout[tile].contains_unit;
    if (otherUnit == -1)
        return 0;
    
    if (otherUnit < -1)
        otherUnit = -(otherUnit + 2);
    if (&unit[otherUnit] == current)
        return 0;
    if (!unit[otherUnit].side != !current->side && unitInfo[current->info].type >= UT_TRACKED && unitInfo[unit[otherUnit].info].type == UT_FOOT)
        return (unitMovingOntoTileBesides(tile, otherUnit) != -1);
    
    return 1;

}

int freeToPlaceUnitOnTile(struct Unit *current, int tile) {
    #ifdef DEBUG_BUILD
    if (tile < 0 || tile >= environment.width * environment.height) errorSI("tile@freeToPlaceUnitOnTile", tile);
    #endif
    if (!freeToMoveUnitViaTile(current, tile))
        return 0;
    
    // check if another unit limits the movement on this tile
    return !isTileBlockedByOtherUnit(current, tile);
}


int nearestEnvironmentTile(int curTile, enum EnvironmentTileGraphics graphicsMin, enum EnvironmentTileGraphics graphicsMax) {
    int maxwh;
    int j, k;
    int ypj_x, ymj_x;
    int xpj_st_envwidth, xmj_lt_zero, ypj_st_envheight, ymj_lt_zero;
    int xpk_st_envwidth, xmk_lt_zero, ypk_st_envheight, ymk_lt_zero;
    
    int x = X_FROM_TILE(curTile);
    int y = Y_FROM_TILE(curTile);
    
    maxwh = max(environment.width, environment.height);
    for (j=1; j<maxwh; j++) {
        
        ypj_x = TILE_FROM_XY(x, y+j);
        ymj_x = TILE_FROM_XY(x, y-j);
        
        if (y+j < environment.height && isEnvironmentTileBetween(ypj_x, graphicsMin, graphicsMax))                          // bottom row middle tile
            return ypj_x;
        if (x+j < environment.width && isEnvironmentTileBetween(TILE_FROM_XY(x+j, y), graphicsMin, graphicsMax))  // right column middle tile
            return TILE_FROM_XY(x+j, y);
        if (x-j >= 0 && isEnvironmentTileBetween(TILE_FROM_XY(x-j, y), graphicsMin, graphicsMax))                  // left column middle tile
            return TILE_FROM_XY(x-j, y);
        if (y-j >= 0 && isEnvironmentTileBetween(ymj_x, graphicsMin, graphicsMax))                                           // top row middle tile
            return ymj_x;
        
        xpj_st_envwidth = x+j < environment.width;
        xmj_lt_zero = x-j >= 0;
        ypj_st_envheight = y+j < environment.height;
        ymj_lt_zero = y-j >= 0;
        
        for (k=1; k<j; k++) {
            xpk_st_envwidth = x+k < environment.width;
            xmk_lt_zero = x-k >= 0;
            ypk_st_envheight = y+k < environment.height;
            ymk_lt_zero = y-k >= 0;
            
            if (ypj_st_envheight) { // bottom row
                if (xpk_st_envwidth && isEnvironmentTileBetween( ypj_x + k, graphicsMin, graphicsMax))
                    return ypj_x + k;
                if (xmk_lt_zero && isEnvironmentTileBetween(ypj_x - k, graphicsMin, graphicsMax))
                    return ypj_x - k;
            }
            if (xpj_st_envwidth) { // right column
                if (ypk_st_envheight && isEnvironmentTileBetween(TILE_FROM_XY(x+j, y+k), graphicsMin, graphicsMax))
                    return TILE_FROM_XY(x+j, y+k);
                if (ymk_lt_zero && isEnvironmentTileBetween(TILE_FROM_XY(x+j, y-k), graphicsMin, graphicsMax))
                    return TILE_FROM_XY(x+j, y-k);
            }
            if (xmj_lt_zero) { // left column
                if (ypk_st_envheight && isEnvironmentTileBetween(TILE_FROM_XY(x-j, y+k), graphicsMin, graphicsMax))
                    return TILE_FROM_XY(x-j, y+k);
                if (ymk_lt_zero && isEnvironmentTileBetween(TILE_FROM_XY(x-j, y-k), graphicsMin, graphicsMax))
                    return TILE_FROM_XY(x-j, y-k);
            }
            if (ymj_lt_zero) { // top row
                if (xpk_st_envwidth && isEnvironmentTileBetween(ymj_x + k, graphicsMin, graphicsMax))
                    return ymj_x + k;
                if (xmk_lt_zero && isEnvironmentTileBetween(ymj_x - k, graphicsMin, graphicsMax))
                    return ymj_x - k;
            }
        }
        
        if (y+j < environment.height) { // bottom row outer tiles (the corner tiles)
            if (x+j < environment.width && isEnvironmentTileBetween(ypj_x + j, graphicsMin, graphicsMax))
                return ypj_x + j;
            if (x-j >= 0 && isEnvironmentTileBetween(ypj_x - j, graphicsMin, graphicsMax))
                return ypj_x - j;
        }
        if (y-j >= 0) { // top row outer tiles (the corner tiles)
            if (x+j < environment.width && isEnvironmentTileBetween(ymj_x + j, graphicsMin, graphicsMax))
                return ymj_x + j;
            if (x-j >= 0 && isEnvironmentTileBetween(ymj_x - j, graphicsMin, graphicsMax))
                return ymj_x - j;
        }
    }
    return -1;
}

int nearestEnvironmentTileUnoccupied(int curTile, enum EnvironmentTileGraphics graphicsMin, enum EnvironmentTileGraphics graphicsMax) {
    int maxwh;
    int j, k;
    int ypj_x, ymj_x;
    int xpj_st_envwidth, xmj_lt_zero, ypj_st_envheight, ymj_lt_zero;
    int xpk_st_envwidth, xmk_lt_zero, ypk_st_envheight, ymk_lt_zero;
    
    int x = X_FROM_TILE(curTile);
    int y = Y_FROM_TILE(curTile);
    
    maxwh = max(environment.width, environment.height);
    for (j=1; j<maxwh; j++) {
        
        ypj_x = TILE_FROM_XY(x, y+j);
        ymj_x = TILE_FROM_XY(x, y-j);
        
        if (y+j < environment.height && environment.layout[ypj_x].contains_unit == -1 && isEnvironmentTileBetween(ypj_x, graphicsMin, graphicsMax))                          // bottom row middle tile
            return ypj_x;
        if (x+j < environment.width && environment.layout[TILE_FROM_XY(x+j, y)].contains_unit == -1 && isEnvironmentTileBetween(environment.width * y + x + j, graphicsMin, graphicsMax))  // right column middle tile
            return TILE_FROM_XY(x+j, y);
        if (x-j >= 0 && environment.layout[TILE_FROM_XY(x-j, y)].contains_unit == -1 && isEnvironmentTileBetween(environment.width * y + x - j, graphicsMin, graphicsMax))                  // left column middle tile
            return TILE_FROM_XY(x-j, y);
        if (y-j >= 0 && environment.layout[ymj_x].contains_unit == -1 && isEnvironmentTileBetween(ymj_x, graphicsMin, graphicsMax))                                           // top row middle tile
            return ymj_x;
        
        xpj_st_envwidth = x+j < environment.width;
        xmj_lt_zero = x-j >= 0;
        ypj_st_envheight = y+j < environment.height;
        ymj_lt_zero = y-j >= 0;
        
        for (k=1; k<j; k++) {
            xpk_st_envwidth = x+k < environment.width;
            xmk_lt_zero = x-k >= 0;
            ypk_st_envheight = y+k < environment.height;
            ymk_lt_zero = y-k >= 0;
            
            if (ypj_st_envheight) { // bottom row
                if (xpk_st_envwidth && environment.layout[ypj_x + k].contains_unit == -1 && isEnvironmentTileBetween(ypj_x + k, graphicsMin, graphicsMax))
                    return ypj_x + k;
                if (xmk_lt_zero && environment.layout[ypj_x - k].contains_unit == -1 && isEnvironmentTileBetween(ypj_x - k, graphicsMin, graphicsMax))
                    return ypj_x - k;
            }
            if (xpj_st_envwidth) { // right column
                if (ypk_st_envheight && environment.layout[TILE_FROM_XY(x+j, y+k)].contains_unit == -1 && isEnvironmentTileBetween(TILE_FROM_XY(x+j, y+k), graphicsMin, graphicsMax))
                    return TILE_FROM_XY(x+j, y+k);
                if (ymk_lt_zero && environment.layout[TILE_FROM_XY(x+j, y-k)].contains_unit == -1 && isEnvironmentTileBetween(TILE_FROM_XY(x+j, y-k), graphicsMin, graphicsMax))
                    return TILE_FROM_XY(x+j, y-k);
            }
            if (xmj_lt_zero) { // left column
                if (ypk_st_envheight && environment.layout[TILE_FROM_XY(x-j, y+k)].contains_unit == -1 && isEnvironmentTileBetween(TILE_FROM_XY(x-j, y+k), graphicsMin, graphicsMax))
                    return TILE_FROM_XY(x-j, y+k);
                if (ymk_lt_zero && environment.layout[TILE_FROM_XY(x-j, y-k)].contains_unit == -1 && isEnvironmentTileBetween(TILE_FROM_XY(x-j, y-k), graphicsMin, graphicsMax))
                    return TILE_FROM_XY(x-j, y-k);
            }
            if (ymj_lt_zero) { // top row
                if (xpk_st_envwidth && environment.layout[ymj_x + k].contains_unit == -1 && isEnvironmentTileBetween(ymj_x + k, graphicsMin, graphicsMax))
                    return ymj_x + k;
                if (xmk_lt_zero && environment.layout[ymj_x - k].contains_unit == -1 && isEnvironmentTileBetween(ymj_x - k, graphicsMin, graphicsMax))
                    return ymj_x - k;
            }
        }
        
        if (y+j < environment.height) { // bottom row outer tiles (the corner tiles)
            if (x+j < environment.width && environment.layout[ypj_x + j].contains_unit == -1 && isEnvironmentTileBetween(ypj_x + j, graphicsMin, graphicsMax))
                return ypj_x + j;
            if (x-j >= 0 && environment.layout[ypj_x - j].contains_unit == -1 && isEnvironmentTileBetween(ypj_x - j, graphicsMin, graphicsMax))
                return ypj_x - j;
        }
        if (y-j >= 0) { // top row outer tiles (the corner tiles)
            if (x+j < environment.width && environment.layout[ymj_x + j].contains_unit == -1 && isEnvironmentTileBetween(ymj_x + j, graphicsMin, graphicsMax))
                return ymj_x + j;
            if (x-j >= 0 && environment.layout[ymj_x - j].contains_unit == -1 && isEnvironmentTileBetween(ymj_x - j, graphicsMin, graphicsMax))
                return ymj_x - j;
        }
    }
    return -1;
}

// WARNING: this function should only be used by placeUnitOnTile
int unitCanLeaveGrid(struct Unit *curUnit, int tile, int radius) {
    int originX = X_FROM_TILE(tile);
    int originY = Y_FROM_TILE(tile);
    int gridDiameter = 1+2*(radius+1);
    int gridTiles = gridDiameter * gridDiameter;
    char grid[gridTiles];
    int curTile;
    int done;
    int i, j;
    
    // 0: have not checked tile
    // 1: ready (and need) to check this tile
    // 2: already checked this tile / unnecessary to check this tile

    #ifdef DEBUG_BUILD
    if (tile < 0 || tile >= environment.width * environment.height) errorSI("tile@unitCanLeaveGrid", tile);
    #endif
    
    for (i=0; i<gridTiles; i++) {
        grid[i] = 0;
        j = (originX - gridDiameter/2) + (i % gridDiameter);
        if (j < 0 || j >= environment.width)
            grid[i] = 2;
        else {
            j = (originY - gridDiameter/2) + (i / gridDiameter);
            if (j < 0 || j >= environment.height)
                grid[i] = 2;
        }
    }
    grid[(gridTiles+1)/2] = 1; // start by checking center tile
    
    do {
        done = 1;
        for (i=gridDiameter; i<gridTiles-gridDiameter; i++) { // check all tiles but the top and bottow row
            if ((i%gridDiameter==0) || (i%gridDiameter==gridDiameter-1)) continue; // and also exclude the outer-left and outer-right columns
            
            if (grid[i] == 1) {
                done = 0;
                curTile = ((originY - gridDiameter/2) + (i / gridDiameter)) * environment.width + ((originX - gridDiameter/2) + (i % gridDiameter));
                
                // up
                if (grid[i - gridDiameter] == 0) {
                    #ifdef DEBUG_BUILD
                    if ((curTile - environment.width) < 0 || (curTile - environment.width) >= environment.width * environment.height) errorSI("curTile-up@unitCanLeaveGrid", curTile);
                    #endif
                    if (freeToMoveUnitViaTile(curUnit, curTile - environment.width)) {
                        if ((i - gridDiameter) / gridDiameter == 0) // found an escape route!
                            return 1;
                        grid[i - gridDiameter] = 1;
                    } else
                        grid[i - gridDiameter] = 2;
                }
                // right-up
                if (grid[(i - gridDiameter) + 1] == 0) {
                    #ifdef DEBUG_BUILD
                    if (((curTile - environment.width) + 1) < 0 || ((curTile - environment.width) + 1) >= environment.width * environment.height) errorSI("curTile-rightup@unitCanLeaveGrid", curTile);
                    #endif
                    if (freeToMoveUnitViaTile(curUnit, (curTile - environment.width) + 1)) {
                        if ((((i - gridDiameter) + 1) / gridDiameter == 0) || (((i - gridDiameter) + 1) % gridDiameter == gridDiameter - 1)) // found an escape route!
                            return 1;
                        grid[((i - gridDiameter) + 1)] = 1;
                    } else
                        grid[((i - gridDiameter) + 1)] = 2;
                }
                // right
                if (grid[i + 1] == 0) {
                    #ifdef DEBUG_BUILD
                    if ((curTile + 1) < 0 || (curTile + 1) >= environment.width * environment.height) errorSI("curTile-right@unitCanLeaveGrid", curTile);
                    #endif
                    if (freeToMoveUnitViaTile(curUnit, curTile + 1)) {
                        if ((i + 1) % gridDiameter == gridDiameter - 1) // found an escape route!
                            return 1;
                        grid[i + 1] = 1;
                    } else
                        grid[i + 1] = 2;
                }
                // right-down
                if (grid[(i + gridDiameter) + 1] == 0) {
                    #ifdef DEBUG_BUILD
                    if (((curTile + environment.width) + 1) < 0 || ((curTile + environment.width) + 1) >= environment.width * environment.height) errorSI("curTile-rightdown@unitCanLeaveGrid", curTile);
                    #endif
                    if (freeToMoveUnitViaTile(curUnit, (curTile + environment.width) + 1)) {
                        if ((((i + gridDiameter) + 1) / gridDiameter == gridDiameter - 1) || (((i + gridDiameter) + 1) % gridDiameter == gridDiameter - 1)) // found an escape route!
                            return 1;
                        grid[((i + gridDiameter) + 1)] = 1;
                    } else
                        grid[((i + gridDiameter) + 1)] = 2;
                }
                // down
                if (grid[i + gridDiameter] == 0) {
                    #ifdef DEBUG_BUILD
                    if ((curTile + environment.width) < 0 || (curTile + environment.width) >= environment.width * environment.height) errorSI("curTile-down@unitCanLeaveGrid", curTile);
                    #endif
                    if (freeToMoveUnitViaTile(curUnit, curTile + environment.width)) {
                        if ((i + gridDiameter) / gridDiameter == gridDiameter - 1) // found an escape route!
                            return 1;
                        grid[i + gridDiameter] = 1;
                    } else
                        grid[i + gridDiameter] = 2;
                }
                // left-down
                if (grid[(i + gridDiameter) - 1] == 0) {
                    #ifdef DEBUG_BUILD
                    if (((curTile + environment.width) - 1) < 0 || ((curTile + environment.width) - 1) >= environment.width * environment.height) errorSI("curTile-leftdown@unitCanLeaveGrid", curTile);
                    #endif
                    if (freeToMoveUnitViaTile(curUnit, (curTile + environment.width) - 1)) {
                        if ((((i + gridDiameter) - 1) / gridDiameter == gridDiameter - 1) || (((i + gridDiameter) - 1) % gridDiameter == 0)) // found an escape route!
                            return 1;
                        grid[((i + gridDiameter) - 1)] = 1;
                    } else
                        grid[((i + gridDiameter) - 1)] = 2;
                }
                // left
                if (grid[i - 1] == 0) {
                    #ifdef DEBUG_BUILD
                    if ((curTile - 1) < 0 || (curTile - 1) >= environment.width * environment.height) errorSI("curTile-left@unitCanLeaveGrid", curTile);
                    #endif
                    if (freeToMoveUnitViaTile(curUnit, curTile - 1)) {
                        if ((i - 1) % gridDiameter == 0) // found an escape route!
                            return 1;
                        grid[i - 1] = 1;
                    } else
                        grid[i - 1] = 2;
                }
                // left-up
                if (grid[(i - gridDiameter) - 1] == 0) {
                    #ifdef DEBUG_BUILD
                    if (((curTile - environment.width) - 1) < 0 || ((curTile - environment.width) - 1) >= environment.width * environment.height) errorSI("curTile-leftup@unitCanLeaveGrid", curTile);
                    #endif
                    if (freeToMoveUnitViaTile(curUnit, (curTile - environment.width) - 1)) {
                        if ((((i - gridDiameter) - 1) / gridDiameter == 0) || (((i - gridDiameter) - 1) % gridDiameter == 0)) // found an escape route!
                            return 1;
                        grid[((i - gridDiameter) - 1)] = 1;
                    } else
                        grid[((i - gridDiameter) - 1)] = 2;
                }
                
                grid[i] = 2;
            }
        }
    } while (!done);
    
    return 0;
}

// WARNING: this function should only be used by dropUnit,
//          because it makes sure a unit can't be placed in/on a structure
//          and that it can actually leave a 7x7 grid centered on this tile
int placeUnitOnTile(int unit_nr, int tile) {
    if (!freeToPlaceUnitOnTile(unit + unit_nr, tile))
        return 0;
    
    if (environment.layout[tile].contains_structure < -1)
        return 0;
    if (environment.layout[tile].contains_structure >= 0 && !structureInfo[structure[environment.layout[tile].contains_structure].info].foundation)
        return 0;
    
    if (!unitCanLeaveGrid(unit + unit_nr, tile, 3))
        return 0;
    
    unit[unit_nr].x = X_FROM_TILE(tile);
    unit[unit_nr].y = Y_FROM_TILE(tile);;
    unit[unit_nr].guard_tile = tile;
    environment.layout[tile].contains_unit = unit_nr;
    return 1;
}

int dropUnit(int unitnr, int x, int y) {
    struct Unit *curUnit;
    struct UnitInfo *curUnitInfo;
    int tile;
    int structureNr;
    int maxwh;
    int ypj_x, ymj_x,
        xpj_st_envwidth, xmj_lt_zero, ypj_st_envheight, ymj_lt_zero,
        xpk_st_envwidth, xmk_lt_zero, ypk_st_envheight, ymk_lt_zero;
    int j, k;
    
    curUnit = unit + unitnr;
    curUnitInfo = unitInfo + curUnit->info;
    
    curUnit->enabled = 1;
    curUnit->x = x;
    curUnit->y = y;
    #ifdef REMOVE_ASTAR_PATHFINDING
    curUnit->hugging_obstacle = 0;
    #endif
    
    tile = TILE_FROM_XY(x,y);
    curUnit->logic_aid = tile;
    structureNr = environment.layout[tile].contains_structure;
    if (structureNr < -1)
        structureNr = environment.layout[tile + structureNr + 1].contains_structure;
    if (structureNr >= 0) {
        if (structure[structureNr].rally_tile < 0)
            curUnit->logic_aid = tile + ((y+1 < environment.height) * environment.width);
        else {
            curUnit->logic_aid = structure[structureNr].rally_tile;
            if (!structureInfo[structure[structureNr].info].can_extract_ore) { // B34, part2 (feeders returned after their mining job to the structure's rally point instead of to the structure itself)
                curUnit->retreat_tile = structure[structureNr].rally_tile;
                curUnit->logic = UL_MOVE_LOCATION; // could have used 'UL_RETREAT' but units will then not perform idle animation at their destination
                curUnit->logic_aid = curUnit->retreat_tile;
            } else { // B34, part1 (feeders should automatically start extracting ore when a rally tile is set - pretending the unit retreated, if it's not a newly created unit, takes care of that)
                curUnit->retreat_tile = tile; // retreat tile set to structure
                if (curUnit->side != FRIENDLY || curUnit->logic != UL_GUARD)
                    curUnit->logic = UL_RETREAT;
            }
        }
    }
    
    if (getGameType() == SINGLEPLAYER) {
        if (curUnit->side != FRIENDLY && curUnitInfo->can_collect_ore) {
            curUnit->retreat_tile = tile;
            curUnit->logic_aid = nearestEnvironmentTile(tile, ORE, OREHILL16);
            curUnit->logic = (curUnit->logic_aid == -1) ? UL_GUARD : UL_RETREAT; // pretending the unit retreated if it can start mining
        }
    }
    
    /* check if the unit can just be dropped on the current tile */
    if (placeUnitOnTile(unitnr, tile)) {
        curUnit->logic = UL_GUARD;
        curUnit->guard_tile = tile;
        curUnit->move = UM_NONE;
        setTileDirtyRadarDirtyBitmap(tile);
        if (curUnit->side == FRIENDLY)
            activateEntityView(x, y, 1, 1, curUnitInfo->view_range);
//        if (structureNr >= 0)
//            curUnit->logic = UL_MOVE_LOCATION;
        return 1;
    }
    
    /* check if the unit can move out of the structure */
    if (environment.layout[tile].contains_unit == -1) {
        if (curUnit->enabled && curUnit->logic == UL_RETREAT && curUnitInfo->can_collect_ore)
            curUnit->logic = UL_MINE_LOCATION;
        else {
            
//            curUnit->logic = UL_MOVE_LOCATION;
//            curUnit->logic_aid = (y+1) * environment.width + x;
        }
        curUnit->move_aid = curUnitInfo->speed;
        if (y < environment.height - 1) {
            if (freeToPlaceUnitOnTile(&unit[unitnr], TILE_FROM_XY(x,y+1)) && unitCanLeaveGrid(curUnit, TILE_FROM_XY(x,y+1), 3)) {
                environment.layout[tile].contains_unit = unitnr;
                curUnit->unit_positioned = DOWN;
                curUnit->turret_positioned = DOWN;
                curUnit->move = UM_MOVE_DOWN;
                reserveTileForUnit(unitnr, curUnit->x, curUnit->y, curUnit->move);
                setTileDirtyRadarDirtyBitmap(tile);
                if (curUnit->side == FRIENDLY)
                    activateEntityView(curUnit->x, curUnit->y, 1, 1, curUnitInfo->view_range);
                return 1;
            }
            if (x > 0 && freeToPlaceUnitOnTile(&unit[unitnr], TILE_FROM_XY(x-1,y+1)) && unitCanLeaveGrid(curUnit, TILE_FROM_XY(x-1,y+1), 3)) {
                environment.layout[tile].contains_unit = unitnr;
//                curUnit->logic_aid -= 1;
                curUnit->unit_positioned = LEFT_DOWN;
                curUnit->turret_positioned = LEFT_DOWN;
                curUnit->move = UM_MOVE_LEFT_DOWN;
                reserveTileForUnit(unitnr, curUnit->x, curUnit->y, curUnit->move);
                setTileDirtyRadarDirtyBitmap(tile);
                if (curUnit->side == FRIENDLY)
                    activateEntityView(curUnit->x, curUnit->y, 1, 1, curUnitInfo->view_range);
                return 1;
            }
            if ((x < environment.width - 1) && freeToPlaceUnitOnTile(&unit[unitnr], TILE_FROM_XY(x+1,y+1)) && unitCanLeaveGrid(curUnit, TILE_FROM_XY(x+1,y+1), 3)) {
                environment.layout[tile].contains_unit = unitnr;
//                curUnit->logic_aid += 1;
                curUnit->unit_positioned = RIGHT_DOWN;
                curUnit->turret_positioned = RIGHT_DOWN;
                curUnit->move = UM_MOVE_RIGHT_DOWN;
                reserveTileForUnit(unitnr, curUnit->x, curUnit->y, curUnit->move);
                setTileDirtyRadarDirtyBitmap(tile);
                if (curUnit->side == FRIENDLY)
                    activateEntityView(curUnit->x, curUnit->y, 1, 1, curUnitInfo->view_range);
                return 1;
            }
        }
    }
    
    /* search for a free tile to just drop the unit */
    if (curUnit->enabled && curUnit->logic == UL_RETREAT && curUnitInfo->can_collect_ore)
        curUnit->logic = UL_MINE_LOCATION;
    else if (curUnit->logic != UL_MINE_LOCATION && !(structureNr >= 0 && structure[structureNr].rally_tile >= 0))
        curUnit->logic = UL_GUARD; // in other cases, the logic of the unit is already assigned within the block which also assigns a value to the variable structureNr
    
    curUnit->move = UM_NONE;
    maxwh = max(environment.width, environment.height);
    for (j=1; j<maxwh; j++) {
        
        ypj_x = TILE_FROM_XY(x, y+j);
        ymj_x = TILE_FROM_XY(x, y-j);
        
        if ((y+j < environment.height && placeUnitOnTile(unitnr, ypj_x)) ||                // bottom row middle tile
            (x+j < environment.width && placeUnitOnTile(unitnr, TILE_FROM_XY(x+j, y))) ||  // right column middle tile
            (x-j >= 0 && placeUnitOnTile(unitnr, TILE_FROM_XY(x-j, y))) ||                  // left column middle tile
            (y-j >= 0 && placeUnitOnTile(unitnr, ymj_x))) {                                  // top row middle tile
            curUnit->unit_positioned = positionedToFaceXY(x, y, curUnit->x, curUnit->y);
            curUnit->turret_positioned = curUnit->unit_positioned;
            setTileDirtyRadarDirtyBitmap(tile);
            if (curUnit->side == FRIENDLY)
                activateEntityView(curUnit->x, curUnit->y, 1, 1, curUnitInfo->view_range);
            return 1;
        }
        
        xpj_st_envwidth = x+j < environment.width;
        xmj_lt_zero = x-j >= 0;
        ypj_st_envheight = y+j < environment.height;
        ymj_lt_zero = y-j >= 0;
        
        for (k=1; k<j; k++) {
            xpk_st_envwidth = x+k < environment.width;
            xmk_lt_zero = x-k >= 0;
            ypk_st_envheight = y+k < environment.height;
            ymk_lt_zero = y-k >= 0;
            
            if ((ypj_st_envheight && // bottom row
                 ((xpk_st_envwidth && placeUnitOnTile(unitnr, ypj_x + k)) ||
                  (xmk_lt_zero && placeUnitOnTile(unitnr, ypj_x - k)))) ||
                (xpj_st_envwidth && // right column
                 ((ypk_st_envheight && placeUnitOnTile(unitnr, TILE_FROM_XY(x+j, y+k))) ||
                  (ymk_lt_zero && placeUnitOnTile(unitnr, TILE_FROM_XY(x+j, y-k))))) ||
                (xmj_lt_zero && // left column
                 ((ypk_st_envheight && placeUnitOnTile(unitnr, TILE_FROM_XY(x-j, y+k))) ||
                  (ymk_lt_zero && placeUnitOnTile(unitnr, TILE_FROM_XY(x-j, y-k))))) ||
                (ymj_lt_zero && // top row
                 ((xpk_st_envwidth && placeUnitOnTile(unitnr, ymj_x + k)) ||
                  (xmk_lt_zero && placeUnitOnTile(unitnr, ymj_x - k))))) {
                curUnit->unit_positioned = positionedToFaceXY(x, y, curUnit->x, curUnit->y);
                curUnit->turret_positioned = curUnit->unit_positioned;
                setTileDirtyRadarDirtyBitmap(tile);
                if (curUnit->side == FRIENDLY)
                    activateEntityView(curUnit->x, curUnit->y, 1, 1, curUnitInfo->view_range);
                return 1;
            }
        }
        
        if ((y+j < environment.height && // bottom row outer tiles (the corner tiles)
             ((x+j < environment.width && placeUnitOnTile(unitnr, ypj_x + j)) ||
              (x-j >= 0 && placeUnitOnTile(unitnr, ypj_x - j)))) ||
            (y-j >= 0 && // top row outer tiles (the corner tiles)
             ((x+j < environment.width && placeUnitOnTile(unitnr, ymj_x + j)) ||
              (x-j >= 0 && placeUnitOnTile(unitnr, ymj_x - j))))) {
            curUnit->unit_positioned = positionedToFaceXY(x, y, curUnit->x, curUnit->y);
            curUnit->turret_positioned = curUnit->unit_positioned;
            setTileDirtyRadarDirtyBitmap(tile);
            if (curUnit->side == FRIENDLY)
                activateEntityView(curUnit->x, curUnit->y, 1, 1, curUnitInfo->view_range);
            return 1;
        }
    }
    
    return 0;
}

int addUnit(enum Side side, int x, int y, int info, int forced) {
    int i, j;
    struct Unit *curUnit = unit;
    struct UnitInfo *curUnitInfo = unitInfo + info;
    struct StructureInfo *curStructureInfo;
    struct Structure *curStructure;
    int structureId;
    
    for (i=0; i<MAX_UNITS_ON_MAP; i++, curUnit++) {
        if (!curUnit->enabled && !(curUnit->group & AWAITING_REPLACEMENT)) {
            curUnit->group = 0;
            curUnit->selected = 0;
            curUnit->info = info;
            curUnit->side = side;
            curUnit->x = x;
            curUnit->y = y;
            curUnit->retreat_tile = TILE_FROM_XY(x,y);
            curUnit->guard_tile   = TILE_FROM_XY(x,y);
            curUnit->ore = 5;
            curUnit->armour = curUnitInfo->max_armour;
            curUnit->unit_positioned = DOWN;
            curUnit->turret_positioned = DOWN;
            curUnit->logic_aid = -1;
            #ifdef REMOVE_ASTAR_PATHFINDING
            curUnit->hugging_obstacle = 0;
            #endif
            curUnit->smoke_time = 0;
            curUnit->multiplayer_id = unitMultiplayerIdCounter++;
            
            curUnit->reload_time = curUnitInfo->reload_time;
            curUnit->misfire_time = 0;
            
            // if (curUnitInfo->double_shot && (curUnit->reload_time - (SHOOT_ANIMATION_DURATION_FAKED + 1) >= DOUBLE_SHOT_AT)) // try to make sure it doesn't look like it's shooting from the get-go
                curUnit->reload_time = max((curUnitInfo->double_shot?DOUBLE_SHOT_AT:0), curUnitInfo->reload_time - (SHOOT_ANIMATION_DURATION_FAKED + 1));
            
            if (curUnitInfo->can_collect_ore) {
                // if structure it's currently located at is not something that can extract ore,
                // we need to search for a better retreat_tile for this unit (primary ore extractor building's release tile)
                j = environment.layout[curUnit->retreat_tile].contains_structure;
                if (j < -1)
                    j = environment.layout[curUnit->retreat_tile + (j + 1)].contains_structure;
                if (j == -1 || !structureInfo[structure[j].info].can_extract_ore) {
                    for (j=0; j<MAX_STRUCTURES_ON_MAP; j++) {
                        if (structure[j].enabled && structure[j].primary && structure[j].side == FRIENDLY && structureInfo[structure[j].info].can_extract_ore) {
                            curUnit->retreat_tile = TILE_FROM_XY(structure[j].x + structureInfo[structure[j].info].release_tile, structure[j].y + structureInfo[structure[j].info].height - 1);
                            break;
                        }
                    }
                    if (j == MAX_STRUCTURES_ON_MAP)
                        curUnit->retreat_tile = -1;
                }
            }
            
            curUnit->logic = UL_GUARD; // used also to check in dropUnit (if added inside a building) if this concerns a new unit 
            if (forced) {
                curUnit->guard_tile = TILE_FROM_XY(x,y);
                curUnit->move = UM_NONE;
                environment.layout[curUnit->retreat_tile].contains_unit = i;
                curUnit->enabled = 1;
                if (curUnit->side == FRIENDLY)
                    activateEntityView(x, y, 1, 1, curUnitInfo->view_range);
                setUnitCount(curUnit->side, getUnitCount(curUnit->side) + 1);
                return i;
            } else if (dropUnit(i, x, y)) {
                // drop unit also performs activateEntityView
                curUnit->enabled = 1;
                structureId = environment.layout[curUnit->retreat_tile].contains_structure;
                if (structureId < -1)
                    structureId = environment.layout[curUnit->retreat_tile + (structureId + 1)].contains_structure;
                if (structureId >= 0) {
                    curStructure = structure + structureId;
                    curStructureInfo = structureInfo + structure->info;
                    if (!structure->logic_aid && curStructureInfo->active_ani > 0 && !curStructureInfo->barrier && 
                        curStructureInfo->projectile_info < 0 && !curStructureInfo->can_extract_ore && !curStructureInfo->can_repair_unit)
                        curStructure->logic_aid = 1;
//                    if (curStructureInfo->can_extract_ore && structure->rally_tile != ((curStructure->y + curStructureInfo->height - (1 * (curStructure->y + curStructureInfo->height >= environment.height))) * environment.width + curStructure->x + curStructureInfo->release_tile)) {
//                        curUnit->logic = UL_MINE_LOCATION;
//                        curUnit->logic_aid = structure[structureId].rally_tile;
//                    }
                }
                setUnitCount(curUnit->side, getUnitCount(curUnit->side) + 1);
                return i;
            }
            return -1;
        }
    }
    return -1;
}


int unitTouchableOnTileCoordinates(int unitnr, int xtouch, int ytouch) {
    struct Unit *curUnit;
    struct UnitInfo *curUnitInfo;
    int x, y; // there are the top-left coordinate values of the unit's 16x16 center, measured in pixels

    curUnit = &unit[unitnr];
    curUnitInfo = &unitInfo[curUnit->info];
    x = 0;
    y = 0;
    alterXYAccordingToUnitMovement(curUnit->move, curUnit->move_aid, curUnitInfo->speed, &x, &y);
    return (xtouch >= x && xtouch < x+16 && ytouch >= y && ytouch < y+16);
}

inline void drawUnitWithShift(int unitnr, int x_add, int y_add) {
    int graphics, hflip, vflip;
    struct Unit *curUnit;
    struct UnitInfo *curUnitInfo;
    int x, y, x_cur, y_cur;
    int aid;
    int graphicsSizePositionAdjustment;

    curUnit = &unit[unitnr];
    curUnitInfo = &unitInfo[curUnit->info];
    graphics = curUnitInfo->graphics_offset;
    graphicsSizePositionAdjustment = (16 * (1 << ((int) curUnitInfo->graphics_size - 1)) - 16) / 2;
    x = 16* (curUnit->x - getViewCurrentX());
    y = 16*((curUnit->y - getViewCurrentY())+ACTION_BAR_SIZE);
    alterXYAccordingToUnitMovement(curUnit->move, curUnit->move_aid, curUnitInfo->speed, &x, &y);
    x += x_add;
    y += y_add;
    if (curUnit->move >= UM_MOVE_UP && curUnit->move <= UM_MOVE_LEFT_UP) {
        if (curUnit->logic != UL_MINE_LOCATION || curUnit->logic_aid != TILE_FROM_XY(curUnit->x, curUnit->y)) {
            aid = (curUnitInfo->move_ani > 0) * ((curUnitInfo->speed - curUnit->move_aid) * (curUnitInfo->move_ani + 1)) / curUnitInfo->speed;
            if (aid & 1)
                graphics +=  (1 + curUnitInfo->shoot_ani + aid/2) * (curUnitInfo->rotation_ani * 4 + 1) * math_power(4, curUnitInfo->graphics_size - 1);
        }
        
    } else if ((curUnit->reload_time >= curUnitInfo->reload_time - SHOOT_ANIMATION_DURATION_FAKED && curUnit->reload_time > DOUBLE_SHOT_AT) || (curUnitInfo->double_shot && curUnit->reload_time < DOUBLE_SHOT_AT && curUnit->reload_time > max(DOUBLE_SHOT_AT-SHOOT_ANIMATION_DURATION_FAKED,0))) { //if ((curUnit->reload_time >= curUnitInfo->reload_time - SHOOT_ANIMATION_DURATION_FAKED && curUnit->reload_time > DOUBLE_SHOT_AT)  || (curUnitInfo->double_shot && curUnit->reload_time < DOUBLE_SHOT_AT && curUnit->reload_time >= DOUBLE_SHOT_AT - SHOOT_ANIMATION_DURATION_FAKED)) {
        if (!curUnitInfo->can_collect_ore)  // (because DOUBLE_SHOT_AT = SHOOT_ANIMATION_DURATION_FAKED)
//  } else if ((curUnitInfo->double_shot && curUnit->reload_time >= curUnitInfo->reload_time - SHOOT_ANIMATION_DURATION)  || (curUnit->reload_time < DOUBLE_SHOT_AT && curUnit->reload_time >= DOUBLE_SHOT_AT - SHOOT_ANIMATION_DURATION))
        graphics += (curUnitInfo->shoot_ani > 0) * (curUnitInfo->rotation_ani * 4 + 1) * math_power(4, curUnitInfo->graphics_size - 1);
    }
    if (curUnit->logic == UL_MINE_LOCATION && curUnit->logic_aid == TILE_FROM_XY(curUnit->x, curUnit->y)) {
        if (curUnitInfo->shoot_ani > 0) {
            aid = ((environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].ore_level % (MAX_ENVIRONMENT_ORE_LEVEL/2)) / ((MAX_ENVIRONMENT_ORE_LEVEL/2) / (5*curUnitInfo->shoot_ani))) % curUnitInfo->shoot_ani;
            if (aid > 0)
                graphics += aid * (curUnitInfo->rotation_ani * 4 + 1) * math_power(4, curUnitInfo->graphics_size - 1);
        }
    }
    
    if (curUnit->selected) {
        if (curUnitInfo->can_collect_ore && curUnit->side == FRIENDLY)
            #ifdef USE_REDUCED_UNIT_COLLECTED_GFX
            setSpritePlayScreen(y+2, ATTR0_SQUARE,
                                x, SPRITE_SIZE_16, 0, 0,
                                0, PS_SPRITES_PAL_GUI, unit_collected_graphics_offset + min(7, ((7 * curUnit->ore) / ((int)MAX_ORED_BY_UNIT))));
            #else
            setSpritePlayScreen(y+2, ATTR0_SQUARE,
                                x, SPRITE_SIZE_16, 0, 0,
                                0, PS_SPRITES_PAL_GUI, unit_collected_graphics_offset + min(14, ((14 * curUnit->ore) / ((int)MAX_ORED_BY_UNIT))));
            #endif
        #ifdef USE_REDUCED_UNIT_SELECTED_GFX
        setSpritePlayScreen(y, ATTR0_SQUARE,
                            x, SPRITE_SIZE_16, 0, 0,
                            0, PS_SPRITES_PAL_GUI, unit_selected_graphics_offset + ((6 * curUnit->armour) / curUnitInfo->max_armour));
        #else
        setSpritePlayScreen(y, ATTR0_SQUARE,
                            x, SPRITE_SIZE_16, 0, 0,
                            0, PS_SPRITES_PAL_GUI, unit_selected_graphics_offset + ((13 * curUnit->armour) / curUnitInfo->max_armour));
        #endif
    }
    
    if (curUnit->smoke_time > 0) {
        if (curUnitInfo->type == UT_FOOT)
            setSpritePlayScreen(y, ATTR0_SQUARE,
                                x, SPRITE_SIZE_16, 0, 0,
                                1, PS_SPRITES_PAL_SMOKE, unit_bleed_graphics_offset + ((curUnit->smoke_time / UNIT_SINGLE_SMOKE_DURATION) % 3));
        else
            setSpritePlayScreen(y - 11, ATTR0_SQUARE,
                                x, SPRITE_SIZE_16, 0, 0,
                                1, PS_SPRITES_PAL_SMOKE, unit_smoke_graphics_offset + ((curUnit->smoke_time / UNIT_SINGLE_SMOKE_DURATION) % 3));
    }
    
    if (curUnitInfo->flag_type > 0) {
        setSpritePlayScreen(y, ATTR0_SQUARE,
                            x, SPRITE_SIZE_16, 0, 0,
                            1, PS_SPRITES_PAL_GUI, base_unit_flag + curUnitInfo->flag_type - 1);
    }
    
    hflip = 0;
    vflip = 0;
    
    if (curUnitInfo->can_rotate_turret) { // graphics: right, right-up, up (for both turret and base)
        if (curUnitInfo->rotation_ani) {
            // turret
            switch (curUnit->turret_positioned) {
                case DOWN:
                    vflip = 1; // fallthrough
                case UP:
                    graphics += math_power(4, (int) curUnitInfo->graphics_size - 1) * 5;
                    break;
                case LEFT_DOWN:
                    vflip = 1; // fallthrough
                case LEFT_UP:
                    hflip = 1;
                    graphics += math_power(4, (int) curUnitInfo->graphics_size - 1) * 4;
                    break;
                case RIGHT_DOWN:
                    vflip = 1; // fallthrough
                case RIGHT_UP:
                    graphics += math_power(4, (int) curUnitInfo->graphics_size - 1) * 4;
                    break;
                case LEFT:
                    hflip = 1; // fallthrough
                case RIGHT:
                    graphics += math_power(4, (int) curUnitInfo->graphics_size - 1) * 3;
                    break;
            }
        }
        y_cur = y - math_power(2, (int) curUnitInfo->graphics_size);
        x_cur = x;
        if (curUnitInfo->rotation_ani) {
            if (curUnitInfo->graphics_size > 0) {
                switch (curUnit->turret_positioned) {
                    case LEFT_DOWN:
                    case LEFT_UP:
                        x_cur = x - 2*math_power(2, ((int) curUnitInfo->graphics_size) - 1);
                        break;
                    case RIGHT_DOWN:
                    case RIGHT_UP:
                        x_cur = x + 2*math_power(2, ((int) curUnitInfo->graphics_size) - 1);
                        break;
                    case LEFT:
                        x_cur = x - math_power(2, ((int) curUnitInfo->graphics_size) - 1);
                        break;
                    case RIGHT:
                        x_cur = x + math_power(2, ((int) curUnitInfo->graphics_size) - 1);
                        break;
                    default:
                        break;
                }
            }
        }
        setSpritePlayScreen(y_cur-graphicsSizePositionAdjustment, ATTR0_SQUARE,
                            x_cur-graphicsSizePositionAdjustment, curUnitInfo->graphics_size, hflip&1, vflip&1,
                            2, PS_SPRITES_PAL_FACTIONS + getFaction(curUnit->side), graphics);
        
        // base
        graphics = curUnitInfo->graphics_offset;
        hflip = 0;
        vflip = 0;
        if (curUnitInfo->rotation_ani) {
            switch (curUnit->unit_positioned) {
                case UP:
                case DOWN:
                    graphics += math_power(4, (int) curUnitInfo->graphics_size - 1) * 2;
                    break;
                case LEFT_UP:
                case RIGHT_DOWN:
                    hflip = 1; // fallthrough
                case RIGHT_UP:
                case LEFT_DOWN:
                    graphics += math_power(4, (int) curUnitInfo->graphics_size - 1);
                    break;
                default: // RIGHT and LEFT need no adjusting
                    break;
            }
        }
        setSpritePlayScreen(y-graphicsSizePositionAdjustment, ATTR0_SQUARE,
                            x-graphicsSizePositionAdjustment, curUnitInfo->graphics_size, hflip&1, vflip&1,
                            2, PS_SPRITES_PAL_FACTIONS + getFaction(curUnit->side), graphics);
    } else { // graphics: right, right-up, up, down, right-down
        if (curUnitInfo->rotation_ani) {
            switch (curUnit->unit_positioned) {
                case LEFT_UP:
                    hflip = 1; // fallthrough
                case RIGHT_UP:
                    graphics += math_power(4, (int) curUnitInfo->graphics_size - 1);
                    break;
                case UP:
                    graphics += math_power(4, (int) curUnitInfo->graphics_size - 1) * 2;
                    break;
                case DOWN:
                    graphics += math_power(4, (int) curUnitInfo->graphics_size - 1) * 3;
                    break;
                case LEFT_DOWN:
                    hflip = 1; // fallthrough
                case RIGHT_DOWN:
                    graphics += math_power(4, (int) curUnitInfo->graphics_size - 1) * 4;
                    break;
                case LEFT:
                    hflip = 1;
                    break;
                default: // RIGHT needs no adjusting
                    break;
            }
        }
        setSpritePlayScreen(y-graphicsSizePositionAdjustment, ATTR0_SQUARE,
                            x-graphicsSizePositionAdjustment, curUnitInfo->graphics_size, hflip&1, vflip&1,
                            2, PS_SPRITES_PAL_FACTIONS + getFaction(curUnit->side), graphics);
    }
}

inline void drawUnit(int unitnr) {
    drawUnitWithShift(unitnr, 0,0);
}

void drawUnits() {
    int i, j, current;
    int mapTile = TILE_FROM_XY(getViewCurrentX(), getViewCurrentY() + HORIZONTAL_HEIGHT - 1); // tile to start drawing (from bottom to top)
    
    startProfilingFunction("drawUnits");
    
    // display per unit in view:
        //   unit-selected
        //   smoke
        //   unit-turret
        //   unit
    
    
    
    for (i=HORIZONTAL_HEIGHT; i--;) {
        for (j=0; j<HORIZONTAL_WIDTH; j++) {
            if (environment.layout[mapTile].status != UNDISCOVERED) {
                current = environment.layout[mapTile].contains_unit;
                if (current >= 0) // a unit exists here
                    drawUnit(current);
            }
            mapTile++;
        }
        mapTile -= (environment.width + HORIZONTAL_WIDTH);
    }
    
    stopProfilingFunction();
}

void loadUnitsGraphicsBG(int baseBg, int *offsetBg) {
}

void loadUnitsGraphicsSprites(int *offsetSp) {
    int i, j;
    char oneline[256];

    for (i=0; i<MAX_DIFFERENT_UNITS && unitInfo[i].enabled; i++) {
        unitInfo[i].graphics_offset = (*offsetSp)/(16*16);
        *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), unitInfo[i].name, FS_UNITS_GRAPHICS);
        
        memcpy(oneline, unitInfo[i].name, MAX_UNIT_NAME_LENGTH);
        if (unitInfo[i].can_rotate_turret) {
            sprintf(oneline + strlen(unitInfo[i].name), "_turret");
            *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), oneline, FS_UNITS_GRAPHICS);
        }
        if (unitInfo[i].shoot_ani) {
            if (unitInfo[i].can_collect_ore) {
                for (j=0; j<unitInfo[i].shoot_ani-1; j++) {
                    sprintf(oneline + strlen(unitInfo[i].name), "_mine%i", j+1);
                    *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), oneline, FS_UNITS_GRAPHICS);
                }
            } else {
                sprintf(oneline + strlen(unitInfo[i].name), "_shoot");
                *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), oneline, FS_UNITS_GRAPHICS);
            }
        }
        for (j=0; j<unitInfo[i].move_ani; j+=2) {
            sprintf(oneline + strlen(unitInfo[i].name), "_move%i", j+1);
            *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), oneline, FS_UNITS_GRAPHICS);
        }
    }
    
    memcpy(oneline, "unit_selected", 13);
    oneline[14] = '\0';
    oneline[15] = '\0';
    unit_selected_graphics_offset = (*offsetSp)/(16*16);
    #ifdef USE_REDUCED_UNIT_SELECTED_GFX
    for (i=2; i<=14; i+=2) {
    #else
    for (i=1; i<=14; i++) {
    #endif
        if (i < 10)
            oneline[13] = 48+i;
        else {
            oneline[13] = 48+(i/10);
            oneline[14] = 48+(i%10);
        }
        *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), oneline, FS_GUI_GRAPHICS);
    }
    
    memcpy(oneline, "unit_collected", 14);
    oneline[15] = '\0';
    oneline[16] = '\0';
    unit_collected_graphics_offset = (*offsetSp)/(16*16);
    #ifdef USE_REDUCED_UNIT_COLLECTED_GFX
    for (i=0; i<=14; i+=2) {
    #else
    for (i=0; i<=14; i++) {
    #endif
        if (i < 10)
            oneline[14] = 48+i;
        else {
            oneline[14] = 48+(i/10);
            oneline[15] = 48+(i%10);
        }
        *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), oneline, FS_GUI_GRAPHICS);
    }
    
    unit_bleed_graphics_offset = (*offsetSp)/(16*16);
    *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), "Bleed", FS_SMOKE_GRAPHICS);
    
    base_unit_flag = (*offsetSp)/(16*16);
    *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), "Unit1", FS_FLAG_GRAPHICS);
    *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), "Unit2", FS_FLAG_GRAPHICS);
    *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), "Unit3", FS_FLAG_GRAPHICS);
    *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), "Unit4", FS_FLAG_GRAPHICS);
    *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), "Unit5", FS_FLAG_GRAPHICS);
    *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), "Unit6", FS_FLAG_GRAPHICS);
    *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), "Unit7", FS_FLAG_GRAPHICS);
    *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), "Unit8", FS_FLAG_GRAPHICS);
    *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), "Unit9", FS_FLAG_GRAPHICS);
}


int recalculateDestination(int unit_nr) {
    struct Unit *curUnit = unit + unit_nr;
    struct UnitInfo *curUnitInfo = unitInfo + curUnit->info;
    int tile;
    struct EnvironmentLayout *envLayout;
    int maxwh;
    int ypj_x, ymj_x,
        xpj_st_envwidth, xmj_lt_zero, ypj_st_envheight, ymj_lt_zero,
        xpk_st_envwidth, xmk_lt_zero, ypk_st_envheight, ymk_lt_zero;
    int j, k;
    int x, y;
    
    if (curUnit->logic == UL_ATTACK_UNIT || curUnit->logic == UL_ATTACK_LOCATION || curUnit->logic == UL_ATTACK_AREA)
        return 0;
    
    if (curUnit->logic == UL_RETREAT && curUnitInfo->can_collect_ore)
        return 0;
    
    tile = curUnit->logic_aid;
    envLayout = environment.layout + tile;
    
// MULTIPLAYER ?
    if (curUnit->side == FRIENDLY && envLayout->status == UNDISCOVERED)
        return 0;
    
    if (freeToPlaceUnitOnTile(curUnit, tile))
        return 0;
    
    x = X_FROM_TILE(tile);
    y = Y_FROM_TILE(tile);
    maxwh = max(environment.width, environment.height);
    for (j=1; j<maxwh; j++) {
        
        ypj_x = TILE_FROM_XY(x, y+j);
        ymj_x = TILE_FROM_XY(x, y-j);
        
        if (y+j < environment.height && freeToPlaceUnitOnTile(curUnit, ypj_x)) {                // bottom row middle tile
            curUnit->logic_aid = ypj_x;
            return 1;
        }
        if (x+j < environment.width && freeToPlaceUnitOnTile(curUnit, TILE_FROM_XY(x+j, y))) {  // right column middle tile
            curUnit->logic_aid = TILE_FROM_XY(x+j, y);
            return 1;
        }
        if (x-j >= 0 && freeToPlaceUnitOnTile(curUnit, TILE_FROM_XY(x-j, y))) {                  // left column middle tile
            curUnit->logic_aid = TILE_FROM_XY(x-j, y);
            return 1;
        }
        if (y-j >= 0 && freeToPlaceUnitOnTile(curUnit, ymj_x)) {                                  // top row middle tile
            curUnit->logic_aid = ymj_x;
            return 1;
        }
        
        xpj_st_envwidth = x+j < environment.width;
        xmj_lt_zero = x-j >= 0;
        ypj_st_envheight = y+j < environment.height;
        ymj_lt_zero = y-j >= 0;
        
        for (k=1; k<j; k++) {
            xpk_st_envwidth = x+k < environment.width;
            xmk_lt_zero = x-k >= 0;
            ypk_st_envheight = y+k < environment.height;
            ymk_lt_zero = y-k >= 0;
            
            if (ypj_st_envheight) { // bottom row
                if (xpk_st_envwidth && freeToPlaceUnitOnTile(curUnit, ypj_x + k)) {
                    curUnit->logic_aid = ypj_x + k;
                    return 1;
                }
                if (xmk_lt_zero && freeToPlaceUnitOnTile(curUnit, ypj_x - k)) {
                    curUnit->logic_aid = ypj_x - k;
                    return 1;
                }
            }
            if (xpj_st_envwidth) { // right column
                if (ypk_st_envheight && freeToPlaceUnitOnTile(curUnit, TILE_FROM_XY(x+j, y+k))) {
                    curUnit->logic_aid = TILE_FROM_XY(x+j, y+k);
                    return 1;
                }
                if (ymk_lt_zero && freeToPlaceUnitOnTile(curUnit, TILE_FROM_XY(x+j, y-k))) {
                    curUnit->logic_aid = TILE_FROM_XY(x+j, y-k);
                    return 1;
                }
            }
            if (xmj_lt_zero) { // left column
                if (ypk_st_envheight && freeToPlaceUnitOnTile(curUnit, TILE_FROM_XY(x-j, y+k))) {
                    curUnit->logic_aid = TILE_FROM_XY(x-j, y+k);
                    return 1;
                }
                if (ymk_lt_zero && freeToPlaceUnitOnTile(curUnit, TILE_FROM_XY(x-j, y-k))) {
                    curUnit->logic_aid = TILE_FROM_XY(x-j, y-k);
                    return 1;
                }
            }
            if (ymj_lt_zero) { // top row
                if (xpk_st_envwidth && freeToPlaceUnitOnTile(curUnit, ymj_x + k)) {
                    curUnit->logic_aid = ymj_x + k;
                    return 1;
                }
                if (xmk_lt_zero && freeToPlaceUnitOnTile(curUnit, ymj_x - k)) {
                    curUnit->logic_aid = ymj_x - k;
                    return 1;
                }
            }
        }
        
        if (y+j < environment.height) { // bottom row outer tiles (the corner tiles)
            if (x+j < environment.width && freeToPlaceUnitOnTile(curUnit, ypj_x + j)) {
                curUnit->logic_aid = ypj_x + j;
                return 1;
            }
            if (x-j >= 0 && freeToPlaceUnitOnTile(curUnit, ypj_x - j)) {
                curUnit->logic_aid = ypj_x - j;
                return 1;
            }
        }
        if (y-j >= 0) { // top row outer tiles (the corner tiles)
            if (x+j < environment.width && freeToPlaceUnitOnTile(curUnit, ymj_x + j)) {
                curUnit->logic_aid = ymj_x + j;
                return 1;
            }
            if (x-j >= 0 && freeToPlaceUnitOnTile(curUnit, ymj_x - j)) {
                curUnit->logic_aid = ymj_x - j;
                return 1;
            }
        }
    }
    
    
// TODO
    return 0;
}

enum UnitMove getBestIgnorantMove(int curX, int curY, int newX, int newY) {
    if (curX == newX && curY == newY)
        return UM_NONE;
    
    if (max(curX, newX) - min(curX, newX) > max(curY, newY) - min(curY, newY)) {
        if (curX == min(curX, newX))
            return UM_MOVE_RIGHT;
//      else
            return UM_MOVE_LEFT;
    }
    
    if (max(curX, newX) - min(curX, newX) < max(curY, newY) - min(curY, newY)) {
        if (curY == min(curY, newY))
            return UM_MOVE_DOWN;
//      else
            return UM_MOVE_UP;
    }
    
//  else {
        if (curX == min(curX, newX)) {
            if (curY == min(curY, newY))
                return UM_MOVE_RIGHT_DOWN;
//          else
                return UM_MOVE_RIGHT_UP;
        }
        
//      else {
            if (curY == min(curY, newY))
                return UM_MOVE_LEFT_DOWN;
//          else
                return UM_MOVE_LEFT_UP;
//      }
//  }
}

#ifndef REMOVE_ASTAR_PATHFINDING
inline enum UnitMove findBestPath(int unit_nr, int curX, int curY, int newX, int newY) {
    #ifdef DEBUG_BUILD
    if (curX < 0 || curY < 0 || TILE_FROM_XY(curX, curY) >= environment.width * environment.height) errorSI("dest@findBestPath.cur", TILE_FROM_XY(curX, curY));
    if (newX < 0 || newY < 0 || TILE_FROM_XY(newX, newY) >= environment.width * environment.height) errorSI("dest@findBestPath.new", TILE_FROM_XY(newX, newY));
    #endif
    enum UnitMove result = getPathfindingPath(unit_nr, curX, curY, newX, newY);
    
    if ((result & UM_MASK) == UM_NONE)
        return result;
    if (freeToPlaceUnitOnTile(unit + unit_nr, getNextTile(curX, curY, result)))
        return result;
    
    removePathfindingPath(unit_nr);
    return getPathfindingPath(unit_nr, curX, curY, newX, newY); // will requeue path search
}
inline enum UnitMove findBestAttackPath(int unit_nr, int curX, int curY, int shoot_range, int newX, int newY) {
    #ifdef DEBUG_BUILD
    if (curX < 0 || curY < 0 || TILE_FROM_XY(curX, curY) >= environment.width * environment.height) errorSI("dest@findBestAttackPath.cur", TILE_FROM_XY(curX, curY));
    if (newX < 0 || newY < 0 || TILE_FROM_XY(newX, newY) >= environment.width * environment.height) errorSI("dest@findBestAttackPath.new", TILE_FROM_XY(newX, newY));
    #endif
    enum UnitMove result = getPathfindingAttackPath(unit_nr, curX, curY, shoot_range, newX, newY);
    
    if ((result & UM_MASK) == UM_NONE)
        return result;
    if (freeToPlaceUnitOnTile(unit + unit_nr, getNextTile(curX, curY, result)))
        return result;
    
    removePathfindingPath(unit_nr);
    return getPathfindingAttackPath(unit_nr, curX, curY, shoot_range, newX, newY); // will requeue path search
}
#else
enum UnitMove findBestPath(int unit_nr, int curX, int curY, int newX, int newY) {
    int ignorantMove, nextMove;
    int i, j;
    struct Unit *curUnit = unit + unit_nr;

    ignorantMove = getBestIgnorantMove(curX, curY, newX, newY);
    if (ignorantMove == UM_NONE) {
        curUnit->hugging_obstacle = 0;
        return UM_NONE;
    }
    
    if (curUnit->hugging_obstacle == 0) {
        if (freeToPlaceUnitOnTile(curUnit, getNextTile(curX, curY, ignorantMove)))
            return ignorantMove;
        
        /* we seemed to have bumped into an obstacle; do we need to change the destination? */
        /* first recalculate if the destination should be changed though */
        if (recalculateDestination(unit_nr)) {
            ignorantMove = getBestIgnorantMove(curX, curY, newX, newY);
            if (ignorantMove == UM_NONE) {
                curUnit->hugging_obstacle = 0;
                return UM_NONE;
            }
            if (freeToPlaceUnitOnTile(curUnit, getNextTile(curX, curY, ignorantMove)))
                return ignorantMove;
        }
        
        /* the ignorant move can't be performed; let's decide whether we go left or right around it */
        for (i=0; i<4; i++) {
            j = ignorantMove - i;
            if (j < UM_MOVE_UP)
                j += 8;
            if (freeToPlaceUnitOnTile(curUnit, getNextTile(curX, curY, j))) {
                if (curUnit->unit_positioned == j-1 || (unitInfo[curUnit->info].can_rotate_turret && (curUnit->unit_positioned + 4) % 8 == j-1))
                    curUnit->hugging_obstacle = -1;
                return j;
            }
            j = ignorantMove + i + 1;
            if (j > UM_MOVE_LEFT_UP)
                j -= 8;
            if (freeToPlaceUnitOnTile(curUnit, getNextTile(curX, curY, j))) {
                if (curUnit->unit_positioned == j-1 || (unitInfo[curUnit->info].can_rotate_turret && (curUnit->unit_positioned + 4) % 8 == j-1))
                    curUnit->hugging_obstacle = 1;
                return j;
            }
        }
        return UM_NONE; // can't make a move! *gasp*   should not occur though.
    }
    
    /* now we need to calculate the best move for if the unit is hugging an obstacle */
    if (curUnit->hugging_obstacle < 0) {
        nextMove = -curUnit->hugging_obstacle; //  1 + ((curUnit->unit_positioned + 2) % 8);
        for (i=0; i<8; i++) {
            j = nextMove - i;
            if (j < UM_MOVE_UP)
                j += 8;
            if (freeToPlaceUnitOnTile(curUnit, getNextTile(curX, curY, j))) {
                if (j == ignorantMove) // stop hugging obstacles if it's actually the best move anyway
                    curUnit->hugging_obstacle = 0;
                else if (1 + ((curUnit->unit_positioned + 1) % 8) == ignorantMove) { // maybe we skipped the best move, make sure we don't!
                    i = ignorantMove + 1;
                    if (i > UM_MOVE_LEFT_UP)
                        i -= 8;
                    if (i == j && freeToPlaceUnitOnTile(curUnit, getNextTile(curX, curY, ignorantMove))) {
                        curUnit->hugging_obstacle = 0;
                        return ignorantMove;
                    }
                }
                return j;
            }
        }
        return UM_NONE; // can't make a move! *gasp*
    }
//    if (curUnit->hugging_obstacle > 0) {
        nextMove = curUnit->hugging_obstacle; //  1 + (curUnit->unit_positioned - 2);
//        if (nextMove < UM_MOVE_UP)
//            nextMove += 8;
        for (i=0; i<8; i++) {
            j = nextMove + i;
            if (j > UM_MOVE_LEFT_UP)
                j -= 8;
            if (freeToPlaceUnitOnTile(curUnit, getNextTile(curX, curY, j))) {
                if (j == ignorantMove) // stop hugging obstacles if it's actually the best move anyway
                    curUnit->hugging_obstacle = 0;
                else { // maybe we skipped the best move, make sure we don't!
                    if (ignorantMove % 8 == curUnit->unit_positioned) {
                        i = j + 1;
                        if (i > UM_MOVE_LEFT_UP)
                            i -= 8;
                        if (i == ignorantMove && freeToPlaceUnitOnTile(curUnit, getNextTile(curX, curY, ignorantMove))) {
                            curUnit->hugging_obstacle = 0;
                            return ignorantMove;
                        }
                    }
                }
                return j;
            }
        }
        return UM_NONE; // can't make a move! *gasp*
//    }
//    return UM_NONE; // can't make a move! *gasp*
}

// this makes the unit move directly towards the location to be shot at, works pretty good this way ;)
enum UnitMove findBestAttackPath(int unit_nr, int curX, int curY, int shoot_range, int newX, int newY) {
    return findBestPath(unit_nr, curX, curY, newX, newY);
}
#endif


int getBestStructureToAttackAI() {
    int i, j;
    struct PriorityStructureAI *priorityStructureAI = getPriorityStructureAI();
    
    // first a quick check if any enemy structures exist
    for (i=0; i<getAmountOfSides(); i++) {
        if (i == FRIENDLY && getStructureCount(i) > 0)
           break;
    }
    if (i == getAmountOfSides())
        return -1;
    
    for (i=0; i<priorityStructureAI->amountOfItems; i++) {
        for (j=MAX_DIFFERENT_FACTIONS; j<MAX_STRUCTURES_ON_MAP; j++) {
            if (structure[j].info == priorityStructureAI->item[i] && structure[j].enabled && structure[j].side == FRIENDLY)
                return j;
        }
    }
    for (i=MAX_DIFFERENT_FACTIONS; i<MAX_STRUCTURES_ON_MAP; i++) {
        if (structure[i].enabled && structure[i].side == FRIENDLY && !structureInfo[structure[i].info].foundation)
            return i;
    }
    return -1;
}

inline int enemyOfStructureOnTile(int tile, enum Side notOfSide) {
    int structureNr;
    
    if (notOfSide == FRIENDLY && environment.layout[tile].status == UNDISCOVERED)
        return -1;

    structureNr = environment.layout[tile].contains_structure;
    if (structureNr < -1)
        structureNr = environment.layout[tile + (structureNr + 1)].contains_structure;
    if (structureNr >= MAX_DIFFERENT_FACTIONS && structure[structureNr].enabled && (!structure[structureNr].side) != (!notOfSide) && structure[structureNr].armour != INFINITE_ARMOUR)
        return structureNr;
    return -1;
}

int getNearestStructureToAttackAI(enum Side ownSide, int x, int y, int offensiveOnly, int maxRadius) {
    // basicly using Manhattan distance for ease of use
    int result;
    int maxwh;
    int i, j, k;
    int ypj_x, ymj_x;
    int xpj_st_envwidth, xmj_lt_zero, ypj_st_envheight, ymj_lt_zero;
    int xpk_st_envwidth, xmk_lt_zero, ypk_st_envheight, ymk_lt_zero;
    
    // first a quick check if any enemy structures exist    
    for (i=0; i<getAmountOfSides(); i++) {
        if (i != ownSide && getStructureCount(i) > 0)
           break;
    }
    if (i == getAmountOfSides())
        return -1;
    
    maxwh = max(environment.width, environment.height);
    if (maxRadius)
        maxwh = min(1 + maxRadius, maxwh);
    for (j=1; j<maxwh; j++) {
        
        ypj_x = TILE_FROM_XY(x, y+j);
        ymj_x = TILE_FROM_XY(x, y-j);
        
        if (y+j < environment.height && (result = enemyOfStructureOnTile(ypj_x, ownSide)) >= 0)                 // bottom column middle tile
            if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                return result; //environment.layout[ypj_x].contains_unit;
        if (x+j < environment.width && (result = enemyOfStructureOnTile(TILE_FROM_XY(x+j, y), ownSide)) >= 0)  // right column middle tile
            if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                return result; //environment.layout[environment.width * y + x + j].contains_unit;
        if (x-j >= 0 && (result = enemyOfStructureOnTile(TILE_FROM_XY(x-j, y), ownSide)) >= 0)                  // left column middle tile
            if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                return result; //environment.layout[environment.width * y + x - j].contains_unit;
        if (y-j >= 0 && (result = enemyOfStructureOnTile(ymj_x, ownSide)) >= 0)                                  // top row middle tile
            if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                return result; //environment.layout[ymj_x].contains_unit;
          
        xpj_st_envwidth = x+j < environment.width;
        xmj_lt_zero = x-j >= 0;
        ypj_st_envheight = y+j < environment.height;
        ymj_lt_zero = y-j >= 0;
        
        for (k=1; k<j; k++) {
            xpk_st_envwidth = x+k < environment.width;
            xmk_lt_zero = x-k >= 0;
            ypk_st_envheight = y+k < environment.height;
            ymk_lt_zero = y-k >= 0;
            
            if (ypj_st_envheight) { // bottom row
                if (xpk_st_envwidth && (result = enemyOfStructureOnTile(ypj_x + k, ownSide)) >= 0)
                    if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                        return result;
                if (xmk_lt_zero && (result = enemyOfStructureOnTile(ypj_x - k, ownSide)) >= 0)
                    if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                        return result;
            }
            if (xpj_st_envwidth) { // right column
                if (ypk_st_envheight && (result = enemyOfStructureOnTile(TILE_FROM_XY(x+j, y+k), ownSide)) >= 0)
                    if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                        return result;
                if (ymk_lt_zero && (result = enemyOfStructureOnTile(TILE_FROM_XY(x+j, y-k), ownSide)) >= 0)
                    if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                        return result;
            }
            if (xmj_lt_zero) { // left column
                if (ypk_st_envheight && (result = enemyOfStructureOnTile(TILE_FROM_XY(x-j, y+k), ownSide)) >= 0)
                    if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                        return result;
                if (ymk_lt_zero && (result = enemyOfStructureOnTile(TILE_FROM_XY(x-j, y-k), ownSide)) >= 0)
                    if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                        return result;
            }
            if (ymj_lt_zero) { // top row
                if (xpk_st_envwidth && (result = enemyOfStructureOnTile(ymj_x + k, ownSide)) >= 0)
                    if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                        return result;
                if (xmk_lt_zero && (result = enemyOfStructureOnTile(ymj_x - k, ownSide)) >= 0)
                    if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                        return result;
            }
        }
        
        if (y+j < environment.height) { // bottom row outer tiles (the corner tiles)
            if (x+j < environment.width && (result = enemyOfStructureOnTile(ypj_x + j, ownSide)) >= 0)
                if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                    return result;
            if (x-j >= 0 && (result = enemyOfStructureOnTile(ypj_x - j, ownSide)) >= 0)
                if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                    return result;
        }
        if (y-j >= 0) { // top row outer tiles (the corner tiles)
            if (x+j < environment.width && (result = enemyOfStructureOnTile(ymj_x + j, ownSide)) >= 0)
                if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                    return result;
            if (x-j >= 0 && (result = enemyOfStructureOnTile(ymj_x - j, ownSide)) >= 0)
                if (!offensiveOnly || structureInfo[structure[result].info].shoot_range > 0)
                    return result;
        }
    }
    
    return -1;
}

inline int enemyOfUnitOnTile(int tile, enum Side notOfSide) {
    int unitNr;
    
    if (notOfSide == FRIENDLY && environment.layout[tile].status == UNDISCOVERED)
        return -1;
    
    unitNr = environment.layout[tile].contains_unit;
    if (unitNr >= 0 && unit[unitNr].enabled && (!unit[unitNr].side) != (!notOfSide) && unit[unitNr].armour != INFINITE_ARMOUR)
        return unitNr;
    return -1;
}

int getNearestUnitToAttackAI(enum Side ownSide, int x, int y, int offensiveOnly, int maxRadius) {
    // basicly using Manhattan distance for ease of use
    int result;
    int maxwh;
    int i, j, k;
    int ypj_x, ymj_x;
    int xpj_st_envwidth, xmj_lt_zero, ypj_st_envheight, ymj_lt_zero;
    int xpk_st_envwidth, xmk_lt_zero, ypk_st_envheight, ymk_lt_zero;

    // first a quick check if any enemy units exist    
    for (i=0; i<getAmountOfSides(); i++) {
        if (i != ownSide && getUnitCount(i) > 0)
           break;
    }
    if (i == getAmountOfSides())
        return -1;
    
    maxwh = max(environment.width, environment.height);
    if (maxRadius)
        maxwh = min(1 + maxRadius, maxwh);
    for (j=1; j<maxwh; j++) {
        
        ypj_x = TILE_FROM_XY(x, y+j);
        ymj_x = TILE_FROM_XY(x, y-j);
        
        if (y+j < environment.height && (result = enemyOfUnitOnTile(ypj_x, ownSide)) >= 0)                 // bottom column middle tile
            if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                return result; //environment.layout[ypj_x].contains_unit;
        if (x+j < environment.width && (result = enemyOfUnitOnTile(TILE_FROM_XY(x+j, y), ownSide)) >= 0)  // right column middle tile
            if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                return result; //environment.layout[environment.width * y + x + j].contains_unit;
        if (x-j >= 0 && (result = enemyOfUnitOnTile(TILE_FROM_XY(x-j, y), ownSide)) >= 0)                  // left column middle tile
            if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                return result; //environment.layout[environment.width * y + x - j].contains_unit;
        if (y-j >= 0 && (result = enemyOfUnitOnTile(ymj_x, ownSide)) >= 0)                                  // top row middle tile
            if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                return result; //environment.layout[ymj_x].contains_unit;
          
        xpj_st_envwidth = x+j < environment.width;
        xmj_lt_zero = x-j >= 0;
        ypj_st_envheight = y+j < environment.height;
        ymj_lt_zero = y-j >= 0;
        
        for (k=1; k<j; k++) {
            xpk_st_envwidth = x+k < environment.width;
            xmk_lt_zero = x-k >= 0;
            ypk_st_envheight = y+k < environment.height;
            ymk_lt_zero = y-k >= 0;
            
            if (ypj_st_envheight) { // bottom row
                if (xpk_st_envwidth && (result = enemyOfUnitOnTile(ypj_x + k, ownSide)) >= 0)
                    if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                        return result; //environment.layout[ypj_x + k].contains_unit;
                if (xmk_lt_zero && (result = enemyOfUnitOnTile(ypj_x - k, ownSide)) >= 0)
                    if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                        return result; //environment.layout[ypj_x - k].contains_unit;
            }
            if (xpj_st_envwidth) { // right column
                if (ypk_st_envheight && (result = enemyOfUnitOnTile(TILE_FROM_XY(x+j, y+k), ownSide)) >= 0)
                    if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                        return result; //environment.layout[environment.width * (y + k) + x + j].contains_unit;
                if (ymk_lt_zero && (result = enemyOfUnitOnTile(TILE_FROM_XY(x+j, y-k), ownSide)) >= 0)
                    if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                        return result; //environment.layout[environment.width * (y - k) + x + j].contains_unit;
            }
            if (xmj_lt_zero) { // left column
                if (ypk_st_envheight && (result = enemyOfUnitOnTile(TILE_FROM_XY(x-j, y+k), ownSide)) >= 0)
                    if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                        return result; //environment.layout[environment.width * (y + k) + x - j].contains_unit;
                if (ymk_lt_zero && (result = enemyOfUnitOnTile(TILE_FROM_XY(x-j, y-k), ownSide)) >= 0)
                    if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                        return result; //environment.layout[environment.width * (y - k) + x - j].contains_unit;
            }
            if (ymj_lt_zero) { // top row
                if (xpk_st_envwidth && (result = enemyOfUnitOnTile(ymj_x + k, ownSide)) >= 0)
                    if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                        return result; //environment.layout[ymj_x + k].contains_unit;
                if (xmk_lt_zero && (result = enemyOfUnitOnTile(ymj_x - k, ownSide)) >= 0)
                    if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                        return result; //environment.layout[ymj_x - k].contains_unit;
            }
        }
        
        if (y+j < environment.height) { // bottom row outer tiles (the corner tiles)
            if (x+j < environment.width && (result = enemyOfUnitOnTile(ypj_x + j, ownSide)) >= 0)
                if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                    return result; //environment.layout[ypj_x + j].contains_unit;
            if (x-j >= 0 && (result = enemyOfUnitOnTile(ypj_x - j, ownSide)) >= 0)
                if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                    return result; //environment.layout[ypj_x - j].contains_unit;
        }
        if (y-j >= 0) { // top row outer tiles (the corner tiles)
            if (x+j < environment.width && (result = enemyOfUnitOnTile(ymj_x + j, ownSide)) >= 0)
                if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                    return result; //environment.layout[ymj_x + j].contains_unit;
            if (x-j >= 0 && (result = enemyOfUnitOnTile(ymj_x - j, ownSide)) >= 0)
                if (!offensiveOnly || unitInfo[unit[result].info].shoot_range > 0)
                    return result; //environment.layout[ymj_x - j].contains_unit;
        }
    }
    
    return -1;
}

// TODO: MAKE THIS OPTIMIZED! OPTIMIZED VERSION FAILS!!!
void activateUnitMoveView(int x, int y, enum UnitMove move, int viewRange) { /* x and y being the new position, not the old */
    #ifndef DISABLE_SHROUD
    char section; // up, right, down, left bit.
    char statusModifier = 0;
    int diagonal;
    int viewRange2 = viewRange * viewRange;
    int coordCalc;
    int i, j, tile;
    
    if ((diagonal = (move % 2 == 0))) // diagonal, affects only two sections
        section = BIT((move/2)-1) | BIT((move/2)%4);
    else // not diagonal, affects three sections
        section = ~(BIT(((move/2)+2)%4));
    
//    if (diagonal) { // use non-optimized stuff
        activateEntityView(x, y, 1, 1, viewRange);
        return;
//    }
    
    if (move <= UM_MOVE_RIGHT_UP && move >= UM_MOVE_LEFT_UP)
        statusModifier |= CLEAR_DOWN;
    else if (move >= UM_MOVE_RIGHT_DOWN && move <= UM_MOVE_LEFT_DOWN)
        statusModifier |= CLEAR_UP;
    if (move >= UM_MOVE_RIGHT_UP && move <= UM_MOVE_RIGHT_DOWN)
        statusModifier |= CLEAR_LEFT;
    else if (move >= UM_MOVE_LEFT_DOWN && move <= UM_MOVE_LEFT_UP)
        statusModifier |= CLEAR_RIGHT;
    
    if (section & BIT(0)) { // up
        if (y - viewRange >= 0) {
//            if (diagonal)
//                environment.layout[((y - viewRange) + 1) * environment.width + x].status = CLEAR_ALL;
            tile = TILE_FROM_XY(x, y - viewRange);
//            if (environment.layout[tile].status == UNDISCOVERED)
                setTileDirtyRadarDirtyBitmap(tile);
            environment.layout[tile].status |= statusModifier;
            adjustShroudType(x, y - viewRange);
        }
    }
    if (section & BIT(1)) { // right
        if (x + viewRange < environment.width) {
//            if (diagonal)
//                environment.layout[y * environment.width + x + viewRange - 1].status = CLEAR_ALL;
            tile = TILE_FROM_XY(x + viewRange, y);
//            if (environment.layout[tile].status == UNDISCOVERED)
                setTileDirtyRadarDirtyBitmap(tile);
            environment.layout[tile].status |= statusModifier;
            adjustShroudType(x + viewRange, y);
        }
    }
    if (section & BIT(2)) { // down
        if (y + viewRange < environment.height) {
//            if (diagonal)
//                environment.layout[(y + viewRange - 1) * environment.width + x].status = CLEAR_ALL;
            tile = TILE_FROM_XY(x, y + viewRange);
//            if (environment.layout[tile].status == UNDISCOVERED)
                setTileDirtyRadarDirtyBitmap(tile);
            environment.layout[tile].status |= statusModifier;
            adjustShroudType(x, y + viewRange);
        }
    }
    if (section & BIT(3)) { // left
        if (x - viewRange >= 0) {
//            if (diagonal)
//                environment.layout[y * environment.width + (x - viewRange) + 1].status = CLEAR_ALL;
            tile = TILE_FROM_XY(x - viewRange, y);
//            if (environment.layout[tile].status == UNDISCOVERED)
                setTileDirtyRadarDirtyBitmap(tile);
            environment.layout[tile].status |= statusModifier;
            adjustShroudType(x - viewRange, y);
        }
    }

    
    for (i=1, j=1; i<viewRange; i++) {
        for (; j <= viewRange - i; j++) {
            coordCalc = viewRange - j;
            if (coordCalc * coordCalc + i*i <= viewRange2) { // found two outer tiles
                if (section & BIT(0)) { // up
                    if (y - coordCalc >= 0) {
                        if (x - i >= 0) {
//                            if (diagonal)
//                                environment.layout[((y - coordCalc) + 1) * environment.width + (x - i)].status = CLEAR_ALL;
                            tile = TILE_FROM_XY(x-i, y - coordCalc);
//                            if (environment.layout[tile].status == UNDISCOVERED)
                                setTileDirtyRadarDirtyBitmap(tile);
                            environment.layout[tile].status |= statusModifier;
                            adjustShroudType(x - i, y - coordCalc);
                        }
                        if (x + i < environment.width) {
//                            if (diagonal)
//                                environment.layout[((y - coordCalc) + 1) * environment.width + (x + i)].status = CLEAR_ALL;
                            tile = TILE_FROM_XY(x+i, y - coordCalc);
//                            if (environment.layout[tile].status == UNDISCOVERED)
                                setTileDirtyRadarDirtyBitmap(tile);
                            environment.layout[tile].status |= statusModifier;
                            adjustShroudType(x + i, y - coordCalc);
                        }
                    }
                }
                if (section & BIT(1)) { // right
                    if (x + coordCalc < environment.width) {
                        if (y - i >= 0) {
//                            if (diagonal)
//                                environment.layout[(y - i) * environment.width + x + coordCalc - 1].status = CLEAR_ALL;
                            tile = TILE_FROM_XY(x + coordCalc, y - i);
//                            if (environment.layout[tile].status == UNDISCOVERED)
                                setTileDirtyRadarDirtyBitmap(tile);
                            environment.layout[tile].status |= statusModifier;
                            adjustShroudType(x + coordCalc, y - i);
                        }
                        if (y + i < environment.height) {
//                            if (diagonal)
//                                environment.layout[(y + i) * environment.width + x + coordCalc - 1].status = CLEAR_ALL;
                            tile = TILE_FROM_XY(x + coordCalc, y + i);
//                            if (environment.layout[tile].status == UNDISCOVERED)
                                setTileDirtyRadarDirtyBitmap(tile);
                            environment.layout[tile].status |= statusModifier;
                            adjustShroudType(x + coordCalc, y + i);
                        }
                    }
                }
                if (section & BIT(2)) { // down
                    if (y + coordCalc < environment.height) {
                        if (x - i >= 0) {
//                            if (diagonal)
//                                environment.layout[((y + coordCalc) - 1) * environment.width + (x - i)].status = CLEAR_ALL;
                            tile = TILE_FROM_XY(x-i, y + coordCalc);
//                            if (environment.layout[tile].status == UNDISCOVERED)
                                setTileDirtyRadarDirtyBitmap(tile);
                            environment.layout[tile].status |= statusModifier;
                            adjustShroudType(x - i, y + coordCalc);
                        }
                        if (x + i < environment.width) {
//                            if (diagonal)
//                                environment.layout[((y + coordCalc) - 1) * environment.width + (x + i)].status = CLEAR_ALL;
                            tile = TILE_FROM_XY(x+i, y + coordCalc);
//                            if (environment.layout[tile].status == UNDISCOVERED)
                                setTileDirtyRadarDirtyBitmap(tile);
                            environment.layout[tile].status |= statusModifier;
                            adjustShroudType(x + i, y + coordCalc);
                        }
                    }
                }
                if (section & BIT(3)) { // left
                    if (x - coordCalc >= 0) {
                        if (y - i >= 0) {
//                            if (diagonal)
//                                environment.layout[(y - i) * environment.width + (x - coordCalc) + 1].status = CLEAR_ALL;
                            tile = TILE_FROM_XY(x - coordCalc, y - i);
//                            if (environment.layout[tile].status == UNDISCOVERED)
                                setTileDirtyRadarDirtyBitmap(tile);
                            environment.layout[tile].status |= statusModifier;
                            adjustShroudType(x - coordCalc, y - i);
                        }
                        if (y + i < environment.height) {
//                            if (diagonal)
//                                environment.layout[(y + i) * environment.width + (x - coordCalc) + 1].status = CLEAR_ALL;
                            tile = TILE_FROM_XY(x - coordCalc, y + i);
//                            if (environment.layout[tile].status == UNDISCOVERED)
                                setTileDirtyRadarDirtyBitmap(tile);
                            environment.layout[tile].status |= statusModifier;
                            adjustShroudType(x - coordCalc, y + i);
                        }
                    }
                }
                break;
            }
        }
    }
    #endif
}


/*inline*/ void doUnitAttemptToShoot(struct Unit *curUnit, struct UnitInfo *curUnitInfo, int tgt_x, int tgt_y, int randomness) {
    if (randomness>0) {
        int rx=0, ry=0;
        
        if (randFixed()%4==0) { // 25% chance of having rx changed
            rx=(randFixed()%randomness)+1;  // 1 to randomness (inclusive)
            if (randFixed()%4>1)
               rx=-rx;
        }
        
        if (randFixed()%4==0) { // 25% chance of having ry changed
            ry=(randFixed()%randomness)+1;  // 1 to randomness (inclusive)
            if (randFixed()%4>1)
               ry=-ry;
        }
        
        // update target and check if still valid (can't shoot out of borders, AFAIK)
        tgt_x+=rx;
        tgt_y+=ry;
        
        if (tgt_x<0) tgt_x=0;
        if (tgt_x>=environment.width) tgt_x=environment.width-1;
        
        if (tgt_y<0) tgt_y=0;
        if (tgt_y>=environment.height) tgt_y=environment.height-1;
    }

    if (curUnitInfo->double_shot && curUnit->reload_time == DOUBLE_SHOT_AT) { // shoot
        if (curUnit->misfire_time > 0)
            curUnit->misfire_time--;
        else if (randFixed()%4 == 0) // 25% chance of misfire
            curUnit->misfire_time= (6/getGameSpeed()) -1;
        else {
            addProjectile(curUnit->side, curUnit->x, curUnit->y, tgt_x, tgt_y, curUnitInfo->projectile_info, 0);
            if (curUnitInfo->tank_shot)
                addTankShot(curUnit->x, curUnit->y);
            curUnit->reload_time--;
        }
    } else if (curUnit->reload_time <= 0) { // shoot
        if (curUnit->misfire_time > 0)
            curUnit->misfire_time--;
        else if (randFixed()%4 == 0) // 25% chance of misfire
            curUnit->misfire_time= (6/getGameSpeed()) -1;
        else {
            addProjectile(curUnit->side, curUnit->x, curUnit->y, tgt_x, tgt_y, curUnitInfo->projectile_info, curUnitInfo->double_shot);
            if (curUnitInfo->tank_shot)
                addTankShot(curUnit->x, curUnit->y);
            
            // sligtly modify reload time (-3,-2,-1,0,+1,+2,+3)*3/gameSpeed
            // curUnit->reload_time = curUnitInfo->reload_time + ((generateRandVariation(4)*3)/getGameSpeed());
            // reload_time throttling temporary deactivated
            curUnit->reload_time = curUnitInfo->reload_time;
            // if (curUnit->reload_time < 1)
            //    curUnit->reload_time = 1;
            
            
            // re-evaluate target: is it needed here?
        }
    }
}


inline void doUnitDeath(int unitnr) {
    struct Unit *curUnit = unit + unitnr;
    struct UnitInfo *curUnitInfo = unitInfo + curUnit->info;
    int mapWidth = environment.width;
    int tilenrCur, tilenrTo;
    int x, y;
    int aid;
    int i;
    
    // any paths stored for this unit should be removed
    #ifndef REMOVE_ASTAR_PATHFINDING
    removePathfindingPath(unitnr);
    #endif
    
    // if focused on, focus should stop
    if (focusOnUnitNr == unitnr)
        focusOnUnitNr = -1;
    
    // any structure attacking this unit should stop
    for (i=0; i<MAX_STRUCTURES_ON_MAP; i++) {
        if (structure[i].enabled && structure[i].logic_aid == unitnr && structure[i].logic >= SL_GUARD_UNIT)
            structure[i].logic -= 2;
    }
    // any unit attacking this unit should stop
    aid = TILE_FROM_XY(curUnit->x, curUnit->y);
    for (i=0; i<MAX_UNITS_ON_MAP; i++) {
        if (unit[i].enabled && unit[i].logic_aid == unitnr && (unit[i].logic == UL_ATTACK_UNIT || unit[i].logic == UL_GUARD_UNIT)) {
            if (getGameType() == SINGLEPLAYER) {
                if (unit[i].side != FRIENDLY) {
                    if (unit[i].group == UGAI_AMBUSH || unit[i].group == UGAI_HUNT) {
                        unit[i].logic = UL_HUNT;
                        unit[i].logic_aid = 0;
                        unit[i].group = UGAI_HUNT;
                    } else if (unit[i].group == UGAI_KAMIKAZE)
                        unit[i].logic = UL_KAMIKAZE;
                    else /*if (unit[i].group == UGAI_GUARD_AREA || unit[i].group == UGAI_PATROL)*/
                        unit[i].logic = UL_GUARD_RETREAT;
                    continue;
                }
            }
            if (unit[i].logic == UL_ATTACK_UNIT) {
                if (unit[i].group & (UGAI_ATTACK_FORCED | UGAI_ATTACK)) {
                    unit[i].logic = UL_CONTINUE_ATTACK;
                    unit[i].logic_aid = aid;
                } else /*if (unit[j].group & UGAI_GUARD)*/
                    unit[i].logic = UL_GUARD_RETREAT;
            } else {
                unit[i].logic = UL_GUARD;
                unit[i].guard_tile = TILE_FROM_XY(unit[i].x, unit[i].y); 
            }
        }
    }
    
    tilenrCur = TILE_FROM_XY(curUnit->x, curUnit->y);
    tilenrTo = tilenrCur;
    // tile which the unit is moving to set to not reference the current unit
    if (curUnit->move >= UM_MOVE_UP && curUnit->move <= UM_MOVE_LEFT_UP) {
        if (curUnit->move >= UM_MOVE_RIGHT_UP && curUnit->move <= UM_MOVE_RIGHT_DOWN)
            tilenrTo++;
        else if (curUnit->move != UM_MOVE_UP && curUnit->move != UM_MOVE_DOWN)
            tilenrTo--;
        if (curUnit->move >= UM_MOVE_RIGHT_DOWN && curUnit->move <= UM_MOVE_LEFT_DOWN)
            tilenrTo += mapWidth;
        else if (curUnit->move != UM_MOVE_LEFT && curUnit->move != UM_MOVE_RIGHT)
            tilenrTo -= mapWidth;
        if (environment.layout[tilenrTo].contains_unit == (-unitnr) - 2)
            environment.layout[tilenrTo].contains_unit = -1;
    }
    // current tile set to not contain the current unit
    if (environment.layout[tilenrCur].contains_unit == unitnr) {
        environment.layout[tilenrCur].contains_unit = -1;
        // maybe tile should be reserved by a unit which was trying to overrun this dead unit
        i = unitMovingOntoTile(tilenrCur);
        if (i >= 0)
            environment.layout[tilenrCur].contains_unit = -1 - (i+1);
        
        if ((curUnitInfo->type == UT_WHEELED || curUnitInfo->type == UT_TRACKED) &&
            environment.layout[tilenrCur].contains_structure == -1)
            addOverlay(curUnit->x, curUnit->y, OT_ROCK_SHOT, 0);
    }
    
    setTileDirtyRadarDirtyBitmap(tilenrCur);
    
    // if it was a mobile structure deploying unit, we'll only have to correct the unit count and not do anything about the remainder
    if (environment.layout[tilenrCur].contains_structure < -1 || environment.layout[tilenrCur].contains_structure >= MAX_DIFFERENT_FACTIONS) {
        setUnitCount(curUnit->side, getUnitCount(curUnit->side) - 1);
        curUnit->enabled = 0;
        return;
    }
    
    x = curUnit->x * 16 + 8;
    y = curUnit->y * 16 + 8;
    alterXYAccordingToUnitMovement(curUnit->move, curUnit->move_aid, curUnitInfo->speed, &x, &y);
    addExplosion(x, y, curUnitInfo->explosion_info, 0);
    if (curUnitInfo->graphics_size > SPRITE_SIZE_16) {
        if (x-16 >= 0)
            addExplosion(x-16, y, curUnitInfo->explosion_info, rand()%6);
        if (x+16 < mapWidth*16)
            addExplosion(x+16, y, curUnitInfo->explosion_info, rand()%6);
        if (y-16 >= 0)
            addExplosion(x, y-16, curUnitInfo->explosion_info, rand()%6);
        if (y+16 < environment.height * 16)
            addExplosion(x, y+16, curUnitInfo->explosion_info, rand()%6);
    }
    if (curUnitInfo->death_soundeffect >= 0) {
        x = curUnit->x;
        y = curUnit->y;
        if (curUnitInfo->type == UT_FOOT)
            playSoundeffectControlled(SE_UNIT_DEATH + curUnitInfo->death_soundeffect, volumePercentageOfLocation(x, y, DISTANCE_TO_HALVE_SE_FOOT_SCREAM), soundeffectPanningOfLocation(x, y));
        else if (curUnitInfo->type == UT_WHEELED)
            playSoundeffectControlled(SE_UNIT_DEATH + curUnitInfo->death_soundeffect, volumePercentageOfLocation(x, y, DISTANCE_TO_HALVE_SE_EXPLOSION_UNIT_WHEELED), soundeffectPanningOfLocation(x, y));
        else if (curUnitInfo->type == UT_TRACKED)
            playSoundeffectControlled(SE_UNIT_DEATH + curUnitInfo->death_soundeffect, volumePercentageOfLocation(x, y, DISTANCE_TO_HALVE_SE_EXPLOSION_UNIT_TRACKED), soundeffectPanningOfLocation(x, y));
        else if (curUnitInfo->type == UT_AERIAL)
            playSoundeffectControlled(SE_UNIT_DEATH + curUnitInfo->death_soundeffect, volumePercentageOfLocation(x, y, DISTANCE_TO_HALVE_SE_EXPLOSION_UNIT_TRACKED), soundeffectPanningOfLocation(x, y));
    }
    
    if (curUnit->side != FRIENDLY) {
//        playSoundeffect(SE_ENEMY_UNIT_DESTROYED);
        if ((curUnit->group & (UGAI_GUARD_AREA | UGAI_PATROL)) && getRebuildDelayUnits(curUnit->side) != -1)
            curUnit->group |= AWAITING_REPLACEMENT;
    }
    
    curUnit->enabled = 0;
    
    setUnitCount(curUnit->side, getUnitCount(curUnit->side) - 1);
    setUnitDeaths(curUnit->side, getUnitDeaths(curUnit->side) + 1);
    
    // if it was the last ore collector, make sure it's given back for free! :)
    if (curUnitInfo->can_collect_ore) {
        for (i=0; i<MAX_UNITS_ON_MAP; i++) {
            if (unit[i].enabled && unit[i].side == curUnit->side && unitInfo[unit[i].info].can_collect_ore)
                break;
        }
        if (i == MAX_UNITS_ON_MAP) { // there wasn't another ore collector for this (destroyed) unit's side
            if (getGameType() == SINGLEPLAYER) {
                for (i=0; i<MAX_STRUCTURES_ON_MAP; i++) {
                    if (structure[i].enabled && structure[i].primary && structure[i].side == curUnit->side && structure[i].armour > 0 && structureInfo[structure[i].info].can_extract_ore) { // there was a proper extracting structure for it
                        i = addUnit(curUnit->side, structure[i].x + structureInfo[structure[i].info].release_tile, structure[i].y + structureInfo[structure[i].info].height - 1, curUnit->info, 0);
                        if (i >= 0 && unit[i].side == FRIENDLY)
                            playSoundeffect(SE_MINER_DEPLOYED);
                        break;
                    }
                }
            }
        }
    }
}

#define CHECK_BLOCKING_PATH_TILES_WIDTH 3 /* must be an odd number */
void setUnitToAttackDestinationBlockingEntity(struct Unit *curUnit, int dest_x, int dest_y) {
    struct EnvironmentLayout *envLayout;
    int tileCheckNr[CHECK_BLOCKING_PATH_TILES_WIDTH];
    int structureNr;
    int i,j,k;
    
    enum Positioned faceDestination = positionedToFaceXY(curUnit->x, curUnit->y, dest_x, dest_y);
    int mapWidth  = environment.width;
    int mapHeight = environment.height;
    int range = unitInfo[curUnit->info].shoot_range;
    
    for (i=0; i<CHECK_BLOCKING_PATH_TILES_WIDTH; i++)
        tileCheckNr[i] = -1;
    
    tileCheckNr[CHECK_BLOCKING_PATH_TILES_WIDTH/2] = TILE_FROM_XY(curUnit->x, curUnit->y);
    if (faceDestination == UP || faceDestination == DOWN) { // using a horizontal set of tiles with curUnit's position as its center
        for (i=1; (i<=CHECK_BLOCKING_PATH_TILES_WIDTH/2) && ((curUnit->x-i) >= 0); i++)
            tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2)-i] = tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2)-(i-1)] - 1;
        for (i=1; (i<=CHECK_BLOCKING_PATH_TILES_WIDTH/2) && ((curUnit->x+i) < mapWidth); i++)
            tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2)+i] = tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2)+(i-1)] + 1;
    } else { // using a vertical set of tiles with curUnit's position as its center
        for (i=1; (i<=CHECK_BLOCKING_PATH_TILES_WIDTH/2) && ((curUnit->y-i) >= 0); i++)
            tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2)-i] = tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2)-(i-1)] - mapWidth;
        for (i=1; (i<=CHECK_BLOCKING_PATH_TILES_WIDTH/2) && ((curUnit->y+i) < mapHeight); i++)
            tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2)+i] = tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2)+(i-1)] + mapWidth;
    }
    
    
    for (i=0; i<range; i++) {
        for (j=0; j<=(CHECK_BLOCKING_PATH_TILES_WIDTH/2); j++) {
            for (k=0; k<2; k++) {
                j *= -1;
                
                tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2) + j] =
                    getNextTile(X_FROM_TILE(tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2) + j]),
                                Y_FROM_TILE(tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2) + j]),
                                UM_MOVE_UP + (faceDestination - UP));
                envLayout = environment.layout + tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2) + j];
                if (envLayout->contains_unit >= 0 &&
                    !unit[envLayout->contains_unit].side != !curUnit->side &&
                    withinRange(curUnit->x, curUnit->y, range, X_FROM_TILE(tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2) + j]), Y_FROM_TILE(tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2) + j])))
                {
                    // attack this opposing unit
                    curUnit->logic = UL_ATTACK_UNIT;
                    curUnit->logic_aid = envLayout->contains_unit;
                    return;
                }
                structureNr = envLayout->contains_structure;
                if (structureNr < -1)
                    structureNr = (envLayout + (structureNr + 1))->contains_structure;
                if (structureNr >= MAX_DIFFERENT_FACTIONS &&
                    !structure[structureNr].side != !curUnit->side &&
                    withinRange(curUnit->x, curUnit->y, range, X_FROM_TILE(tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2) + j]), Y_FROM_TILE(tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2) + j])))
                {
                    // attack this opposing structure
                    curUnit->logic = UL_ATTACK_LOCATION;
                    curUnit->logic_aid = tileCheckNr[(CHECK_BLOCKING_PATH_TILES_WIDTH/2) + j];
                    return;
                }
                
                if (i==0 && envLayout->contains_unit == -1 && structureNr >= -1 && structureNr < MAX_DIFFERENT_FACTIONS) 
                    return; // apparently there was room to move closer to the target,
                            // and curUnit is probably just waiting for a new path search to complete!
                
                if (j==0) break; // ensure we don't calculate the center tile twice and end up moving it along twice as far as the actual range
            }
        }
    }
}


void doUnitsLogic() {
    static int frame = 0;
    int i, j;
    int rotate;
    int tilenrTo;
    int bestStructureToAttack, closestUnitToAttack;
    int aid;
    int creditsReceived;
    struct Unit *curUnit;
    struct UnitInfo *curUnitInfo;
    struct UnitReinforcement *curUnitReinforcement;
    int mapWidth = environment.width;
    int stagingChecked[MAX_DIFFERENT_FACTIONS];
    
    startProfilingFunction("doUnitsLogic");
    
    for (i=0; i<MAX_DIFFERENT_FACTIONS; i++)
        stagingChecked[i] = 0;
    
    curUnit = &unit[0];
    for (i=0; i<MAX_UNITS_ON_MAP; i++,curUnit++) {
        if (curUnit->enabled) {
            
            curUnitInfo = unitInfo + curUnit->info;
            
            // death of unit 
            if (curUnit->armour <= 0) {
                doUnitDeath(i);
                continue;
            }
            
            // making sure that some moves of a "collector" are noted as the right thing
            if (curUnitInfo->can_collect_ore) {
                if (curUnit->logic == UL_ATTACK_LOCATION /*|| (curUnit->logic == UL_MOVE_LOCATION && isEnvironmentTileBetween(curUnit->logic_aid, ORE, OREHILL16))*/)
                    curUnit->logic = UL_MINE_LOCATION;
                else if (curUnit->logic == UL_ATTACK_UNIT) {
                    curUnit->logic = UL_MOVE_LOCATION;
                    curUnit->logic_aid = TILE_FROM_XY(unit[curUnit->logic_aid].x, unit[curUnit->logic_aid].y);
                }
                #ifdef REMOVE_ASTAR_PATHFINDING
                curUnit->hugging_obstacle = 0;
                #endif
            }
            
            // hunt or kamikaze AI
            if (getGameType() == SINGLEPLAYER) {
                if (curUnit->side != FRIENDLY && (curUnit->logic == UL_HUNT || curUnit->logic == UL_KAMIKAZE)) {
                    if (curUnit->logic == UL_HUNT && curUnit->logic_aid > 0)
                        curUnit->logic_aid--;
                    else {
                        bestStructureToAttack = getBestStructureToAttackAI();
                        if (bestStructureToAttack >= 0) {
                            curUnit->logic = UL_ATTACK_LOCATION;
                            curUnit->logic_aid = TILE_FROM_XY(structure[bestStructureToAttack].x, structure[bestStructureToAttack].y);
                            #ifdef DEBUG_BUILD
                            if (curUnit->logic_aid < 0 || curUnit->logic_aid >= mapWidth * environment.height) errorSI("hunt/kamikaze's bestStructureToAttack's attack-tile", curUnit->logic_aid);
                            #endif
                            #ifdef REMOVE_ASTAR_PATHFINDING
                            curUnit->hugging_obstacle = 0;
                            #endif
                        } else {
                            closestUnitToAttack = getNearestUnitToAttackAI(curUnit->side, curUnit->x, curUnit->y, 0, 0);
                            if (closestUnitToAttack >= 0) {
                                curUnit->logic = UL_ATTACK_UNIT;
                                curUnit->logic_aid = closestUnitToAttack;
                                #ifdef DEBUG_BUILD
                                if (TILE_FROM_XY(unit[closestUnitToAttack].x, unit[closestUnitToAttack].y) < 0 || TILE_FROM_XY(unit[closestUnitToAttack].x, unit[closestUnitToAttack].y) >= mapWidth * environment.height) errorSI("hunt/kamikaze's closestUnitToAttack's attack-tile", TILE_FROM_XY(unit[closestUnitToAttack].x, unit[closestUnitToAttack].y));
                                #endif
                                #ifdef REMOVE_ASTAR_PATHFINDING
                                curUnit->hugging_obstacle = 0;
                                #endif
                            } else {
                                // FRIENDLY is wiped out. nothing left to do. :'(
                                curUnit->logic = UL_NOTHING; // DarkEz: added this statement to prevent slo-mo bug
                            }
                        }
                        // making sure the other units in the same group attack the same
                        for (j=0; j<MAX_UNITS_ON_MAP; j++) {
                            if (unit[j].group == curUnit->group && unit[j].enabled && unit[j].side != FRIENDLY && ((unit[j].logic == UL_HUNT && unit[j].logic_aid == 0) || unit[j].logic == UL_KAMIKAZE)) {
                                unit[j].logic     = curUnit->logic;
                                unit[j].logic_aid = curUnit->logic_aid;
                            }
                        }
                    }
                }
            }
            
            // new action might be needed
            if ((curUnit->move & UM_MASK) == UM_NONE) {
                
                // unit might not even be capable of "thinking"
                if (curUnit->logic == UL_NOTHING)
                    continue;
                
                // unit might be in the middle of deployment and if that's the case, it will not need to change its move
                if (curUnitInfo->contains_structure >= 0 && getModifier() == PSTM_DEPLOY_STRUCTURE && getDeployNrUnit() == i)
                    continue;
                
                // unit might be inside a structure and if that's the case, it will not need to change its move
                j = environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].contains_structure;
                if (j < -1 || (j >= 0 && !structureInfo[structure[j].info].foundation))
                    continue;
                
                // ambush
                if (curUnit->logic == UL_AMBUSH) {
                    j = sideUnitWithinRange(!curUnit->side, curUnit->x, curUnit->y, curUnitInfo->view_range);
                    if (j >= 0) {
                        curUnit->logic = UL_ATTACK_UNIT;
                        curUnit->logic_aid = j;
                        #ifdef REMOVE_ASTAR_PATHFINDING
                        curUnit->hugging_obstacle = 0;
                        #endif
                    }
                }
                
                // continue to attack (friendly AI)
                if (curUnit->logic == UL_CONTINUE_ATTACK) {
                    // figure out what to attack. i.e. set a new (ATTACK) logic and logic_aid, remembering the old logic_aid.
                    aid = curUnit->logic_aid;
                    curUnit->logic_aid = getNearestStructureToAttackAI(curUnit->side, X_FROM_TILE(aid), Y_FROM_TILE(aid), 1, MAX_RADIUS_CONTINUE_ATTACK);
                    if (curUnit->logic_aid >= 0) {
                        curUnit->logic = UL_ATTACK_LOCATION;
                        curUnit->logic_aid = TILE_FROM_XY(structure[curUnit->logic_aid].x + structureInfo[structure[curUnit->logic_aid].info].width/2, structure[curUnit->logic_aid].y + structureInfo[structure[curUnit->logic_aid].info].height/2);
                    } else {
                        curUnit->logic_aid = getNearestUnitToAttackAI(curUnit->side, X_FROM_TILE(aid), Y_FROM_TILE(aid), 1, MAX_RADIUS_CONTINUE_ATTACK);
                        if (curUnit->logic_aid >= 0)
                            curUnit->logic = UL_ATTACK_UNIT;
                        else {
                            curUnit->logic_aid = getNearestStructureToAttackAI(curUnit->side, X_FROM_TILE(aid), Y_FROM_TILE(aid), 0, MAX_RADIUS_CONTINUE_ATTACK);
                            if (curUnit->logic_aid >= 0) {
                                curUnit->logic = UL_ATTACK_LOCATION;
                                curUnit->logic_aid = TILE_FROM_XY(structure[curUnit->logic_aid].x + structureInfo[structure[curUnit->logic_aid].info].width/2, structure[curUnit->logic_aid].y + structureInfo[structure[curUnit->logic_aid].info].height/2);
                            } else {
                                curUnit->logic_aid = getNearestUnitToAttackAI(curUnit->side, X_FROM_TILE(aid), Y_FROM_TILE(aid), 0, MAX_RADIUS_CONTINUE_ATTACK);
                                if (curUnit->logic_aid >= 0)
                                    curUnit->logic = UL_ATTACK_UNIT;
                                else {
                                    curUnit->logic = UL_GUARD;
                                    curUnit->guard_tile = TILE_FROM_XY(curUnit->x, curUnit->y);
                                }
                            }
                        }
                    }
                    
                    // find all units which have the same logic_aid (tile it -was- attacking before) and set the logic and logic_aid
                    for (j=0; j<MAX_UNITS_ON_MAP; j++) {
                        if (unit[j].enabled && unit[j].logic == UL_CONTINUE_ATTACK && unit[j].logic_aid == aid && unit[j].side == curUnit->side) {
                            unit[j].logic = curUnit->logic;
                            unit[j].logic_aid = curUnit->logic_aid;
                            unit[j].guard_tile = TILE_FROM_XY(unit[j].x, unit[j].y);
                        }
                    }
                }
                
                // if I'm an enemy unit and a friendly unit is leaning into me I should
                // shoot, if I'm not a Kamikaze (that should just ignore the friendly unit)
                // "should be capable of shooting and damaging the opponent" DKez say.
                if (curUnit->side != FRIENDLY &&
                    curUnitInfo->projectile_info >= 0 && !curUnitInfo->can_heal_foot &&
                    (curUnit->group == UGAI_HUNT || curUnit->group == UGAI_PATROL)) {
                    
                    // should not already be busy attacking something else 
                    // (i.e. it should not be within the shooting range of its target
                    // if it is set to UL_ATTACK_LOCATION/UNIT).
                    if (!((curUnit->logic==UL_ATTACK_UNIT     && withinShootRange(curUnit->x, curUnit->y, curUnitInfo->shoot_range, curUnitInfo->projectile_info, unit[curUnit->logic_aid].x, unit[curUnit->logic_aid].y))
                       || (curUnit->logic==UL_ATTACK_LOCATION && withinShootRange(curUnit->x, curUnit->y, curUnitInfo->shoot_range, curUnitInfo->projectile_info, X_FROM_TILE(curUnit->logic_aid), Y_FROM_TILE(curUnit->logic_aid))))) {
                        
                        // tell me if there's a -friendly- unit, in a radius of 1 tile
                        closestUnitToAttack = getNearestUnitToAttackAI(curUnit->side, curUnit->x, curUnit->y, 0, 2);
                        if (closestUnitToAttack!=-1 && withinShootRange(curUnit->x, curUnit->y, curUnitInfo->shoot_range, curUnitInfo->projectile_info, unit[closestUnitToAttack].x, unit[closestUnitToAttack].y)) {
                            // set mode to GUARD (stop & shoot around!)
                            curUnit->logic = UL_GUARD;
                        } else {
                            // if 'UL_GUARD' when should be 'UL_HUNT' then switch
                            if (curUnit->group == UGAI_HUNT && curUnit->logic == UL_GUARD) {
                                curUnit->logic = UL_HUNT;
                                curUnit->logic_aid = 0;
                            }
                        }
                    }
                }
                
                // move
                if (curUnit->logic == UL_MOVE_LOCATION || curUnit->logic == UL_GUARD_RETREAT || curUnit->logic == UL_RETREAT || (curUnit->logic == UL_MINE_LOCATION && curUnit->move != UM_MINE_TILE /*&& curUnit->ore < MAX_ORED_BY_UNIT*/)) {
                    if (curUnit->logic == UL_GUARD_RETREAT)
                        tilenrTo = curUnit->guard_tile;
                    else if (curUnit->logic == UL_RETREAT) {
                        if (curUnit->retreat_tile < 0) {
                            curUnit->logic = UL_GUARD;
                            curUnit->guard_tile = TILE_FROM_XY(curUnit->x, curUnit->y);
                            continue;
                        }
                        tilenrTo = curUnit->retreat_tile;
                    } else
                        tilenrTo = curUnit->logic_aid;
                    
                    if (TILE_FROM_XY(curUnit->x, curUnit->y) == tilenrTo) { // destination has already been reached?
                        if (curUnit->logic == UL_MOVE_LOCATION || curUnit->logic == UL_GUARD_RETREAT || (curUnit->logic == UL_RETREAT && !curUnitInfo->can_collect_ore)) {
                            curUnit->logic = UL_GUARD;
                            curUnit->guard_tile = tilenrTo;
                            if (getGameType() == SINGLEPLAYER) {
                                if (curUnit->side != FRIENDLY) {
                                    if (curUnit->group & (UGAI_GUARD_AREA | UGAI_PATROL))
                                        curUnit->logic = UL_GUARD_AREA;
                                    if ((curUnit->group & UGAI_STAGING) && !stagingChecked[curUnit->side]) {
                                        // if all other staging units of this side have logic != UL_MOVE_LOCATION, we send them off to do their jobs
                                        for (j=0; j<MAX_UNITS_ON_MAP; j++) {
                                            if (j!=i && unit[j].enabled && unit[j].side == curUnit->side && (unit[j].group & UGAI_STAGING) && unit[j].logic == UL_MOVE_LOCATION)
                                                break;
                                        }
                                        if (j == MAX_UNITS_ON_MAP) {
                                            for (j=0; j<MAX_UNITS_ON_MAP; j++) {
                                                if (unit[j].enabled && unit[j].side == curUnit->side && (unit[j].group & UGAI_STAGING)) {
                                                    unit[j].group &= (~UGAI_STAGING);
                                                    if (unit[j].group == UGAI_HUNT) {
                                                        unit[j].logic = UL_HUNT;
                                                        unit[j].logic_aid = 0;
                                                    } else if (unit[j].group == UGAI_KAMIKAZE)
                                                        unit[j].logic = UL_KAMIKAZE;
                                                }
                                            }
                                        }
                                        stagingChecked[curUnit->side] = 1;
                                    }
                                }
                            }
                        }
                    } else {
                        #ifdef DEBUG_BUILD
                        if (tilenrTo < 0 || tilenrTo >= mapWidth * environment.height) { breakpointSI("dest@doUnitsLogic-x1", tilenrTo);
                        errorI4(i, TILE_FROM_XY(curUnit->x, curUnit->y), curUnit->info, curUnit->logic);}
                        #endif
                        curUnit->move = findBestPath(i, curUnit->x, curUnit->y, X_FROM_TILE(tilenrTo), Y_FROM_TILE(tilenrTo));
                        if (curUnit->move == UM_NONE && (curUnit->logic == UL_MOVE_LOCATION || curUnit->logic == UL_GUARD_RETREAT || (curUnit->logic == UL_RETREAT && !curUnitInfo->can_collect_ore))) {
                            curUnit->logic = UL_GUARD;
                            curUnit->guard_tile = TILE_FROM_XY(curUnit->x, curUnit->y);
                            if (getGameType() == SINGLEPLAYER) {
                                if (curUnit->side != FRIENDLY && (curUnit->group & (UGAI_GUARD_AREA | UGAI_PATROL)))
                                    curUnit->logic = UL_GUARD_AREA;
                            }
                        // DarkEz Fix for: Feeders try to collect resource from a rallytile, and won't continue to another tile before that tile is empty and can be attempted to harvest.
                        } else if (curUnit->move == UM_NONE && curUnit->logic == UL_MINE_LOCATION) {
                            curUnit->logic_aid = TILE_FROM_XY(curUnit->x, curUnit->y); // make current position the tile to try to extract ore from.
                        } else if (curUnit->move != UM_MOVE_HOLD && curUnit->move != UM_MOVE_HOLD_INITIAL) {
                            curUnit->move_aid = curUnitInfo->speed;
                            rotate = 0;
                            if (curUnitInfo->can_rotate_turret) {
                                rotate = rotateToFace(curUnit->turret_positioned, curUnit->move - 1);
                                if (rotate > 0)
                                    curUnit->move = UM_ROTATE_TURRET_LEFT + rotate - 1;
                                else {
                                    rotate = rotateBaseToFace(curUnit->unit_positioned, curUnit->move - 1);
                                    if (rotate > 0)
                                        curUnit->move = UM_ROTATE_LEFT + rotate - 1;
                                }
                            } else {
                                rotate = rotateToFace(curUnit->unit_positioned, curUnit->move - 1);
                                if (rotate > 0)
                                    curUnit->move = UM_ROTATE_LEFT + rotate - 1;
                            }
                            if (rotate)
                                curUnit->move_aid = curUnitInfo->rotate_speed;
                            else {
                                curUnit->unit_positioned = curUnit->move - 1; // making sure, even with tracked units
                                #ifdef REMOVE_ASTAR_PATHFINDING
                                if (curUnit->hugging_obstacle < 0)
                                    curUnit->hugging_obstacle = -(1 + ((curUnit->unit_positioned + 2) % 8));
                                else if (curUnit->hugging_obstacle > 0) {
                                    curUnit->hugging_obstacle = 1 + (curUnit->unit_positioned - 2);
                                    if (curUnit->hugging_obstacle < UM_MOVE_UP)
                                        curUnit->hugging_obstacle += 8;
                                }
                                #endif
                                if (/*curUnitInfo->type == UT_TRACKED*/ curUnitInfo->tracks_type >= 0 && (environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].graphics <= SANDHILL16 || environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].graphics >= ORE))
                                    addOverlay(curUnit->x, curUnit->y, OT_TRACKS + curUnitInfo->tracks_type, curUnit->move - 1);
                                reserveTileForUnit(i, curUnit->x, curUnit->y, curUnit->move);
                            }
                        }
                    }
                }
                
                // attack
                if (curUnit->logic == UL_ATTACK_UNIT) {
                    if (!withinShootRange(curUnit->x, curUnit->y, curUnitInfo->shoot_range, curUnitInfo->projectile_info, unit[curUnit->logic_aid].x, unit[curUnit->logic_aid].y)) { // move
                        if (((curUnit->side == FRIENDLY && (curUnit->group & UGAI_GUARD)) ||
                             (curUnit->side != FRIENDLY && (curUnit->group & (UGAI_GUARD_AREA | UGAI_PATROL)))) &&
                            (abs(Y_FROM_TILE(curUnit->guard_tile) - unit[curUnit->logic_aid].y) > MAX_RADIUS_CHASE_ENEMY_UNIT + curUnitInfo->shoot_range ||
                             abs(X_FROM_TILE(curUnit->guard_tile) - unit[curUnit->logic_aid].x) > MAX_RADIUS_CHASE_ENEMY_UNIT + curUnitInfo->shoot_range))
                            // the unit which we're going after has gone miles away. stop chasing it.
                            curUnit->logic = UL_GUARD_RETREAT;
                        else {
                            // feel free to chase the unit
                            #ifdef DEBUG_BUILD
                            if (TILE_FROM_XY(unit[curUnit->logic_aid].x, unit[curUnit->logic_aid].y) < 0 || TILE_FROM_XY(unit[curUnit->logic_aid].x, unit[curUnit->logic_aid].y) >= environment.width * environment.height) errorSI("dest@doLogicUnits-1", TILE_FROM_XY(unit[curUnit->logic_aid].x, unit[curUnit->logic_aid].y));
                            #endif
                            curUnit->move = findBestAttackPath(i, curUnit->x, curUnit->y, curUnitInfo->shoot_range, unit[curUnit->logic_aid].x, unit[curUnit->logic_aid].y);
                            if ((curUnit->move & UM_MASK) != UM_NONE) {
                                curUnit->move_aid = curUnitInfo->speed;
                                rotate = 0;
                                if (curUnitInfo->can_rotate_turret) {
                                    rotate = rotateToFace(curUnit->turret_positioned, curUnit->move - 1);
                                    if (rotate > 0)
                                        curUnit->move = UM_ROTATE_TURRET_LEFT + rotate - 1;
                                    else {
                                        rotate = rotateBaseToFace(curUnit->unit_positioned, curUnit->move - 1);
                                        if (rotate > 0)
                                            curUnit->move = UM_ROTATE_LEFT + rotate - 1;
                                    }
                                } else {
                                    rotate = rotateToFace(curUnit->unit_positioned, curUnit->move - 1);
                                    if (rotate > 0)
                                        curUnit->move = UM_ROTATE_LEFT + rotate - 1;
                                }
                                if (rotate)
                                    curUnit->move_aid = curUnitInfo->rotate_speed;
                                else {
                                    curUnit->unit_positioned = curUnit->move - 1; // making sure, even with tracked units
                                    #ifdef REMOVE_ASTAR_PATHFINDING
                                    if (curUnit->hugging_obstacle < 0)
                                        curUnit->hugging_obstacle = -(1 + ((curUnit->unit_positioned + 2) % 8));
                                    else if (curUnit->hugging_obstacle > 0) {
                                        curUnit->hugging_obstacle = 1 + (curUnit->unit_positioned - 2);
                                        if (curUnit->hugging_obstacle < UM_MOVE_UP)
                                            curUnit->hugging_obstacle += 8;
                                    }
                                    #endif
                                    if (/*curUnitInfo->type == UT_TRACKED*/ curUnitInfo->tracks_type >= 0 && (environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].graphics <= SANDHILL16 || environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].graphics >= ORE))
                                        addOverlay(curUnit->x, curUnit->y, OT_TRACKS + curUnitInfo->tracks_type, curUnit->move - 1);
                                    reserveTileForUnit(i, curUnit->x, curUnit->y, curUnit->move);
                                }
                            }
                        }
                    }
                    else { /*if (curUnit->logic == UL_ATTACK_UNIT) { // make sure it hasn't changed to GUARD_RETREAT
                        if (curUnit->move == UM_MOVE_HOLD) {
                        } else if ((curUnit->move & UM_MASK) == UM_NONE) {*/ // rotate or shoot
                            if (curUnitInfo->can_rotate_turret) {
                                curUnit->move = rotateToFaceXY(curUnit->x, curUnit->y, curUnit->turret_positioned, unit[curUnit->logic_aid].x, unit[curUnit->logic_aid].y);
                                if (curUnit->move != UM_NONE) {
                                    curUnit->move += UM_ROTATE_TURRET_LEFT - 1;
                                    curUnit->move_aid = curUnitInfo->rotate_speed;
                                }
                            } else {
                                curUnit->move = rotateToFaceXY(curUnit->x, curUnit->y, curUnit->unit_positioned, unit[curUnit->logic_aid].x, unit[curUnit->logic_aid].y);
                                if (curUnit->move != UM_NONE) {
                                    curUnit->move += UM_ROTATE_LEFT - 1;
                                    curUnit->move_aid = curUnitInfo->rotate_speed;
                                }
                            }
                            if (curUnit->move == UM_NONE)
                                doUnitAttemptToShoot(curUnit, curUnitInfo, unit[curUnit->logic_aid].x, unit[curUnit->logic_aid].y, 0);
                        /*}*/
                    }
                }
                if (curUnit->logic == UL_ATTACK_LOCATION || curUnit->logic == UL_ATTACK_AREA) {
                    if (!withinShootRange(curUnit->x, curUnit->y, curUnitInfo->shoot_range, curUnitInfo->projectile_info, X_FROM_TILE(curUnit->logic_aid), Y_FROM_TILE(curUnit->logic_aid))) { // move
                        #ifdef DEBUG_BUILD
                        if (curUnit->logic_aid < 0 || curUnit->logic_aid >= environment.width * environment.height) errorSI("dest@doLogicUnits-2", curUnit->logic_aid);
                        #endif
                        curUnit->move = findBestAttackPath(i, curUnit->x, curUnit->y, curUnitInfo->shoot_range, X_FROM_TILE(curUnit->logic_aid), Y_FROM_TILE(curUnit->logic_aid));
                        if ((curUnit->move & UM_MASK) == UM_NONE) {
                            // perhaps some enemy object is blocking our path (and within projectile reach); if so, eradicate it!
                            setUnitToAttackDestinationBlockingEntity(curUnit, X_FROM_TILE(curUnit->logic_aid), Y_FROM_TILE(curUnit->logic_aid));
                        }
                        else {
                            curUnit->move_aid = curUnitInfo->speed;
                            rotate = 0;
                            if (curUnitInfo->can_rotate_turret) {
                                rotate = rotateToFace(curUnit->turret_positioned, curUnit->move - 1);
                                if (rotate > 0)
                                    curUnit->move = UM_ROTATE_TURRET_LEFT + rotate - 1;
                                else {
                                    rotate = rotateBaseToFace(curUnit->unit_positioned, curUnit->move - 1);
                                    if (rotate > 0)
                                        curUnit->move = UM_ROTATE_LEFT + rotate - 1;
                                }
                            } else {
                                rotate = rotateToFace(curUnit->unit_positioned, curUnit->move - 1);
                                if (rotate > 0)
                                    curUnit->move = UM_ROTATE_LEFT + rotate - 1;
                            }
                            if (rotate)
                                curUnit->move_aid = curUnitInfo->rotate_speed;
                            else {
                                curUnit->unit_positioned = curUnit->move - 1; // making sure, even with tracked units
                                #ifdef REMOVE_ASTAR_PATHFINDING
                                if (curUnit->hugging_obstacle < 0)
                                    curUnit->hugging_obstacle = -(1 + ((curUnit->unit_positioned + 2) % 8));
                                else if (curUnit->hugging_obstacle > 0) {
                                    curUnit->hugging_obstacle = 1 + (curUnit->unit_positioned - 2);
                                    if (curUnit->hugging_obstacle < UM_MOVE_UP)
                                        curUnit->hugging_obstacle += 8;
                                }
                                #endif
                                if (/*curUnitInfo->type == UT_TRACKED*/ curUnitInfo->tracks_type >= 0 && (environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].graphics <= SANDHILL16 || environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].graphics >= ORE))
                                    addOverlay(curUnit->x, curUnit->y, OT_TRACKS + curUnitInfo->tracks_type, curUnit->move - 1);
                                reserveTileForUnit(i, curUnit->x, curUnit->y, curUnit->move);
                            }
                        }
                    } else {
                        if ((curUnit->move & UM_MASK) == UM_NONE) { // rotate or shoot
                            if (curUnitInfo->can_rotate_turret) {
                                curUnit->move = rotateToFaceXY(curUnit->x, curUnit->y, curUnit->turret_positioned, X_FROM_TILE(curUnit->logic_aid), Y_FROM_TILE(curUnit->logic_aid));
                                if (curUnit->move != UM_NONE) {
                                    curUnit->move += UM_ROTATE_TURRET_LEFT - 1;
                                    curUnit->move_aid = curUnitInfo->rotate_speed;
                                }
                            } else {
                                curUnit->move = rotateToFaceXY(curUnit->x, curUnit->y, curUnit->unit_positioned, X_FROM_TILE(curUnit->logic_aid), Y_FROM_TILE(curUnit->logic_aid));
                                if (curUnit->move != UM_NONE) {
                                    curUnit->move += UM_ROTATE_LEFT - 1;
                                    curUnit->move_aid = curUnitInfo->rotate_speed;
                                }
                            }
                            if (curUnit->move == UM_NONE)
                                doUnitAttemptToShoot(curUnit, curUnitInfo, X_FROM_TILE(curUnit->logic_aid), Y_FROM_TILE(curUnit->logic_aid), 0);
                        }
                    }
                }
            
                // guard
                if (curUnit->logic == UL_GUARD) {
                    if (curUnitInfo->can_heal_foot)
                        curUnit->logic = UL_GUARD_AREA;
                }
                if (curUnit->logic == UL_GUARD || (curUnit->move == UM_MOVE_HOLD && !curUnitInfo->can_heal_foot)) {
                    // Check if an enemy unit is within shooting range, but ensure this doesn't 
                    // happen every frame, which would slow down the game considerably.
                    if ((frame + i) % (FPS / getGameSpeed()) == 0) {
                        aid = sideUnitWithinShootRange(!curUnit->side, curUnit->x, curUnit->y, curUnitInfo->shoot_range, curUnitInfo->projectile_info);
                        if (aid != -1) {
                            curUnit->logic = UL_GUARD_UNIT;
                            curUnit->logic_aid = aid;
                        }
                    }
                }
                if (curUnit->logic == UL_GUARD_AREA || (curUnit->move == UM_MOVE_HOLD && curUnitInfo->can_heal_foot)) {
                    aid = (curUnitInfo->can_heal_foot) ?
                           damagedFootWithinRange(curUnit->side, curUnit->x, curUnit->y, curUnitInfo->view_range) :
                           sideUnitWithinRange(!curUnit->side, curUnit->x, curUnit->y, curUnitInfo->view_range);
                    if (aid != -1) {
                        curUnit->logic = UL_ATTACK_UNIT;
                        curUnit->logic_aid = aid;
                    } else if (curUnit->group & UGAI_PATROL) {
                        if (rand() % UNITS_PATROLLING_MOVE_CHANCE == 0) {
                            curUnit->logic = UL_GUARD_RETREAT;
                            curUnit->guard_tile = curUnit->retreat_tile;
                            j = (rand() % (2*MAX_RADIUS_PATROLLING + 1)) - MAX_RADIUS_PATROLLING; // adjust y coordinate
                            if (Y_FROM_TILE(curUnit->retreat_tile) + j < 0)
                                j = 0;
                            else if (Y_FROM_TILE(curUnit->retreat_tile) + j >= environment.height)
                                j = environment.height - 1;
                            curUnit->guard_tile += j * mapWidth;
                            j = (rand() % (2*MAX_RADIUS_PATROLLING + 1)) - MAX_RADIUS_PATROLLING; // adjust x coordinate
                            if (X_FROM_TILE(curUnit->retreat_tile) + j < 0)
                                j = 0;
                            else if (X_FROM_TILE(curUnit->retreat_tile) + j >= environment.width)
                                j = environment.width - 1;
                            curUnit->guard_tile += j;
                        }
                    }
                    #ifdef REMOVE_ASTAR_PATHFINDING
                    curUnit->hugging_obstacle = 0;
                    #endif
                }
                if (curUnit->logic == UL_GUARD_UNIT) {
                    if (!withinShootRange(curUnit->x, curUnit->y, curUnitInfo->shoot_range, curUnitInfo->projectile_info, unit[curUnit->logic_aid].x, unit[curUnit->logic_aid].y)) {
                        curUnit->logic = UL_GUARD;
                    } else {
                        if (curUnitInfo->can_rotate_turret) {
                            curUnit->move = rotateToFaceXY(curUnit->x, curUnit->y, curUnit->turret_positioned, unit[curUnit->logic_aid].x, unit[curUnit->logic_aid].y);
                            if (curUnit->move != UM_NONE) {
                                curUnit->move += UM_ROTATE_TURRET_LEFT - 1;
                                curUnit->move_aid = curUnitInfo->rotate_speed;
                            }
                        } else {
                            curUnit->move = rotateToFaceXY(curUnit->x, curUnit->y, curUnit->unit_positioned, unit[curUnit->logic_aid].x, unit[curUnit->logic_aid].y);
                            if (curUnit->move != UM_NONE) {
                                curUnit->move += UM_ROTATE_LEFT - 1;
                                curUnit->move_aid = curUnitInfo->rotate_speed;
                            }
                        }
                        if (curUnit->move == UM_NONE)
                            doUnitAttemptToShoot(curUnit, curUnitInfo, unit[curUnit->logic_aid].x, unit[curUnit->logic_aid].y, 0);
                    }
                }
                
                // mine
                if (curUnit->logic == UL_MINE_LOCATION && curUnit->logic_aid == TILE_FROM_XY(curUnit->x, curUnit->y)) {
                    if (curUnit->ore >= MAX_ORED_BY_UNIT) { // move to structure which can extract ore
                        curUnit->logic = UL_RETREAT;
                        curUnit->move = UM_NONE;
                        if (curUnit->side == FRIENDLY)
                            playSoundeffect(SE_MINER_FULL);
                    } else {
                        curUnit->move = UM_MINE_TILE;
                        curUnit->move_aid = 0;
                    }
                }
            }
            
            // unit smoke
            if ((curUnit->move & UM_MASK) != UM_NONE && curUnit->armour < curUnitInfo->max_armour / 4)
                curUnit->smoke_time = UNIT_SMOKE_DURATION + (curUnit->smoke_time % (3 * UNIT_SINGLE_SMOKE_DURATION));
            
            // unit possibly stopping the idling timer
            if (curUnit->smoke_time < 0 && (curUnit->move & UM_MASK) != UM_NONE)
                curUnit->smoke_time = 0;
            
            // unit movement
            if (curUnit->move >= UM_MOVE_UP && curUnit->move <= UM_MOVE_LEFT_UP) {
                curUnit->move_aid--;
                if (curUnit->move_aid == curUnitInfo->speed/2) {
                    setTileDirtyRadarDirtyBitmap(TILE_FROM_XY(curUnit->x, curUnit->y));
                    environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].contains_unit = -1;
                    switch (curUnit->move) {
                        case UM_MOVE_LEFT_UP:
                            curUnit->x -= 1;
                            curUnit->y -= 1;
                            break;
                        case UM_MOVE_RIGHT_UP:
                            curUnit->x += 1;
                            curUnit->y -= 1;
                            break;
                        case UM_MOVE_LEFT_DOWN:
                            curUnit->x -= 1;
                            curUnit->y += 1;
                            break;
                        case UM_MOVE_RIGHT_DOWN:
                            curUnit->x += 1;
                            curUnit->y += 1;
                            break;
                        case UM_MOVE_LEFT:
                            curUnit->x -= 1;
                            break;
                        case UM_MOVE_RIGHT:
                            curUnit->x += 1;
                            break;
                        case UM_MOVE_UP:
                            curUnit->y -= 1;
                            break;
                        case UM_MOVE_DOWN:
                            curUnit->y += 1;
                            break;
                        default: break;
                    }
                    if (environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].contains_unit >= 0) { // must've been infantry now run over
                        unit[environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].contains_unit].armour = 0;
                        addOverlay(curUnit->x, curUnit->y, OT_BLOOD, 0);
                        playSoundeffectControlled(SE_FOOT_CRUSH, volumePercentageOfLocation(curUnit->x, curUnit->y, DISTANCE_TO_HALVE_SE_FOOT_CRUSH), soundeffectPanningOfLocation(curUnit->x, curUnit->y));
                    }
                    
                    j = environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].contains_structure;
                    if (j < -1)
                        j = environment.layout[TILE_FROM_XY(curUnit->x + j + 1, curUnit->y)].contains_structure;
                    if (j >= 0 && !structureInfo[structure[j].info].foundation) { // unit entered a structure
                        environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].contains_unit = -1;
                        curUnit->selected = 0;
                        curUnit->move = UM_NONE;
                        curUnit->move_aid = 0;
                        curUnit->x = structure[j].x + structureInfo[structure[j].info].release_tile;
                        curUnit->y = structure[j].y + structureInfo[structure[j].info].height - 1;
                        curUnit->unit_positioned = DOWN;
                        if (structure[j].contains_unit >= 0) // structure already contains a unit
                            dropUnit(i, structure[j].x + structureInfo[structure[j].info].release_tile, structure[j].y + structureInfo[structure[j].info].height - 1);
                        else {
                            structure[j].contains_unit = i;
//                          if (structureInfo[structure[j].info].can_extract_ore)
//                              environment.layout[curUnit->y*mapWidth + curUnit->x].contains_unit = i;
                            
                            // any structure attacking this unit should stop
                            for (j=0; j<MAX_STRUCTURES_ON_MAP; j++) {
                                if (structure[j].enabled && structure[j].logic >= SL_GUARD_UNIT && structure[j].logic_aid == i)
                                    structure[j].logic -= 2;
                            }
                            
                            // any unit attacking this unit should either stop or divert its attention to the structure
                            for (j=0; j<MAX_UNITS_ON_MAP; j++) {
                                if (unit[j].enabled && unit[j].logic_aid == i) {
                                    if (unit[j].logic == UL_ATTACK_UNIT) {
                                        unit[j].logic = UL_ATTACK_LOCATION;
                                        unit[j].logic_aid = TILE_FROM_XY(curUnit->x, curUnit->y);
                                    }
                                    if (unit[j].logic == UL_GUARD_UNIT)
                                        unit[j].logic = UL_GUARD_RETREAT;
                                }
                            }
                        }
                    } else {
                        environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].contains_unit = i;
                        setTileDirtyRadarDirtyBitmap(TILE_FROM_XY(curUnit->x, curUnit->y));
                        if (curUnit->side == FRIENDLY)
                            activateUnitMoveView(curUnit->x, curUnit->y, curUnit->move, curUnitInfo->view_range);
                    }
//                    // if a unit is going into the shroud, deselect it
//                    if (curUnit->side != FRIENDLY && environment.layout[curUnit->y*mapWidth + curUnit->x].status != CLEAR_ALL)
//                        curUnit->selected = 0;
                }
                if (curUnit->move_aid <= 0) curUnit->move = UM_NONE;
            }
            
            // rotation of chassis
            if (curUnit->move == UM_ROTATE_LEFT || (curUnit->move & UM_MASK) == UM_IDLE1 || (curUnit->move & UM_MASK) == UM_IDLE4) {
                curUnit->move_aid--;
                if (curUnit->move_aid <= 0) {
                    if (curUnit->unit_positioned == UP) curUnit->unit_positioned = LEFT_UP;
                    else curUnit->unit_positioned = curUnit->unit_positioned - 1;
                    if (curUnit->move == UM_IDLE1_HOLD || (curUnit->move == UM_IDLE1 && (curUnit->logic == UL_GUARD || curUnit->logic == UL_GUARD_AREA || curUnit->logic == UL_AMBUSH))) {
                        curUnit->move++; // going to the next UM_IDLE or UM_IDLE_HOLD
                        curUnit->move_aid = curUnitInfo->rotate_speed;
                    } else
                        curUnit->move = (curUnit->move == UM_IDLE4_HOLD) ? UM_MOVE_HOLD : UM_NONE;
                }
            }
            if (curUnit->move == UM_ROTATE_RIGHT || (curUnit->move & UM_MASK) == UM_IDLE2 || (curUnit->move & UM_MASK) == UM_IDLE3) {
                curUnit->move_aid--;
                if (curUnit->move_aid <= 0) {
                    if (curUnit->unit_positioned == LEFT_UP) curUnit->unit_positioned = UP;
                    else curUnit->unit_positioned = curUnit->unit_positioned + 1;
                    if ((curUnit->move == UM_IDLE2_HOLD || curUnit->move == UM_IDLE3_HOLD) || (curUnit->move != UM_ROTATE_RIGHT && (curUnit->logic == UL_GUARD || curUnit->logic == UL_GUARD_AREA || curUnit->logic == UL_AMBUSH))) {
                        curUnit->move++; // going to the next UM_IDLE or UM_IDLE_HOLD
                        curUnit->move_aid = curUnitInfo->rotate_speed;
                    } else
                        curUnit->move = UM_NONE;
                }
            }
            
            // rotation of turret
            if (curUnit->move == UM_ROTATE_TURRET_LEFT || (curUnit->move & UM_MASK) == UM_IDLE_TURRET1 || (curUnit->move & UM_MASK) == UM_IDLE_TURRET4) {
                curUnit->move_aid--;
                if (curUnit->move_aid == curUnitInfo->rotate_speed / 2) {
                    if (curUnit->turret_positioned == UP) curUnit->turret_positioned = LEFT_UP;
                    else curUnit->turret_positioned = curUnit->turret_positioned - 1;
                }
                if (curUnit->move_aid <= 0) {
                    if (curUnit->move == UM_IDLE_TURRET1_HOLD || (curUnit->move == UM_IDLE_TURRET1 && (curUnit->logic == UL_GUARD || curUnit->logic == UL_GUARD_AREA || curUnit->logic == UL_AMBUSH))) {
                        curUnit->move++; // going to the next UM_IDLE_TURRET or UM_IDLE_TURRET_HOLD
                        curUnit->move_aid = curUnitInfo->rotate_speed;
                    } else
                        curUnit->move = (curUnit->move == UM_IDLE_TURRET4_HOLD) ? UM_MOVE_HOLD : UM_NONE;
                }
            }
            if (curUnit->move == UM_ROTATE_TURRET_RIGHT || (curUnit->move & UM_MASK) == UM_IDLE_TURRET2 || (curUnit->move & UM_MASK) == UM_IDLE_TURRET3) {
                curUnit->move_aid--;
                if (curUnit->move_aid == curUnitInfo->rotate_speed / 2) {
                    if (curUnit->turret_positioned == LEFT_UP) curUnit->turret_positioned = UP;
                    else curUnit->turret_positioned = curUnit->turret_positioned + 1;
                }
                if (curUnit->move_aid <= 0) {
                    if ((curUnit->move == UM_IDLE_TURRET2_HOLD || curUnit->move == UM_IDLE_TURRET3_HOLD) || (curUnit->move != UM_ROTATE_TURRET_RIGHT && (curUnit->logic == UL_GUARD || curUnit->logic == UL_GUARD_AREA || curUnit->logic == UL_AMBUSH))) {
                        curUnit->move++; // going to the next UM_IDLE_TURRET or UM_IDLE_TURRET_HOLD
                        curUnit->move_aid = curUnitInfo->rotate_speed;
                    } else
                        curUnit->move = UM_NONE;
                }
            }
            
            // unit mining
            if (curUnit->move == UM_MINE_TILE) {
                if (!(curUnit->logic == UL_MINE_LOCATION && curUnit->logic_aid == TILE_FROM_XY(curUnit->x, curUnit->y)))
                    curUnit->move = UM_NONE;
                else {
                    if (curUnit->ore >= MAX_ORED_BY_UNIT) { // move to structure which can extract ore
//                        curUnit->logic = UL_RETREAT;
                        curUnit->move = UM_NONE;
//                        if (curUnit->side == FRIENDLY)
//                            playSoundeffect(SE_MINER_FULL);
                    } else {
                        aid = environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].ore_level;
                        if (aid > 0) {
                            creditsReceived = (((curUnit->move_aid+1) * (MAX_ENVIRONMENT_ORE_LEVEL/2)) / MAX_TIME_TO_ORE_TILE) -
                                              (((curUnit->move_aid) * (MAX_ENVIRONMENT_ORE_LEVEL/2)) / MAX_TIME_TO_ORE_TILE);
                            if (creditsReceived > aid)
                                creditsReceived = aid;
                            if (curUnit->ore + creditsReceived > MAX_ORED_BY_UNIT)
                                creditsReceived = MAX_ORED_BY_UNIT - curUnit->ore;
                            reduceEnvironmentOre(TILE_FROM_XY(curUnit->x, curUnit->y), creditsReceived);
                            curUnit->ore += creditsReceived;
                            curUnit->move_aid++;
                            if ((curUnit->move_aid % (3 * FPS) == 0) && curUnit->x >= getViewCurrentX() && curUnit->x < getViewCurrentX() + HORIZONTAL_WIDTH && curUnit->y >= getViewCurrentY() && curUnit->y < getViewCurrentY() + HORIZONTAL_HEIGHT)
                                playSoundeffect(SE_MINING);
                        }
                        if (curUnit->ore >= MAX_ORED_BY_UNIT) {
                            curUnit->move = UM_NONE;
                        } else if ((aid > MAX_ENVIRONMENT_ORE_LEVEL/2 && environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].ore_level <= MAX_ENVIRONMENT_ORE_LEVEL/2) ||
                                   (environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].ore_level <= 0)) { // move to a nearby place to ore and mine that. otherwise continue mining here.
                            tilenrTo = getNextTile(curUnit->x, curUnit->y, curUnit->unit_positioned + 1);
                            if (isEnvironmentTileBetween(tilenrTo, ORE, OREHILL16) && environment.layout[tilenrTo].contains_unit == -1)
                                curUnit->logic_aid = tilenrTo;
                            else {
                                tilenrTo = nearestEnvironmentTileUnoccupied(TILE_FROM_XY(curUnit->x, curUnit->y), ORE, OREHILL16);
                                if (tilenrTo >= 0)
                                    curUnit->logic_aid = tilenrTo;
                                else if (environment.layout[TILE_FROM_XY(curUnit->x, curUnit->y)].ore_level <= 0) // no other place to harvest
                                    curUnit->logic = UL_RETREAT;
                            }
                            curUnit->move = UM_NONE;
                        }
                    }
                }
            }
            
            if (curUnit->reload_time > 0 && !(curUnitInfo->double_shot && curUnit->reload_time == DOUBLE_SHOT_AT))
                curUnit->reload_time--;
            
            if (curUnit->smoke_time > 0) { // concerning smoke
                curUnit->smoke_time--;
                if (curUnitInfo->type == UT_FOOT && (curUnit->smoke_time % (3 * FPS) == 0) && curUnit->x >= getViewCurrentX() && curUnit->x < getViewCurrentX() + HORIZONTAL_WIDTH && curUnit->y >= getViewCurrentY() && curUnit->y < getViewCurrentY() + HORIZONTAL_HEIGHT)
                    playSoundeffect(SE_FOOT_HURT);
            } else if (getGameType() == SINGLEPLAYER) {
                if (curUnitInfo->idle_ani) {
                    if (curUnit->smoke_time > -UNITS_IDLE_TIME_THRESHHOLD) // concerning idling
                        curUnit->smoke_time--;
                    else if (curUnit->smoke_time == -UNITS_IDLE_TIME_THRESHHOLD) { // maybe let the unit start looking around out of boredom
                        if ((rand() % /*UNITS_IDLE_CHANCE*/ (curUnitInfo->idle_ani / getGameSpeed())) == 0 &&
                            (curUnit->move == UM_MOVE_HOLD || (curUnit->move == UM_NONE && (curUnit->logic == UL_GUARD || curUnit->logic == UL_GUARD_AREA || curUnit->logic == UL_AMBUSH || curUnit->logic == UL_MOVE_LOCATION)))) {
                            if (curUnitInfo->can_rotate_turret)
                                curUnit->move = (curUnit->move == UM_MOVE_HOLD) ? UM_IDLE_TURRET1_HOLD : UM_IDLE_TURRET1;
                            else
                                curUnit->move = (curUnit->move == UM_MOVE_HOLD) ? UM_IDLE1_HOLD : UM_IDLE1;
                            curUnit->move_aid = curUnitInfo->rotate_speed;
                        }
                    }
                }
            }
        }
    }
    
    for (i=0,curUnitReinforcement=unitReinforcement; i<MAX_REINFORCEMENTS; i++,curUnitReinforcement++) {
        if (curUnitReinforcement->enabled && --curUnitReinforcement->delay <= 0) {
            j = addUnit(curUnitReinforcement->side, curUnitReinforcement->x, curUnitReinforcement->y, curUnitReinforcement->info, 0);
            if (j >= 0) {
                curUnit = unit + j;
                curUnit->armour = curUnitReinforcement->armour;
                if (curUnitReinforcement->x == 0) {
                    curUnit->unit_positioned = RIGHT;
                    curUnit->turret_positioned = RIGHT;
                } else if (curUnitReinforcement->x == environment.width - 1) {
                    curUnit->unit_positioned = LEFT;
                    curUnit->turret_positioned = LEFT;
                } else if (curUnitReinforcement->y == 0) {
                    curUnit->unit_positioned = DOWN;
                    curUnit->turret_positioned = DOWN;
                } else /*if (curUnitReinforcement->y == environment.height - 1)*/ {
                    curUnit->unit_positioned = UP;
                    curUnit->turret_positioned = UP;
                }
//                if (getGameType() == SINGLEPLAYER) {
                    if (curUnitReinforcement->side != FRIENDLY) {
                        curUnit->logic = UL_HUNT;
                        curUnit->logic_aid = 1;
                        curUnit->group = UGAI_HUNT;
                    } else {
                        // (curUnitReinforcement->side == FRIENDLY and as such this unit is used as Reinforcement moving to a target location)
                        curUnit->logic = UL_MOVE_LOCATION;
                        curUnit->logic_aid = curUnitReinforcement->logic_aid;
                        curUnit->group = 0;
                        
                        curUnit->guard_tile   = curUnit->logic_aid;
                        curUnit->retreat_tile = curUnit->logic_aid;
                        
                        /*
                        // guard 1st friendly structure (should be HQ?)
                        int k;
                        for (k=MAX_DIFFERENT_FACTIONS; k<MAX_STRUCTURES_ON_MAP && structure[k].enabled; k++) {
                            if (structure[k].side == FRIENDLY) {
                                curUnit->guard_tile = environment.width * (structure[k].y + structureInfo[structure[k].info].height/2)+
                                                      structure[k].x + structureInfo[structure[k].info].width/2;
                                curUnit->retreat_tile = curUnit->guard_tile;
                                break;
                            }
                        }
                        */
                        
                    }
//                }
            }
            curUnitReinforcement->enabled = 0;
        }
    }
    
    frame++;
    if (frame == 99999) frame = 0;
    
    stopProfilingFunction();
}

void doUnitLogicHealing(int nr, int projectileSourceTile) {
    int unitNrWhichShot = environment.layout[projectileSourceTile].contains_unit;
    if (unitNrWhichShot >= 0 && unitInfo[unit[unitNrWhichShot].info].can_heal_foot) {
        // the shot unit will not have to act upon getting shot.
        // if it reached full health though, the medic shooting should cease fire.
        if (unit[nr].armour >= unitInfo[unit[nr].info].max_armour) {
            unit[unitNrWhichShot].logic = UL_GUARD_RETREAT;
        }
    }
}


void doUnitLogicHit(int nr, int projectileSourceTile) {
    int unitNrWhichShot, structureNrWhichShot;
    int structureCurrentlyShootingAt;
    int i;
    int x, y;
    
    if (unit[nr].logic == UL_NOTHING)
        return;
    
    if (getGameType() == SINGLEPLAYER) {
        if (unit[nr].side != FRIENDLY && unitInfo[unit[nr].info].shoot_range > 0 && unit[nr].logic != UL_ATTACK_UNIT && unit[nr].group != UGAI_KAMIKAZE) {
            unitNrWhichShot = environment.layout[projectileSourceTile].contains_unit;
            structureNrWhichShot = environment.layout[projectileSourceTile].contains_structure;
            if (unitNrWhichShot >= 0 && !unit[unitNrWhichShot].side != !unit[nr].side) {
                unit[nr].logic = UL_ATTACK_UNIT;
                unit[nr].logic_aid = unitNrWhichShot;
                #ifdef REMOVE_ASTAR_PATHFINDING
                unit[nr].hugging_obstacle = 0;
                #endif
            } else if (structureNrWhichShot != -1) {
                if (unit[nr].logic == UL_ATTACK_LOCATION || unit[nr].logic == UL_ATTACK_AREA) {
                    structureCurrentlyShootingAt = environment.layout[unit[nr].logic_aid].contains_structure;
                    if (structureCurrentlyShootingAt < -1)
                        structureCurrentlyShootingAt = environment.layout[unit[nr].logic_aid + (structureCurrentlyShootingAt + 1)].contains_structure;
                    if (structureCurrentlyShootingAt >= MAX_DIFFERENT_FACTIONS && structureInfo[structure[structureCurrentlyShootingAt].info].shoot_range > 0)
                        return;
                }
                if (structureNrWhichShot < -1) structureNrWhichShot = environment.layout[projectileSourceTile + structureNrWhichShot + 1].contains_structure;
                if (!structure[structureNrWhichShot].side != !unit[nr].side) {
                    unit[nr].logic = UL_ATTACK_LOCATION;
                    unit[nr].logic_aid = projectileSourceTile;
                    #ifdef REMOVE_ASTAR_PATHFINDING
                    unit[nr].hugging_obstacle = 0;
                    #endif
                }
            }
        }
    }
    
    if (unit[nr].side == FRIENDLY) {
        unitNrWhichShot = environment.layout[projectileSourceTile].contains_unit;
        if (unitNrWhichShot >= 0) { // was a unit that shot
            
            if (unit[unitNrWhichShot].side != FRIENDLY) {
                x = unit[nr].x; //unit[unitNrWhichShot].x;
                y = unit[nr].y; //unit[unitNrWhichShot].y;
                // check all units in the area to see if they should attack the unit back: when GUARD/GUARD_RETREAT or ATTACK_LOCATION
                for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                    if (unit[i].enabled && unit[i].side == FRIENDLY &&
                        abs(unit[i].y - y) <= MAX_RADIUS_FRIENDLY_UNIT_AID && abs(unit[i].x - x) <= MAX_RADIUS_FRIENDLY_UNIT_AID &&
                        unitInfo[unit[i].info].shoot_range > 0 && !unitInfo[unit[i].info].can_heal_foot) {
                        if (unit[i].logic == UL_GUARD || unit[i].logic == UL_GUARD_RETREAT || unit[i].move == UM_MOVE_HOLD) {
                            unit[i].logic = UL_ATTACK_UNIT;
                            unit[i].logic_aid = unitNrWhichShot;
                            unit[i].group = (unit[i].group & (~UGAI_FRIENDLY_MASK)) | UGAI_GUARD;
                        }
                        if (unit[i].logic == UL_ATTACK_LOCATION && !(unit[i].group & UGAI_ATTACK_FORCED)) {
                            // only change logic to aid the attacked unit if the current target isn't an offensive/shooting structure
                            structureCurrentlyShootingAt = environment.layout[unit[i].logic_aid].contains_structure;
                            if (structureCurrentlyShootingAt < -1)
                                structureCurrentlyShootingAt = environment.layout[unit[i].logic_aid + (structureCurrentlyShootingAt + 1)].contains_structure;
                            if (structureCurrentlyShootingAt >= 0 && structureInfo[structure[structureCurrentlyShootingAt].info].shoot_range <= 0) {
                                unit[i].logic = UL_ATTACK_UNIT;
                                unit[i].logic_aid = unitNrWhichShot;
                            }
                        }
                    }
                }
            }
            
        } else {
            structureNrWhichShot = environment.layout[projectileSourceTile].contains_structure;
            if (structureNrWhichShot < -1)
                structureNrWhichShot = environment.layout[projectileSourceTile + (structureNrWhichShot + 1)].contains_structure;
            if (structureNrWhichShot >= MAX_DIFFERENT_FACTIONS && structure[structureNrWhichShot].side != FRIENDLY && structureInfo[structure[structureNrWhichShot].info].shoot_range > 0) {
                
                // check this unit's mode. if GUARD let it move away in opposite direction.
                //                         if ATTACK, check all units in the area to see if they should attack back

                if (unit[nr].logic == UL_GUARD) { // let it move away in opposite direction. out of range if possible.
                    x = structure[structureNrWhichShot].x - unit[nr].x; // xdiff rather
                    y = structure[structureNrWhichShot].y - unit[nr].y; // ydiff rather
                    if (x < 0 && unit[nr].x < environment.width - 1)
                        unit[nr].guard_tile++;
                    if (x > 0 && unit[nr].x > 0)
                        unit[nr].guard_tile--;
                    if (y < 0 && unit[nr].y < environment.height - 1)
                        unit[nr].guard_tile += environment.width;
                    if (y > 0 && unit[nr].y > 0)
                        unit[nr].guard_tile -= environment.width;
                    if (unit[nr].guard_tile != TILE_FROM_XY(unit[nr].x, unit[nr].y)) // guard_tile changed. move away.
                        unit[nr].logic = UL_GUARD_RETREAT;
                    else { // can't move away in this direction. let's freak out and attack.
                        unit[nr].logic = UL_ATTACK_LOCATION;
                        unit[nr].logic_aid = projectileSourceTile;
                        unit[nr].group = (unit[nr].group & (~UGAI_FRIENDLY_MASK)) | UGAI_GUARD;
                    }
                }
                
                if (unit[nr].logic == UL_ATTACK_UNIT || unit[nr].logic == UL_ATTACK_LOCATION || unit[nr].move == UM_MOVE_HOLD) {
                    x = unit[nr].x; //structure[structureNrWhichShot].x;
                    y = unit[nr].y; //structure[structureNrWhichShot].y;
                    // check all units in the area to see if they should attack the structure back: when GUARD/GUARD_RETREAT or ATTACK_LOCATION
                    for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                        if (unit[i].enabled && unit[i].side == FRIENDLY &&
                            abs(unit[i].y - y) <= MAX_RADIUS_FRIENDLY_UNIT_AID && abs(unit[i].x - x) <= MAX_RADIUS_FRIENDLY_UNIT_AID &&
                            unitInfo[unit[i].info].shoot_range > 0 && !unitInfo[unit[i].info].can_heal_foot) {
                            if (unit[i].logic == UL_GUARD || unit[i].logic == UL_GUARD_RETREAT || (unit[nr].move == UM_MOVE_HOLD && unit[i].logic != UL_ATTACK_LOCATION)) {
                                unit[i].logic = UL_ATTACK_LOCATION;
                                unit[i].logic_aid = projectileSourceTile;
                                unit[i].group = (unit[i].group & (~UGAI_FRIENDLY_MASK)) | UGAI_GUARD;
                            }
                            if (unit[i].logic == UL_ATTACK_LOCATION && !(unit[i].group & UGAI_ATTACK_FORCED)) {
                                // only change logic to aid the attacked unit if the current target isn't an offensive/shooting structure
                                structureCurrentlyShootingAt = environment.layout[unit[i].logic_aid].contains_structure;
                                if (structureCurrentlyShootingAt < -1)
                                    structureCurrentlyShootingAt = environment.layout[unit[i].logic_aid + (structureCurrentlyShootingAt + 1)].contains_structure;
                                if (structureCurrentlyShootingAt >= MAX_DIFFERENT_FACTIONS && structureInfo[structure[structureCurrentlyShootingAt].info].shoot_range <= 0) {
                                    unit[i].logic = UL_ATTACK_LOCATION;
                                    unit[i].logic_aid = projectileSourceTile;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void doUnitsSelectAll() {
    int i;
    struct UnitInfo *curUnitInfo;
    struct Unit *curUnit = &unit[0];
    
    // select all FRIENDLY units, other than collectors. deselect ENEMY units too.
    for (i=0; i<MAX_UNITS_ON_MAP; i++,curUnit++) {
        if (curUnit->enabled) {
            if (curUnit->side==FRIENDLY) {
                curUnitInfo = unitInfo + curUnit->info;
                curUnit->selected=!curUnitInfo->can_collect_ore;
            } else {
                curUnit->selected=0;  // enemy unit
            }
        }
    }

    // deselect every structure.
    for (i=0; i<MAX_STRUCTURES_ON_MAP; i++)
        structure[i].selected = 0;
}

inline struct UnitReinforcement *getUnitsReinforcements() {
    return unitReinforcement;
}

int getUnitsSaveSize(void) {
    return sizeof(unit);
}

int getUnitsReinforcementsSaveSize(void) {
    return sizeof(unitReinforcement);
}


int getUnitsSaveData(void *dest, int max_size) {
    int size=sizeof(unit);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&unit,size);
    return (size);
}

int getUnitsReinforcementsSaveData(void *dest, int max_size) {
    int size=sizeof(unitReinforcement);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&unitReinforcement,size);
    return (size);
}
