// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "structures.h"

#include "fileio.h"

#include "playscreen.h"
#include "infoscreen.h"
#include "view.h"
#include "sprites.h"
#include "game.h"
#include "environment.h"
#include "overlay.h"
#include "info.h"
#include "factions.h"
#include "explosions.h"
#include "units.h"
#include "radar.h"
#include "soundeffects.h"
#include "rumble.h"


struct StructureInfo structureInfo[MAX_DIFFERENT_STRUCTURES];
struct Structure structure[MAX_STRUCTURES_ON_MAP];

static int structure_smoke_graphics_offset = 0;
static int structure_rally_graphics_offset;
static int base_structure_selected;
static int base_structure_flag;


static int flagAnimationTimer = 0;
static int structureAnimationTimer = 1;
static int structureFoundationRequired;

static int structureMultiplayerIdCounter;




void initStructures() {
    char oneline[256];
    int amountOfStructures = 0;
    int i, j;
    FILE *fp;

    // get the names of all available structures
    fp = openFile("structures.ini", FS_PROJECT_FILE);
    readstr(fp, oneline);
    sscanf(oneline, "AMOUNT-OF-STRUCTURES %i", &amountOfStructures);
    if (amountOfStructures > MAX_DIFFERENT_STRUCTURES)
        errorSI("Too much types of structures exist. Limit is:", MAX_DIFFERENT_STRUCTURES);
    for (i=0; i<amountOfStructures; i++) {
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 255);
        strcpy(structureInfo[i].name, oneline);
        structureInfo[i].enabled = 1;
    }
    closeFile(fp);
    
    // get information on each and every available structure
    for (i=0; i<amountOfStructures; i++) {
        fp = openFile(structureInfo[i].name, FS_STRUCTURES_INFO);
        
        // GENERAL section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[GENERAL]", strlen("[GENERAL]")));
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 255);
        strcpy(structureInfo[i].description, oneline + strlen("Description="));
        
        // CONSTRUCTING section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[CONSTRUCTING]", strlen("[CONSTRUCTING]")));
        
        readstr(fp, oneline);
        sscanf(oneline, "Cost=%i", &structureInfo[i].build_cost);
        readstr(fp, oneline);
        sscanf(oneline, "Speed=%i", &structureInfo[i].original_build_time);
        structureInfo[i].original_build_time *= FPS;
        
        structureInfo[i].class_mask = -1;
        readstr(fp, oneline);
        sscanf(oneline, "Class=%i", &structureInfo[i].class_mask);
        if (structureInfo[i].class_mask != -1)
          structureInfo[i].class_mask = BIT(structureInfo[i].class_mask);
        else
          structureInfo[i].class_mask = BIT(0); // default, if "Class=" is missing
        
        // REPAIRING section
        while (strncmp(oneline, "[REPAIRING]", strlen("[REPAIRING]"))) {
            readstr(fp, oneline);
        }
        readstr(fp, oneline);
        sscanf(oneline, "Cost=%i", &structureInfo[i].repair_cost);
        readstr(fp, oneline);
        sscanf(oneline, "Speed=%i", &structureInfo[i].original_repair_time);
        structureInfo[i].original_repair_time *= FPS;
        
        // MEASUREMENTS section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[MEASUREMENTS]", strlen("[MEASUREMENTS]")));
        readstr(fp, oneline);
        sscanf(oneline, "Width=%i", &structureInfo[i].width);
        readstr(fp, oneline);
        sscanf(oneline, "Height=%i", &structureInfo[i].height);
        
        // ANIMATION section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[ANIMATION]", strlen("[ANIMATION]")));
        readstr(fp, oneline);
        sscanf(oneline, "Idle=%i", &structureInfo[i].idle_ani);
        readstr(fp, oneline);
        sscanf(oneline, "Active=%i", &structureInfo[i].active_ani);
        
        // FLAGS section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[FLAGS]", strlen("[FLAGS]")));
        readstr(fp, oneline);
        sscanf(oneline, "Amount=%i", &structureInfo[i].flag_amount);
        for (j=0; j<structureInfo[i].flag_amount; j++) {
            readstr(fp, oneline);
            sscanf(oneline, "Position=%i,%i", &structureInfo[i].flag[j].x, &structureInfo[i].flag[j].y);
        }
        
        // VIEW section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[VIEW]", strlen("[VIEW]")));
        readstr(fp, oneline);
        sscanf(oneline, "Range=%i", &structureInfo[i].view_range);
        if (structureInfo[i].view_range < MAX_BUILDING_DISTANCE)
            structureInfo[i].view_range = MAX_BUILDING_DISTANCE;
        
        // WEAPON section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[WEAPON]", strlen("[WEAPON]")));
        readstr(fp, oneline);
        sscanf(oneline, "Range=%i", &structureInfo[i].shoot_range);
        if (structureInfo[i].shoot_range != 0) {
            readstr(fp, oneline);
            sscanf(oneline, "Reload-Time=%i", &structureInfo[i].original_reload_time);
            structureInfo[i].original_reload_time = (structureInfo[i].original_reload_time * FPS) / 4;
            readstr(fp, oneline);
            sscanf(oneline, "Double-Shot=%i", &structureInfo[i].double_shot);
            readstr(fp, oneline);
            structureInfo[i].projectile_info = -1;
            for (j=0; j<MAX_DIFFERENT_PROJECTILES && projectileInfo[j].enabled; j++) {
                if (!strncmp(oneline + strlen("Projectile="), projectileInfo[j].name, strlen(projectileInfo[j].name))) {
                    structureInfo[i].projectile_info = j;
                    break;
                }
            }
        }
        
        // ARMOUR section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[ARMOUR]", strlen("[ARMOUR]")));
        readstr(fp, oneline);
        sscanf(oneline, "Armour=%i", &structureInfo[i].max_armour);
        if (structureInfo[i].max_armour < 0)
            structureInfo[i].max_armour = INFINITE_ARMOUR;
        
        // EXPLOSION section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[EXPLOSION]", strlen("[EXPLOSION]")));
        readstr(fp, oneline);
        structureInfo[i].created_explosion_info = -1;
        for (j=0; j<MAX_DIFFERENT_EXPLOSIONS && explosionInfo[j].enabled; j++) {
            if (!strncmp(oneline + strlen("Created="), explosionInfo[j].name, strlen(explosionInfo[j].name))) {
                structureInfo[i].created_explosion_info = j;
                break;
            }
        }
        readstr(fp, oneline);
        structureInfo[i].destroyed_explosion_info = -1;
        for (j=0; j<MAX_DIFFERENT_EXPLOSIONS && explosionInfo[j].enabled; j++) {
            if (!strncmp(oneline + strlen("Destroyed="), explosionInfo[j].name, strlen(explosionInfo[j].name))) {
                structureInfo[i].destroyed_explosion_info = j;
                break;
            }
        }
        
        // MOVEMENT section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[MOVEMENT]", strlen("[MOVEMENT]")));
        readstr(fp, oneline);
        sscanf(oneline, "RotateSpeed=%i", &structureInfo[i].original_rotate_speed);
        structureInfo[i].original_rotate_speed = (structureInfo[i].original_rotate_speed * FPS) / 4;
        
        // POWER section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[POWER]", strlen("[POWER]")));
        readstr(fp, oneline);
        sscanf(oneline, "Consuming=%i", &structureInfo[i].power_consuming);
        readstr(fp, oneline);
        sscanf(oneline, "Generating=%i", &structureInfo[i].power_generating);
        
        // CREDITS section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[CREDITS]", strlen("[CREDITS]")));
        readstr(fp, oneline);
        sscanf(oneline, "Storage=%i", &structureInfo[i].ore_storage);
        
        // RELEASING section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[RELEASING]", strlen("[RELEASING]")));
        readstr(fp, oneline);
        sscanf(oneline, "Tile=%i", &structureInfo[i].release_tile);
        structureInfo[i].release_tile--;
        
        // MISC section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[MISC]", strlen("[MISC]")));
        readstr(fp, oneline);
        sscanf(oneline, "Foundation=%i", &structureInfo[i].foundation);
        readstr(fp, oneline);
        sscanf(oneline, "Barrier=%i", &structureInfo[i].barrier);
        readstr(fp, oneline);
        sscanf(oneline, "Can-Extract-Ore=%i", &structureInfo[i].can_extract_ore);
        readstr(fp, oneline);
        sscanf(oneline, "Can-Repair-Unit=%i", &structureInfo[i].can_repair_unit);
        readstr(fp, oneline);
        sscanf(oneline, "Can-Rotate-Turret=%i", &structureInfo[i].can_rotate_turret);
        readstr(fp, oneline);
        sscanf(oneline, "No-Sound-On-Destruction=%i", &structureInfo[i].no_sound_on_destruction);
        readstr(fp, oneline);
        sscanf(oneline, "No-Overlay-On-Destruction=%i", &structureInfo[i].no_overlay_on_destruction);
        closeFile(fp);
    }
    for (i=amountOfStructures; i<MAX_DIFFERENT_STRUCTURES; i++) // setting all unused ones to disabled
        structureInfo[i].enabled = 0;
    
    structureFoundationRequired = 0;
    for (i=0; i<amountOfStructures; i++) {
        if (structureInfo[i].foundation) {
            structureFoundationRequired = 1;
            
            for (; i<MAX_DIFFERENT_STRUCTURES; i++) {
                if (structureInfo[i].enabled && structureInfo[i].foundation &&
                    structureInfo[i].width == 1 && structureInfo[i].height == 1) { // found 1x1 foundation
                    for (j=0; j<MAX_DIFFERENT_FACTIONS; j++) {
                        structure[j].enabled = 1;
                        structure[j].info = i;
                        structure[j].side = j;
                        structure[j].armour = 1;
                        structure[j].group = 0;
                        structure[j].selected = 0;
                    }
                    break;
                }
            }
            if (i == MAX_DIFFERENT_STRUCTURES)
                error("Could not find the obligatory 1x1 foundation.", "");
            break;
        }
    }
}

void initStructuresSpeed() {
    int i;
    int gameSpeed = getGameSpeed();
    
    for (i=0; i<MAX_DIFFERENT_STRUCTURES && structureInfo[i].enabled; i++) {
        structureInfo[i].build_time = (2*structureInfo[i].original_build_time) / gameSpeed;
        structureInfo[i].repair_time = (2*structureInfo[i].original_repair_time) / gameSpeed;
        structureInfo[i].rotate_speed = (2*structureInfo[i].original_rotate_speed) / gameSpeed;
        structureInfo[i].reload_time = (2*structureInfo[i].original_reload_time) / gameSpeed;
    }
}

void initStructuresWithScenario() {
    char oneline[256];
    char *positionBehindSide;
    int amountOfStructures = MAX_DIFFERENT_FACTIONS; // already have a 1x1 foundation structure for each Faction
    FILE *fp;
    int i,j,k,l;
    
    structureMultiplayerIdCounter = 0;
    
    for (i=0; i<MAX_DIFFERENT_FACTIONS; i++) {
        setStructureCount(i, 0);
        setStructureDeaths(i, 0);
    }
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    
    // STRUCTURES section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[STRUCTURES]", strlen("[STRUCTURES]")));
    while (1) {
        readstr(fp, oneline);
        if (!strncmp(oneline, "Friendly,", strlen("Friendly,"))) {
            structure[amountOfStructures].side = FRIENDLY;
            positionBehindSide = oneline + strlen("Friendly,");
        } else if (!strncmp(oneline, "Enemy", strlen("Enemy"))) {
            sscanf(oneline, "Enemy%i,", &i);
            structure[amountOfStructures].side = i;
            positionBehindSide = oneline + strlen("Enemy#,");
        } else
            break;
        
        for (i=0; i<MAX_DIFFERENT_STRUCTURES && structureInfo[i].enabled; i++) {
            if (!strncmp(positionBehindSide, structureInfo[i].name, strlen(structureInfo[i].name)) && positionBehindSide[strlen(structureInfo[i].name)] == ',') {
                structure[amountOfStructures].info = i;
                break;
            }
        }
        if (i == MAX_DIFFERENT_STRUCTURES)
            error("In the scenario an unknown structure was mentioned", oneline);
        
        sscanf(positionBehindSide + strlen(structureInfo[i].name), ",%i,%i,%i", &k, &structure[amountOfStructures].x, &structure[amountOfStructures].y);
        structure[amountOfStructures].armour = (k < 0) ? (INFINITE_ARMOUR) : ((structureInfo[i].max_armour * k) / 100);
        amountOfStructures++;
    }
    closeFile(fp);
    
    for (i=0; i<getAmountOfSides(); i++) {
        setOreStorage(i, 0);
        setPowerGeneration(i, 0);
        setPowerConsumation(i, 0);
    }
    
    for (j=0; j<environment.width*environment.height; j++)
        environment.layout[j].contains_structure = -1;
    for (j=MAX_DIFFERENT_FACTIONS; j<MAX_STRUCTURES_ON_MAP; j++) {
        structure[j].enabled = 0;
        structure[j].group = 0;
    }
    for (j=MAX_DIFFERENT_FACTIONS; j<amountOfStructures; j++) {
        k = structure[j].armour;
        if (getGameType() == MULTIPLAYER_CLIENT)
            l = addStructure(!structure[j].side, structure[j].x, structure[j].y, structure[j].info, 1);
        else
            l = addStructure(structure[j].side, structure[j].x, structure[j].y, structure[j].info, 1);
        structure[l].armour = k;
    }
    
    // set B/Down hotkey to friendly base
    i = 0;
    for (j=MAX_DIFFERENT_FACTIONS; j<MAX_STRUCTURES_ON_MAP; j++) {
        if (structure[j].enabled && structure[j].side == FRIENDLY) {
            if (i == 0 || structure[j].info < structure[i].info)
                i = j;
        }
    }
    if (i)
        structure[i].group = (getPrimaryHand() == LEFT_HANDED) ? KEY_DOWN : KEY_B;
    
    initStructuresSpeed();
}


int getStructureNameInfo(char *buffer) {
    int i, len;
    
    len = strlen("None");
    if (!strncmp(buffer, "None", len) && (buffer[len]==0 || buffer[len]==','))
        return -1;
    
    for (i=0; i<MAX_DIFFERENT_STRUCTURES && structureInfo[i].enabled; i++) {
        len = strlen(structureInfo[i].name);
        if (!strncmp(buffer, structureInfo[i].name, len) && (buffer[len]==0 || buffer[len]==','))
            return i;
    }
    error("Non existant structure name encountered:", buffer);
    return -1;
}


int foundationRequired() {
    return structureFoundationRequired;
}





void adjustBarrierType(int x, int y) {
    int connected = 0; // LEFT, RIGHT, UP, DOWN
    enum Side side;
    int info;
    int structureNr;
    
    structureNr = environment.layout[TILE_FROM_XY(x,y)].contains_structure;
    if (structureNr < 0) return;
    side = structure[structureNr].side;
    info = structure[structureNr].info;
    
    if (x > 0) {
        structureNr = environment.layout[TILE_FROM_XY(x-1, y)].contains_structure;
        if (structureNr >= 0 && structure[structureNr].side == side && structure[structureNr].info == info)
            connected |= BIT(0);
    }
    if (x < environment.width - 1) {
        structureNr = environment.layout[TILE_FROM_XY(x+1, y)].contains_structure;
        if (structureNr >= 0 && structure[structureNr].side == side && structure[structureNr].info == info)
            connected |= BIT(1);
    }
    if (y > 0) {
        structureNr = environment.layout[TILE_FROM_XY(x, y-1)].contains_structure;
        if (structureNr >= 0 && structure[structureNr].side == side && structure[structureNr].info == info)
            connected |= BIT(2);
    }
    if (y < environment.height - 1) {
        structureNr = environment.layout[TILE_FROM_XY(x, y+1)].contains_structure;
        if (structureNr >= 0 && structure[structureNr].side == side && structure[structureNr].info == info)
            connected |= BIT(3);
    }
    
    structureNr = environment.layout[TILE_FROM_XY(x,y)].contains_structure;
    if (connected == 0 || connected == 4) // no connections or only up connected
        structure[structureNr].logic_aid = 0;
    else if (connected == 15) // all connections
        structure[structureNr].logic_aid = 11;
    else if (connected == 11) // left, right, down
        structure[structureNr].logic_aid = 10;
    else if (connected == 7) // left, right, up
        structure[structureNr].logic_aid = 9;
    else if (connected == 14) // right, up, down
        structure[structureNr].logic_aid = 8;
    else if (connected == 13) // left, up, down
        structure[structureNr].logic_aid = 7;
    else if (connected == 10) // right, down
        structure[structureNr].logic_aid = 6;
    else if (connected == 9) // left, down
        structure[structureNr].logic_aid = 5;
    else if (connected == 12 || connected == 8) // up, down connected or only down connected
        structure[structureNr].logic_aid = 4;
    else if (connected == 6)
        structure[structureNr].logic_aid = 3;
    else if (connected == 5)
        structure[structureNr].logic_aid = 2;
    else
        structure[structureNr].logic_aid = 1;
}


int addStructure(enum Side side, int x, int y, int info, int forced) {
    int tile = TILE_FROM_XY(x,y);
    struct EnvironmentLayout *envLayout;
    int structure_t;
    struct Structure *curStructure;
    struct StructureInfo *curStructureInfo = structureInfo + info;
    int i, j=-1, k, l=0;
    
    if (!forced || getGameState() == INGAME) {
        /* can place structure here? */
        for (i=0; i<curStructureInfo->height; i++) {
            for (j=0; j<curStructureInfo->width; j++) {
                envLayout = &environment.layout[tile + TILE_FROM_XY(j,i)];
                if (envLayout->contains_unit != -1 || envLayout->graphics < ROCK || envLayout->graphics > ROCKCUSTOM32) {
                    j = -1;
                    break;
                }
                structure_t = envLayout->contains_structure;
                if (structure_t < -1)
                    structure_t = environment.layout[tile + TILE_FROM_XY(j + (structure_t + 1), i)].contains_structure;
                if (structure_t > -1) {
                    if (curStructureInfo->foundation || !structureInfo[structure[structure_t].info].foundation) {
                        j = -1;
                        break;
                    }
                } else { // no structure at this specific tile: is underground needed, and if so, is the buildable structure underground?
                    if (structureFoundationRequired && !curStructureInfo->foundation && !forced) {
                        j = -1;
                        break;
                    }
                }
                if (!forced && getDistanceToNearestStructure(side, x+j, y+i) > MAX_BUILDING_DISTANCE) {
                    j = -1;
                    break;
                }
            }
            if (j== -1)
                break;
        }
        if (j == -1 && (!curStructureInfo->foundation || (curStructureInfo->width == 1 && curStructureInfo->height == 1)))
            return -1;
    }
    
    /* can place structure here! */
    if (curStructureInfo->foundation && (curStructureInfo->width > 1 || curStructureInfo->height > 1)) {
        /* find the obligatory 1x1 foundation */
        for (i=0; i<MAX_DIFFERENT_STRUCTURES && structureInfo[i].enabled; i++) {
            if (structureInfo[i].foundation && structureInfo[i].width == 1 && structureInfo[i].height == 1) {
                l = -1;
                for (j=0; j<curStructureInfo->height; j++) {
                    for (k=0; k<curStructureInfo->width; k++) {
                        l = max(l, addStructure(side, x+k, y+j, i, 0));
                    }
                }
                if (l > -1) {
                    if (!forced)
                        playSoundeffect(SE_PLACE_STRUCTURE);
                    if (getGameState() == INGAME)
                        createBarStructures(); // automaticly creates barBuildables
                }
                return l;
            }
        }
        error("Could not find obligatory 1x1 foundation structure", "");
    }
    
    if (curStructureInfo->foundation)
        j = side;
    else {
        for (j=MAX_DIFFERENT_FACTIONS; j<MAX_STRUCTURES_ON_MAP && (structure[j].enabled || (structure[j].group & AWAITING_REPLACEMENT)); j++);
        if (j == MAX_STRUCTURES_ON_MAP)
            return -1;
    }
    curStructure = structure + j;
    
    curStructure->enabled = 1;
    curStructure->side = side;
    curStructure->group = 0;
    curStructure->selected = 0;
    curStructure->info = info;
    curStructure->x = x;
    curStructure->y = y;
    curStructure->turret_positioned = UP;
    curStructure->move = SM_NONE;
    curStructure->logic_aid = 0;
    curStructure->contains_unit = -1;
    curStructure->armour = curStructureInfo->max_armour;
    curStructure->smoke_time = 0;
    curStructure->multiplayer_id = structureMultiplayerIdCounter++;
    
    curStructure->reload_time = curStructureInfo->reload_time;
    curStructure->misfire_time = 0;
    
    if (curStructureInfo->double_shot && (curStructure->reload_time - (SHOOT_ANIMATION_DURATION + 1) >= DOUBLE_SHOT_AT)) // try to make sure it doesn't look like it's shooting from the get-go
        curStructure->reload_time -= (SHOOT_ANIMATION_DURATION + 1);
    
    curStructure->rally_tile = -1;
    if (curStructureInfo->release_tile >= 0 /*&& !curStructureInfo->can_extract_ore*/)
        curStructure->rally_tile = TILE_FROM_XY(x + curStructureInfo->release_tile, y + curStructureInfo->height - (1 * (y + curStructureInfo->height >= environment.height)));
    
    if (curStructureInfo->shoot_range > 0) curStructure->logic = SL_GUARD;
    else curStructure->logic = SL_NONE;
    
    for (i=0; i<curStructureInfo->height; i++) {
        for (k=0; k<curStructureInfo->width; k++) {
            // first disable the structure (read: foundation) on this tile if that was required
            if (!forced && structureFoundationRequired)
                structure[environment.layout[TILE_FROM_XY(x+k, y+i)].contains_structure].enabled = 0;
            // now place the new structure tile on it
            if (i==0 && k==0) {
                environment.layout[TILE_FROM_XY(x,y)].contains_structure = j;
                setTileDirtyRadarDirtyBitmap(TILE_FROM_XY(x,y));
            } else {
                environment.layout[TILE_FROM_XY(x+k, y+i)].contains_structure = -(TILE_FROM_XY(k,i)) -1;
                setTileDirtyRadarDirtyBitmap(TILE_FROM_XY(x+k, y+i));
            }
            // set tile's traversability to be untraversable due to the newly created structure
            if (!curStructureInfo->foundation)
                environment.layout[TILE_FROM_XY(x+k, y+i)].traversability = UNTRAVERSABLE;
            // also cause a creation-explosion
//          if (!forced)
                addExplosion((x + k) * 16 + 8, (y + i) * 16 + 8, curStructureInfo->created_explosion_info, (i & 1) * (FPS/5) + (k&1 * (FPS/10)));
            if (!forced && x+k >= getViewCurrentX() && x+k < getViewCurrentX() + HORIZONTAL_WIDTH &&
                            y+i >= getViewCurrentY() && y+i < getViewCurrentY() + HORIZONTAL_HEIGHT)
                addRumble(8, FPS/2);
        }
    }
    setOreStorage(side, getOreStorage(side) + curStructureInfo->ore_storage);
    setPowerGeneration(side, getPowerGeneration(side) + curStructureInfo->power_generating);
    setPowerConsumation(side, getPowerConsumation(side) + curStructureInfo->power_consuming);
    
    curStructure->primary = 1;
    for (i=0; i<MAX_STRUCTURES_ON_MAP; i++) {
        if (structure[i].enabled && i != j && structure[i].side == side && structure[i].info == info) {
            curStructure->primary = 0;
            break;
        }
    }
    
    if (curStructureInfo->can_extract_ore && (forced || getUnitCount(side) < getUnitLimit(side))) {
        for (i=0; i<MAX_DIFFERENT_UNITS && unitInfo[i].enabled; i++) {
            if (unitInfo[i].can_collect_ore) {
                i = addUnit(side, curStructure->x + curStructureInfo->release_tile, curStructure->y + curStructureInfo->height - 1, i, 1);
                if (i >= 0) {
                    environment.layout[TILE_FROM_XY(unit[i].x, unit[i].y)].contains_unit = -1;
                    curStructure->contains_unit = i;
                    if (side == FRIENDLY)
                        playSoundeffect(SE_MINER_DEPLOYED);
                }
                break;
            }
        }
    }
    
    if (curStructureInfo->barrier) {
        adjustBarrierType(x, y);
        if (x > 0 && environment.layout[tile - 1].contains_structure >= 0 && structure[environment.layout[tile - 1].contains_structure].info == info)
            adjustBarrierType(x-1, y);
        if (x < environment.width - 1 && environment.layout[tile + 1].contains_structure >= 0 && structure[environment.layout[tile + 1].contains_structure].info == info)
            adjustBarrierType(x+1, y);
        if (y > 0 && environment.layout[tile - environment.width].contains_structure >= 0 && structure[environment.layout[tile - environment.width].contains_structure].info == info)
            adjustBarrierType(x, y-1);
        if (y < environment.height - 1 && environment.layout[tile + environment.width].contains_structure >= 0 && structure[environment.layout[tile + environment.width].contains_structure].info == info)
            adjustBarrierType(x, y+1);
    }
    
    if (!curStructureInfo->foundation && !curStructureInfo->barrier)
        setStructureCount(side, getStructureCount(side) + 1);
    
    if (!forced && x >= getViewCurrentX() && x < getViewCurrentX() + HORIZONTAL_WIDTH && y >= getViewCurrentY() && y < getViewCurrentY() + HORIZONTAL_HEIGHT)
        playSoundeffect(SE_PLACE_STRUCTURE);
    
    if (side == FRIENDLY && !curStructureInfo->foundation && !curStructureInfo->barrier)
        activateEntityView(x, y, curStructureInfo->width, curStructureInfo->height, curStructureInfo->view_range);
    
    if (getGameState() == INGAME)
        createBarStructures(); // automaticly creates barBuildables
    return j;
}

inline int structureOnTile(int tile, enum Side side) {
    // excluding foundation and barrier here, as this function is used only to check where structures may be placed
    int result = environment.layout[tile].contains_structure;
    if (result == -1)
        return -1;
    if (result < -1)
        result = environment.layout[tile + (result + 1)].contains_structure;
    if (result >= 0 && structure[result].side == side && !structureInfo[structure[result].info].foundation && !structureInfo[structure[result].info].barrier)
        return result;
    return -1;
}

int getDistanceToNearestStructure(enum Side side, int x, int y) {
    // basicly using Manhattan distance for ease of use, and excluding foundation and barrier
    int maxwh;
    int j, k;
    int ypj_x, ymj_x;
    int xpj_st_envwidth, xmj_lt_zero, ypj_st_envheight, ymj_lt_zero;
    int xpk_st_envwidth, xmk_lt_zero, ypk_st_envheight, ymk_lt_zero;
    
    //maxwh = max(environment.width, environment.height); // <-- actual value
    maxwh = MAX_BUILDING_DISTANCE + 1; // <-- cheat, as I know it'll only be used to check for the allowing of placing buildings
    
    for (j=1; j<maxwh; j++) {
        
        ypj_x = TILE_FROM_XY(x, y+j);
        ymj_x = TILE_FROM_XY(x, y-j);
        
        if ((y+j < environment.height && structureOnTile(ypj_x, side) >= 0) ||                 // bottom column middle tile
            (x+j < environment.width && structureOnTile(TILE_FROM_XY(x+j, y), side) >= 0) ||   // right column middle tile
            (x-j >= 0 && structureOnTile(TILE_FROM_XY(x-j, y), side) >= 0) ||                   // left column middle tile
            (y-j >= 0 && structureOnTile(ymj_x, side) >= 0))                                     // top row middle tile
            return j;
          
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
                ((xpk_st_envwidth && structureOnTile(ypj_x + k, side) >= 0) ||
                 (xmk_lt_zero && structureOnTile(ypj_x - k, side) >= 0))) ||
               (xpj_st_envwidth && // right column
                ((ypk_st_envheight && structureOnTile(TILE_FROM_XY(x+j, y+k), side) >= 0) ||
                 (ymk_lt_zero && structureOnTile(TILE_FROM_XY(x+j, y-k), side) >= 0))) ||
               (xmj_lt_zero && // left column
                ((ypk_st_envheight && structureOnTile(TILE_FROM_XY(x-j, y+k), side) >= 0) ||
                 (ymk_lt_zero && structureOnTile(TILE_FROM_XY(x-j, y-k), side) >= 0))) ||
               (ymj_lt_zero && // top row
                ((xpk_st_envwidth && structureOnTile(ymj_x + k, side) >= 0) ||
                 (xmk_lt_zero && structureOnTile(ymj_x - k, side) >= 0))))
                    return (j + k);
        }
        
        if ((y+j < environment.height && // bottom row outer tiles (the corner tiles)
            ((x+j < environment.width && structureOnTile(ypj_x + j, side) >= 0) ||
             (x-j >= 0 && structureOnTile(ypj_x - j, side) >= 0))) ||
           (y-j >= 0 && // top row outer tiles (the corner tiles)
            ((x+j < environment.width && structureOnTile(ymj_x + j, side) >= 0) ||
             (x-j >= 0 && structureOnTile(ymj_x - j, side) >= 0))))
                return (j + j);
    }
    
    return MAX_BUILDING_DISTANCE + 1;
}


void drawStructures() {
    int i, j, k;
    int baseTile;
    int mapTile;
    struct Structure *curStructure;
    struct StructureInfo *curStructureInfo;
    int x_aid, y_aid;
    struct Structure *structureSelected = 0;
    int healthPixels, healthBase;
    int hflip;
    int x = getViewCurrentX();
    int y = getViewCurrentY();
    
    startProfilingFunction("drawStructures");
    
    mapTile = TILE_FROM_XY(x,y); // tile to start drawing
    
    for (i=0; i<HORIZONTAL_HEIGHT; i++) {
        for (j=0; j<HORIZONTAL_WIDTH; j++) {
            hflip = 0;
            baseTile = environment.layout[mapTile].contains_structure;
            if (baseTile != -1) { // there exists a piece of structure here
                if (baseTile < -1) {
                    if (structureSelected == 0) {
                        structureSelected = &structure[environment.layout[mapTile + baseTile + 1].contains_structure];
                        if (!structureSelected->selected)
                            structureSelected = 0;
                    }
                    curStructure = structure + environment.layout[mapTile + baseTile + 1].contains_structure;
                    curStructureInfo = structureInfo + curStructure->info;
                    x_aid = X_FROM_TILE(-baseTile - 1);
                    y_aid = Y_FROM_TILE(-baseTile - 1);
                    
                    if (curStructureInfo->can_extract_ore && curStructure->contains_unit >= 0 &&
                        x_aid == curStructureInfo->width-1 && y_aid == curStructureInfo->height-1)
                        drawUnitWithShift(curStructure->contains_unit, getExplosionShiftX(),getExplosionShiftY());
                        
                    if (curStructure->smoke_time) {
                        if (curStructure->armour < curStructureInfo->max_armour / 4) { // smoke points 2 and 3 are added
                            if ((x_aid == curStructureInfo->width / 3 && y_aid == curStructureInfo->height - 1) ||
                                (x_aid == curStructureInfo->width - 1 && y_aid == 0 && curStructureInfo->width >= 3))
                                setSpritePlayScreen(ACTION_BAR_SIZE * 16 + i*16 - 10 + getExplosionShiftY(), ATTR0_SQUARE,
                                                    j*16 - 2 + getExplosionShiftX(), SPRITE_SIZE_16, 0, 0,
                                                    2,  PS_SPRITES_PAL_SMOKE, structure_smoke_graphics_offset + (((curStructure->smoke_time + x_aid * (FPS/2) + y_aid * (2*FPS)) / STRUCTURE_SINGLE_SMOKE_DURATION) % 3));
                        }
                        if ((curStructureInfo->width >= 3 || curStructureInfo->height >= 3) &&
                            x_aid == curStructureInfo->width / 3 && y_aid == curStructureInfo->height / 3) // smoke point 1
                            setSpritePlayScreen(ACTION_BAR_SIZE * 16 + i*16 - 6 + getExplosionShiftY(), ATTR0_SQUARE,
                                                j*16 + 5 + getExplosionShiftX(), SPRITE_SIZE_16, 0, 0,
                                                2,  PS_SPRITES_PAL_SMOKE, structure_smoke_graphics_offset + (((curStructure->smoke_time + x_aid * (FPS/2) + y_aid * (2*FPS)) / STRUCTURE_SINGLE_SMOKE_DURATION) % 3));
                    }
                    baseTile = curStructureInfo->graphics_offset + (y_aid * curStructureInfo->width + x_aid) * 4;
                    x_aid *= 16;
                    y_aid *= 16;
                    for (k=0; k<curStructureInfo->flag_amount; k++) {
                        if (curStructureInfo->flag[k].x - x_aid >= 0 && curStructureInfo->flag[k].x - x_aid < 16 && curStructureInfo->flag[k].y - y_aid >= 0 && curStructureInfo->flag[k].y - y_aid < 16)
                            setSpritePlayScreen((ACTION_BAR_SIZE * 16 + i*16 + (curStructureInfo->flag[k].y % 16) - 8) + getExplosionShiftY(), ATTR0_SQUARE,
                                                (j*16 + (curStructureInfo->flag[k].x % 16) - 7) + getExplosionShiftX(), SPRITE_SIZE_8, 0, 0,
                                                2,  PS_SPRITES_PAL_FACTIONS + getFaction(curStructure->side), base_structure_flag + ((curStructure->x + curStructureInfo->flag[k].x + curStructure->y + curStructureInfo->flag[k].y + (flagAnimationTimer / (FPS/2))) & 1));
                    }
                } else {
                    curStructure = structure + baseTile;
                    if (curStructure->selected)
                        structureSelected = curStructure;
                    curStructureInfo = structureInfo + curStructure->info;
                    
                    if (curStructureInfo->can_extract_ore && curStructure->contains_unit >= 0 &&
                        curStructureInfo->width == 1 && curStructureInfo->height == 1)
                        drawUnitWithShift(curStructure->contains_unit, getExplosionShiftX(),getExplosionShiftY());
                    
                    if (curStructure->smoke_time && curStructureInfo->width < 3 && curStructureInfo->height < 3) {
                        setSpritePlayScreen(ACTION_BAR_SIZE * 16 + i*16 - 8 + getExplosionShiftY(), ATTR0_SQUARE,
                                            j*16 + getExplosionShiftX(), SPRITE_SIZE_16, 0, 0,
                                            2,  PS_SPRITES_PAL_SMOKE, structure_smoke_graphics_offset + ((curStructure->smoke_time / STRUCTURE_SINGLE_SMOKE_DURATION) % 3));
                    }
                    baseTile = curStructureInfo->graphics_offset;
                    if (curStructureInfo->can_rotate_turret) {
                        if (curStructure->turret_positioned >= LEFT_DOWN && curStructure->turret_positioned <= LEFT_UP)
                            hflip = 1;
                        switch (curStructure->turret_positioned) {
                            case RIGHT_UP:
                            case LEFT_UP:
                                baseTile += 1*4;
                                break;
                            case UP:
                                baseTile += 2*4;
                                break;
                            case DOWN:
                                baseTile += 3*4;
                                break;
                            case RIGHT_DOWN:
                            case LEFT_DOWN:
                                baseTile += 4*4;
                                break;
                            default:
                                break;
                        }
                    }
                    if (curStructureInfo->barrier)
                        baseTile += curStructure->logic_aid*4;
                    
                    for (k=0; k<curStructureInfo->flag_amount; k++) {
                        if (curStructureInfo->flag[k].x < 16 && curStructureInfo->flag[k].y < 16)
                            setSpritePlayScreen((ACTION_BAR_SIZE * 16 + i*16 + curStructureInfo->flag[k].y - 8) + getExplosionShiftY(), ATTR0_SQUARE,
                                                (j*16 + curStructureInfo->flag[k].x - 7) + getExplosionShiftX(), SPRITE_SIZE_8, 0, 0,
                                                2, PS_SPRITES_PAL_FACTIONS + getFaction(curStructure->side), base_structure_flag + ((curStructure->x + curStructureInfo->flag[k].x + curStructure->y + curStructureInfo->flag[k].y + (flagAnimationTimer / (FPS/2))) & 1));
                    }
                }
                // check for animation, possibly needing to shift baseTile
                if (curStructureInfo->active_ani &&
                     ((curStructure->logic_aid && !curStructureInfo->shoot_range && !curStructureInfo->barrier) ||
                      (curStructureInfo->shoot_range && ((curStructureInfo->double_shot && curStructure->reload_time >= curStructureInfo->reload_time - SHOOT_ANIMATION_DURATION)  || (curStructure->reload_time < DOUBLE_SHOT_AT && curStructure->reload_time >= DOUBLE_SHOT_AT - SHOOT_ANIMATION_DURATION)))))
                    baseTile += (curStructureInfo->idle_ani + (curStructure->logic_aid / STRUCTURE_ANIMATION_FRAME_DURATION)) * curStructureInfo->width * curStructureInfo->height * 4 * (curStructureInfo->can_rotate_turret * 4 + 1);
                else if (curStructureInfo->idle_ani > 1)
                    baseTile += ((structureAnimationTimer / STRUCTURE_ANIMATION_FRAME_DURATION) % curStructureInfo->idle_ani) * curStructureInfo->width * curStructureInfo->height * 4 * (curStructureInfo->can_rotate_turret * 4 + 1) * (curStructureInfo->barrier * 11 + 1);
                // draw the correct tile of the structure
                mapBG2[ACTION_BAR_SIZE * 64 + 64*i + 2*j]      = (hflip<<10) | (baseTile   +hflip);
                mapBG2[ACTION_BAR_SIZE * 64 + 64*i + 2*j+1]    = (hflip<<10) | (baseTile+1 -hflip);
                mapBG2[ACTION_BAR_SIZE * 64 + 64*i + 2*j+32]   = (hflip<<10) | (baseTile+2 +hflip);
                mapBG2[ACTION_BAR_SIZE * 64 + 64*i + 2*j+1+32] = (hflip<<10) | (baseTile+3 -hflip);
            }
            mapTile++;
        }
        mapTile += (environment.width - HORIZONTAL_WIDTH);
    }
    if (structureSelected) {
        /* the selected structure is (partly) in view, so that requires a selection box around it */
        x = structureSelected->x - getViewCurrentX();
        y = structureSelected->y - getViewCurrentY();
        if (y > 0 && y <= HORIZONTAL_HEIGHT) { // draw top (excluding health bar)
            if (x-1 >= 0 && x-1 < HORIZONTAL_WIDTH) // draw top left corner
                mapBG1[(ACTION_BAR_SIZE * 64 - 31) + 2* (y*32 + x - 1)] = base_structure_selected+1;
            for (i=0; i<structureInfo[structureSelected->info].width; i++) {
                if (x+i >= 0 && x+i < HORIZONTAL_WIDTH) {
                    mapBG1[(ACTION_BAR_SIZE * 64 - 32) + 2* (y*32 + x + i)] = base_structure_selected;
                    mapBG1[(ACTION_BAR_SIZE * 64 - 31) + 2* (y*32 + x + i)] = base_structure_selected;
                }
            }
            if (x+i >= 0 && x+i < HORIZONTAL_WIDTH) // draw top right corner
                mapBG1[(ACTION_BAR_SIZE * 64 - 32) + 2* (y*32 + x + i)] = (1<<10) | (base_structure_selected+1);
        }
        if (y >= 0 && y < HORIZONTAL_HEIGHT) { // draw health bar
            healthPixels = (structureSelected->armour * structureInfo[structureSelected->info].width * 16) / structureInfo[structureSelected->info].max_armour;
            healthBase = base_structure_selected + 4;
            if (structureSelected->armour * 4 < structureInfo[structureSelected->info].max_armour)
                healthBase += 0;
            else if (structureSelected->armour * 2 < structureInfo[structureSelected->info].max_armour)
                healthBase += 8;
            else
                healthBase += 16;
            for (i=0; i<structureInfo[structureSelected->info].width; i++) {
                if (x+i >= 0 && x+i < HORIZONTAL_WIDTH) {
                    if (healthPixels <= 0)
                        mapBG1[(ACTION_BAR_SIZE * 64) + 2* (y*32 + x + i)] = base_structure_selected+3;
                    else if (healthPixels < 8) {
                        mapBG1[(ACTION_BAR_SIZE * 64) + 2* (y*32 + x + i)] = healthBase + healthPixels - 1;
                        healthPixels = 0;
                    } else {
                        mapBG1[(ACTION_BAR_SIZE * 64) + 2* (y*32 + x + i)] = healthBase + 7;
                        healthPixels -= 8;
                    }
                    
                    if (healthPixels <= 0)
                        mapBG1[(ACTION_BAR_SIZE * 64 + 1) + 2* (y*32 + x + i)] = base_structure_selected+3;
                    else if (healthPixels < 8) {
                        mapBG1[(ACTION_BAR_SIZE * 64 + 1) + 2* (y*32 + x + i)] = healthBase + healthPixels - 1;
                        healthPixels = 0;
                    } else {
                        mapBG1[(ACTION_BAR_SIZE * 64 + 1) + 2* (y*32 + x + i)] = healthBase + 7;
                        healthPixels -= 8;
                    }
                }
            }
        }
        if (x > 0 && x <= HORIZONTAL_WIDTH) { // draw left side
            for (i=0; i<structureInfo[structureSelected->info].height; i++) {
                if (y+i >= 0 && y+i < HORIZONTAL_HEIGHT) {
                    mapBG1[(ACTION_BAR_SIZE * 64) +      2* ((y+i)*32 + x) - 1] = base_structure_selected+2;
                    mapBG1[(ACTION_BAR_SIZE * 64 + 32) + 2* ((y+i)*32 + x) - 1] = base_structure_selected+2;
                }
            }
        }
        if (x + structureInfo[structureSelected->info].width + 1 > 0 && x + structureInfo[structureSelected->info].width + 1 <= HORIZONTAL_WIDTH) { // draw right side
            for (i=0; i<structureInfo[structureSelected->info].height; i++) {
                if (y+i >= 0 && y+i < HORIZONTAL_HEIGHT) {
                    mapBG1[(ACTION_BAR_SIZE * 64) +      2* ((y+i)*32 + (x+structureInfo[structureSelected->info].width))] = (1<<10) | (base_structure_selected+2);
                    mapBG1[(ACTION_BAR_SIZE * 64 + 32) + 2* ((y+i)*32 + (x+structureInfo[structureSelected->info].width))] = (1<<10) | (base_structure_selected+2);
                }
            }
        }
        if (y + structureInfo[structureSelected->info].height + 1 > 0 && y + structureInfo[structureSelected->info].height + 1 <= HORIZONTAL_HEIGHT) { // draw bottom
            if (x-1 >= 0 && x-1 < HORIZONTAL_WIDTH) // draw bottom left corner
                mapBG1[(ACTION_BAR_SIZE * 64 + 1) + 2* ((y+structureInfo[structureSelected->info].height)*32 + x - 1)] = (1<<11) | (base_structure_selected+1);
            for (i=0; i<structureInfo[structureSelected->info].width; i++) {
                if (x+i >= 0 && x+i < HORIZONTAL_WIDTH) {
                    mapBG1[(ACTION_BAR_SIZE * 64) +     2* ((y+structureInfo[structureSelected->info].height)*32 + x + i)] = (1<<11) | base_structure_selected;
                    mapBG1[(ACTION_BAR_SIZE * 64 + 1) + 2* ((y+structureInfo[structureSelected->info].height)*32 + x + i)] = (1<<11) | base_structure_selected;
                }
            }
            if (x+i >= 0 && x+i < HORIZONTAL_WIDTH) // draw bottom right corner
                mapBG1[(ACTION_BAR_SIZE * 64) + 2* ((y+structureInfo[structureSelected->info].height)*32 + x + i)] = (1<<11) | (1<<10) | (base_structure_selected+1);
        }
    } else { // !structureSelected
        // find a selected structure anywhere on the map, if it exists
        for (i=0, curStructure=structure; i<MAX_STRUCTURES_ON_MAP; i++, curStructure++) {
            if (curStructure->selected && curStructure->enabled) {
                structureSelected = curStructure;
                break;
            }
        }
    }
    if (structureSelected && structureSelected->side == FRIENDLY && structureInfo[structureSelected->info].release_tile >= 0) { // we might need to draw the rally point (flag)
        x = X_FROM_TILE(structureSelected->rally_tile);
        y = Y_FROM_TILE(structureSelected->rally_tile);
        x -= getViewCurrentX();
        y -= getViewCurrentY();
        if (x >= 0 && x < HORIZONTAL_WIDTH && y >= 0 && y < HORIZONTAL_HEIGHT) {
            setPlayScreenSpritesMode(PSSM_NORMAL);
            setSpritePlayScreen(ACTION_BAR_SIZE*16 + y*16, ATTR0_SQUARE,
                                x*16, SPRITE_SIZE_16, 0, 0,
                                0, PS_SPRITES_PAL_GUI, structure_rally_graphics_offset);
            setPlayScreenSpritesMode(PSSM_EXPANDED);
        }
    }
    
    stopProfilingFunction();
}


void loadStructuresSelectionGraphicsBG(int baseBg, int *offsetBg) {
    base_structure_selected = (*offsetBg)/(8*8);
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), "structure_selected", FS_GUI_GRAPHICS);
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), "structure_selected_low", FS_GUI_GRAPHICS);
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), "structure_selected_med", FS_GUI_GRAPHICS);
    *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), "structure_selected_hgh", FS_GUI_GRAPHICS);
}

void loadStructuresGraphicsBG(int baseBg, int *offsetBg) {
    int i;
    char oneline[256];
    char filename[256];
    char *filePosition;
    FILE *fp;
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    // GRAPHICS section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[GRAPHICS]", strlen("[GRAPHICS]")));
    for (i=0; i<3; i++)
        readstr(fp, oneline);
    replaceEOLwithEOF(oneline, 255);
    closeFile(fp);
    
    strcpy(filename, oneline + strlen("Structures="));
    if (filename[0] != 0)
        strcat(filename, "/");
    filePosition = filename + strlen(filename);
    
    for (i=0; i<MAX_DIFFERENT_STRUCTURES && structureInfo[i].enabled; i++) {
        structureInfo[i].graphics_offset = (*offsetBg)/(8*8);
        strcpy(filePosition, structureInfo[i].name);
        *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_STRUCTURES_GRAPHICS);
        if (structureInfo[i].active_ani > 0) {
            sprintf(filePosition, "%s_active", structureInfo[i].name);
            *offsetBg += copyFileVRAM((uint16*)(baseBg + *offsetBg), filename, FS_STRUCTURES_GRAPHICS);
        }
    }
}

void loadStructuresGraphicsSprites(int *offsetSp) {
    uint16 buffer[64];
    int i;
    
    copyFile(buffer, "Structure", FS_FLAG_GRAPHICS);
    base_structure_flag = (*offsetSp)/(16*16);
    for (i=0; i<32; i++) {
        *(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1) + i) = buffer[i];
        *(SPRITE_EXPANDED_GFX + ((*offsetSp + (16*16))>>1) + i) = buffer[32 + i];
    }
    *offsetSp += 512;
}

void loadStructureRallyGraphicsSprites(int *offsetSp) {
    structure_rally_graphics_offset = (*offsetSp)/(16*16);
    *offsetSp += copyFileVRAM(SPRITE_GFX + ((*offsetSp)>>1), "structure_rallypoint", FS_GUI_GRAPHICS);
}



inline void doStructureAttemptToShoot(struct Structure *curStructure, struct StructureInfo *curStructureInfo, int tgt_x, int tgt_y) {
    if (curStructureInfo->double_shot && curStructure->reload_time == DOUBLE_SHOT_AT) { // shoot
        if (curStructure->misfire_time > 0)
          curStructure->misfire_time--;
        else if (randFixed()%4 == 0) // 25% chance of misfire
          curStructure->misfire_time = (6/getGameSpeed()) -1;
        else {
            addProjectile(curStructure->side, curStructure->x, curStructure->y, tgt_x, tgt_y, curStructureInfo->projectile_info, 0);
            if (curStructureInfo->shoot_range >= 10)
                addTankShot(curStructure->x + (curStructureInfo->width - 1) / 2, curStructure->y + (curStructureInfo->height - 1));
            curStructure->reload_time--;
        }
        
    } else if (curStructure->reload_time <= 0) { // shoot
        if (curStructure->misfire_time > 0)
          curStructure->misfire_time--;
        else if (randFixed()%4 == 0) // 25% chance of misfire
            curStructure->misfire_time= (6/getGameSpeed()) -1;
        else {
            addProjectile(curStructure->side, curStructure->x, curStructure->y, tgt_x, tgt_y, curStructureInfo->projectile_info, curStructureInfo->double_shot);
            if (curStructureInfo->shoot_range >= 10)
                addTankShot(curStructure->x + (curStructureInfo->width - 1) / 2, curStructure->y + (curStructureInfo->height - 1));
            
            // sligtly modify reload time to (-3,-2,-1,0,+1,+2,+3)*3/gameSpeed
            curStructure->reload_time = curStructureInfo->reload_time + ((generateRandVariation(4)*3)/getGameSpeed());
            if (curStructure->reload_time < 1)
                curStructure->reload_time = 1;
            
            // re-evaluate target:
            curStructure->logic_aid = sideUnitWithinShootRange(!curStructure->side, curStructure->x, curStructure->y, curStructureInfo->shoot_range, curStructureInfo->projectile_info);
            // ... and switch mode to regular guard if there's no target.
            if (curStructure->logic_aid == -1)
                curStructure->logic -= 2; // to regular guard
        }
    }
}


void doStructuresLogic() {
    static int frame = 0;
    int i, j, k, l;
    int aid;
    int creditsRequired, creditsReceived;
    int oldRetreatTile, newRetreatTile;
    struct Structure *curStructure;
    struct StructureInfo *curStructureInfo;
    struct TechtreeItem *techtreeItem;
    int rebuildDelay[MAX_DIFFERENT_FACTIONS];
    
    startProfilingFunction("doStructuresLogic");
    
    for (i=1; i<getAmountOfSides(); i++) {
        rebuildDelay[i] = getRebuildDelayStructures(i);
    }
    
    flagAnimationTimer++;
    if (flagAnimationTimer == FPS)
        flagAnimationTimer = 0;
    structureAnimationTimer++;
    if (structureAnimationTimer == STRUCTURE_ANIMATION_FRAME_DURATION * (1*2*3*4*5*6*7*8*9*10)) // max 10 animation frames, k?
        structureAnimationTimer = 1;
    
    curStructure = structure + MAX_DIFFERENT_FACTIONS;
    for (i=MAX_DIFFERENT_FACTIONS; i<MAX_STRUCTURES_ON_MAP; i++, curStructure++) {
        if (curStructure->enabled) {
            
            curStructureInfo = structureInfo + curStructure->info;
            
            // death of structure
            if (curStructure->armour <= 0) {
//              assert(!curStructureInfo->foundation);
                
                // if it were to be sold, bad luck. no deal!
                if (getModifier() == PSTM_SELL_STRUCTURE && getSellNrStructure() == i)
                    setModifierNone();
                
                // if a rally point for it was to be set, tough luck. no go!
                if (getModifier() == PSTM_RALLYPOINT_STRUCTURE && curStructure->selected)
                    setModifierNone();
                
                // if it was a primary, a new primary should be set if possible
                if (curStructure->primary) {
                    for (j=0; j<MAX_STRUCTURES_ON_MAP; j++) {
                        if (structure[j].enabled && structure[j].info == curStructure->info && structure[j].side == curStructure->side && i != j) {
                            structure[j].primary = 1;
                            curStructure->primary = 0;
                            break;
                        }
                    }
                }
                
                // any unit attacking this structure should stop
                aid = TILE_FROM_XY(curStructure->x + curStructureInfo->width/2, curStructure->y + curStructureInfo->height/2);
                for (j=0; j<MAX_UNITS_ON_MAP; j++) {
                    if (unit[j].enabled && (unit[j].logic == UL_ATTACK_LOCATION || unit[j].logic == UL_ATTACK_AREA)) {
                        k = environment.layout[unit[j].logic_aid].contains_structure;
                        if (k < -1)
                            k = environment.layout[unit[j].logic_aid + (k + 1)].contains_structure;
                        if (k == i) {
                            if (getGameType() == SINGLEPLAYER) {
                                if (unit[j].side != FRIENDLY) {
                                    if (unit[j].group == UGAI_HUNT) {
                                        unit[j].logic = UL_HUNT;
                                        unit[j].logic_aid = 0;
                                    } else if (unit[j].group == UGAI_KAMIKAZE)
                                        unit[j].logic = UL_KAMIKAZE;
                                    else /*if (unit[j].group == UGAI_GUARD_AREA || unit[j].group == UGAI_PATROL)*/
                                        unit[j].logic = UL_GUARD_RETREAT;
                                    continue;
                                }
                            }
                            if (unit[j].group & (UGAI_ATTACK_FORCED | UGAI_ATTACK)) {
                                unit[j].logic = UL_CONTINUE_ATTACK;
                                unit[j].logic_aid = aid;
                            } else /*if (unit[j].group & UGAI_GUARD)*/
                                unit[j].logic = UL_GUARD_RETREAT;
                        }
                    }
                }
                
                // collectors of ore which had this set as retreat tile should get a new location to retreat to
                if (curStructureInfo->can_extract_ore) {
                    oldRetreatTile = TILE_FROM_XY(curStructure->x + curStructureInfo->release_tile, curStructure->y + curStructureInfo->height - 1);
                    newRetreatTile = -1;
                    for (j=0; j<MAX_STRUCTURES_ON_MAP; j++) {
                        if (structure[j].enabled && structure[j].primary && structure[j].info == curStructure->info && structure[j].side == curStructure->side && i != j) {
                            newRetreatTile = TILE_FROM_XY(structure[j].x + structureInfo[structure[j].info].release_tile, structure[j].y + structureInfo[structure[j].info].height - 1);
                            break;
                        }
                    }
                    for (j=0; j<MAX_UNITS_ON_MAP; j++) {
                        if (unit[j].enabled && unit[j].retreat_tile == oldRetreatTile && unitInfo[unit[j].info].can_collect_ore)
                            unit[j].retreat_tile = newRetreatTile;
                    }
                }
            
                // if it was an enemy AI structure, it would be good to put it in their build queue
                if (curStructure->side != FRIENDLY) {
                    if (!curStructureInfo->barrier && rebuildDelay[curStructure->side] != -1)
                        curStructure->group = AWAITING_REPLACEMENT;
                }
                
                // adjust power and credits
                setOreStorage(curStructure->side, getOreStorage(curStructure->side) - curStructureInfo->ore_storage);
                setPowerGeneration(curStructure->side, getPowerGeneration(curStructure->side) - curStructureInfo->power_generating);
                setPowerConsumation(curStructure->side, getPowerConsumation(curStructure->side) - curStructureInfo->power_consuming);
                
                // remove it from map
                for (j=0; j<curStructureInfo->height; j++) {
                    for (k=0; k<curStructureInfo->width; k++) {
                        environment.layout[TILE_FROM_XY(curStructure->x + k, curStructure->y + j)].contains_structure = -1;
                        setTileDirtyRadarDirtyBitmap(TILE_FROM_XY(curStructure->x + k, curStructure->y + j));
                        addExplosion((curStructure->x + k) * 16 + 8, (curStructure->y + j) * 16 + 8, curStructureInfo->destroyed_explosion_info, (j & 1) * (FPS/5) + (k&1 * (FPS/10)));
                        initTileTraversability(environment.layout + TILE_FROM_XY(curStructure->x + k, curStructure->y + j));
                    }
                }
                if (!curStructureInfo->no_overlay_on_destruction)
                    addStructureDestroyedOverlay(curStructure->x, curStructure->y, curStructureInfo->width, curStructureInfo->height);
                if (!curStructureInfo->no_sound_on_destruction)
                    playSoundeffectControlled(SE_EXPLOSION_STRUCTURE, volumePercentageOfLocation(curStructure->x, curStructure->y, DISTANCE_TO_HALVE_SE_EXPLOSION_STRUCTURE), soundeffectPanningOfLocation(curStructure->x, curStructure->y));
                
                if (curStructureInfo->barrier) {
                    if (curStructure->x > 0 && environment.layout[TILE_FROM_XY(curStructure->x-1, curStructure->y)].contains_structure >= 0 && structure[environment.layout[TILE_FROM_XY(curStructure->x-1, curStructure->y)].contains_structure].info == curStructure->info)
                        adjustBarrierType(curStructure->x-1, curStructure->y);
                    if (curStructure->x < environment.width - 1 && environment.layout[TILE_FROM_XY(curStructure->x+1, curStructure->y)].contains_structure >= 0 && structure[environment.layout[TILE_FROM_XY(curStructure->x+1, curStructure->y)].contains_structure].info == curStructure->info)
                        adjustBarrierType(curStructure->x+1, curStructure->y);
                    if (curStructure->y > 0 && environment.layout[TILE_FROM_XY(curStructure->x, curStructure->y-1)].contains_structure >= 0 && structure[environment.layout[TILE_FROM_XY(curStructure->x, curStructure->y-1)].contains_structure].info == curStructure->info)
                        adjustBarrierType(curStructure->x, curStructure->y-1);
                    if (curStructure->y < environment.height - 1 && environment.layout[TILE_FROM_XY(curStructure->x, curStructure->y+1)].contains_structure >= 0 && structure[environment.layout[TILE_FROM_XY(curStructure->x, curStructure->y+1)].contains_structure].info == curStructure->info)
                        adjustBarrierType(curStructure->x, curStructure->y+1);
                }
                
                curStructure->enabled = 0;
                if (curStructure->contains_unit >= 0)
                    doUnitDeath(curStructure->contains_unit); // this "death" always needs to occur -after- the structure is disabled
                if (curStructure->primary) {
                    l = getTechtree(curStructure->side)->amountOfItems;
                    techtreeItem = &getTechtree(curStructure->side)->item[0];
                    for (j=0; j<l; j++, techtreeItem++) {
                        if (techtreeItem->buildingInfo == curStructure->info) {
// TODO: make sure the buildinfo shown (IF shown) must be switched to radar view if it's the same item as the one just blown up
                            if (techtreeItem->buildStructure) {
                                for (k=0; k<5; k++)
                                    removeItemStructuresQueueWithInfo(curStructure->side, techtreeItem->buildableInfo, 0, 0);
                            } else {
                                for (k=0; k<10; k++)
                                    removeItemUnitsQueueWithInfo(curStructure->side, techtreeItem->buildableInfo, 0, 0);
                            }
                        }
                    }
                    if (curStructure->side == FRIENDLY)
                        createBarStructures(); // automaticly creates barBuildables as well
                }
                
                if (!curStructureInfo->foundation && !curStructureInfo->barrier) {
                    setStructureCount(curStructure->side, getStructureCount(curStructure->side) - 1);
                    setStructureDeaths(curStructure->side, getStructureDeaths(curStructure->side) + 1);
                }
                
                continue;
            }
            
            // posess no logic other than being able to be destroyed... or act as tripwire.
            if (curStructureInfo->barrier) {
                if (curStructureInfo->shoot_range != 0) { // tripwire. use move_aid instead of logic_aid.
                    curStructure->move_aid = sideUnitWithinRange(!curStructure->side, curStructure->x, curStructure->y, curStructureInfo->shoot_range);
                    if (curStructure->move_aid >= 0) { // shoot it and make sure this structure blows up -immediately-
                        addProjectile(curStructure->side, curStructure->x, curStructure->y, unit[curStructure->move_aid].x, unit[curStructure->move_aid].y, curStructureInfo->projectile_info, 0);
                        curStructure->armour = 0;
                        i--;
                        curStructure--;
                    }
                }
                continue;
            }
            
/*            if (curStructure->move <= SM_REPAIRING) {
                
                // repairing structure
                if (curStructure->logic & 1) {
                    if (curStructure->armour >= curStructureInfo->max_armour)
                        curStructure->logic--;
                    else if (curStructure->move != SM_REPAIRING) {
                        curStructure->move = SM_REPAIRING;
                        curStructure->move_aid = 0;
                    }
                } else if (curStructure->move == SM_REPAIRING)
                    curStructure->move = SM_NONE;
            
            }*/
            
            if (curStructure->move == SM_NONE) {
                
                // repairing unit
                if (curStructure->contains_unit >= 0 && curStructureInfo->can_repair_unit) {
                    curStructure->move = SM_REPAIRING_UNIT;
                    unit[curStructure->contains_unit].move_aid = (unit[curStructure->contains_unit].armour * unitInfo[unit[curStructure->contains_unit].info].repair_time) / unitInfo[unit[curStructure->contains_unit].info].max_armour;
                    if (!curStructure->logic_aid)
                        curStructure->logic_aid = 1;
                }
                
                // extracting ore
                else if (curStructure->contains_unit >= 0 && curStructureInfo->can_extract_ore) {
                    curStructure->move = SM_EXTRACTING_ORE;
                    unit[curStructure->contains_unit].move_aid = ((MAX_ORED_BY_UNIT - unit[curStructure->contains_unit].ore) * TIME_TO_EXTRACT_MAX_ORED) / MAX_ORED_BY_UNIT;
                    if (!curStructure->logic_aid)
                        curStructure->logic_aid = 1;
                }
            
                // guarding
                else if (curStructureInfo->power_consuming == 0 || getPowerGeneration(curStructure->side) >= getPowerConsumation(curStructure->side)) {
                    if (curStructure->logic == SL_GUARD || curStructure->logic == SL_GUARD_N_REPAIRING) {
                        // Check if an enemy unit is within shooting range, but ensure this doesn't 
                        // happen every frame, which would slow down the game considerably.
                        if ((frame + i) % (FPS / getGameSpeed()) == 0) {
                            aid = sideUnitWithinShootRange(!curStructure->side, curStructure->x, curStructure->y, curStructureInfo->shoot_range, curStructureInfo->projectile_info);
                            if (aid != -1) {
                                curStructure->logic_aid = aid; 
                                curStructure->logic += 2; // to guarding unit
                            }
                        }
                    }
                    if (curStructure->logic == SL_GUARD_UNIT || curStructure->logic == SL_GUARD_UNIT_N_REPAIRING) {
                        if (!withinShootRange(curStructure->x, curStructure->y, curStructureInfo->shoot_range, curStructureInfo->projectile_info, unit[curStructure->logic_aid].x, unit[curStructure->logic_aid].y))
                            curStructure->logic -= 2; // to regular guard
                        else {
                            if (curStructureInfo->can_rotate_turret /*&& curStructure->move == SM_NONE*/)
                                curStructure->move = rotateToFaceXY(curStructure->x, curStructure->y, curStructure->turret_positioned, unit[curStructure->logic_aid].x, unit[curStructure->logic_aid].y);
                            if (curStructure->move != SM_NONE) {
                                curStructure->move += (SM_ROTATE_TURRET_LEFT - 1);
                                curStructure->move_aid = curStructureInfo->rotate_speed | (curStructure->move_aid << 16);
                            } else
                                doStructureAttemptToShoot(curStructure, curStructureInfo, unit[curStructure->logic_aid].x, unit[curStructure->logic_aid].y);
                        }
                    }
                }
                
            }
            
            // repairing (can only be done when not rotating its turret)
            if ((curStructure->logic & 1) && curStructure->move < SM_ROTATE_TURRET_LEFT) {
                
                if (getRepairSpeed(curStructure->side) > 0 && (frame % (100/getRepairSpeed(curStructure->side)) == 0)) {
                    creditsRequired = ((curStructureInfo->repair_cost * (curStructure->move_aid + 1)) / curStructureInfo->repair_time) -
                                      ((curStructureInfo->repair_cost * (curStructure->move_aid)) / curStructureInfo->repair_time);
                    
                    if (changeCredits(curStructure->side, -creditsRequired)) {
                        curStructure->armour += ((curStructureInfo->max_armour * (curStructure->move_aid + 1)) / curStructureInfo->repair_time) -
                                                ((curStructureInfo->max_armour * (curStructure->move_aid)) / curStructureInfo->repair_time);
                        curStructure->move_aid++;
                        if (curStructure->move_aid > curStructureInfo->repair_time) // making sure the "timer" doesn't overflow
                            curStructure->move_aid = 0;
                        if (curStructure->armour >= curStructureInfo->max_armour / 2)
                            curStructure->smoke_time = 0;
                        if (curStructure->armour >= curStructureInfo->max_armour) {
                            curStructure->armour = curStructureInfo->max_armour;
                            curStructure->logic--;
                        }
                    }
                }
            }
            
            // repairing unit
            if (curStructure->move == SM_REPAIRING_UNIT) {
                if (unit[curStructure->contains_unit].armour >= unitInfo[unit[curStructure->contains_unit].info].max_armour) {
                    unit[curStructure->contains_unit].armour = unitInfo[unit[curStructure->contains_unit].info].max_armour;
                    unit[curStructure->contains_unit].smoke_time = 0;
//                    environment.layout[(curStructure->y + curStructureInfo->height - 1) * mapWidth + curStructure->x + curStructureInfo->release_tile].contains_unit = -1;
                    if (getGameType() == SINGLEPLAYER) {
                        if (dropUnit(curStructure->contains_unit, curStructure->x + curStructureInfo->release_tile, curStructure->y + curStructureInfo->height - 1)) {
                            curStructure->contains_unit = -1;
                            if (curStructure->side == FRIENDLY)
                                playSoundeffect(SE_UNIT_REPAIRED);
                            curStructure->move = SM_NONE;
                        }
//                      else
//                          environment.layout[(curStructure->y + curStructureInfo->height - 1) * mapWidth + curStructure->x + curStructureInfo->release_tile].contains_unit = curStructure->contains_unit;
                    }
                } else {
                    creditsRequired = ((unitInfo[unit[curStructure->contains_unit].info].repair_cost * (unit[curStructure->contains_unit].move_aid + 1)) / unitInfo[unit[curStructure->contains_unit].info].repair_time) -
                                      ((unitInfo[unit[curStructure->contains_unit].info].repair_cost * (unit[curStructure->contains_unit].move_aid)) / unitInfo[unit[curStructure->contains_unit].info].repair_time);
                    if (changeCredits(curStructure->side, -creditsRequired)) {
                        unit[curStructure->contains_unit].armour += ((unitInfo[unit[curStructure->contains_unit].info].max_armour * (unit[curStructure->contains_unit].move_aid + 1)) / unitInfo[unit[curStructure->contains_unit].info].repair_time) -
                                                                     ((unitInfo[unit[curStructure->contains_unit].info].max_armour * (unit[curStructure->contains_unit].move_aid)) / unitInfo[unit[curStructure->contains_unit].info].repair_time);
                        unit[curStructure->contains_unit].move_aid++;
                    }
                }
            }
            
            // extracting ore
            else if (curStructure->move == SM_EXTRACTING_ORE) {
                if (unit[curStructure->contains_unit].ore <= 0) {
//                  environment.layout[(curStructure->y + curStructureInfo->height - 1) * mapWidth + curStructure->x + curStructureInfo->release_tile].contains_unit = -1;
                    if (dropUnit(curStructure->contains_unit, curStructure->x + curStructureInfo->release_tile, curStructure->y + curStructureInfo->height - 1)) {
                        unit[curStructure->contains_unit].retreat_tile = TILE_FROM_XY(curStructure->x + curStructureInfo->release_tile, curStructure->y + curStructureInfo->height - 1);
                        unit[curStructure->contains_unit].ore = 0;
                        curStructure->contains_unit = -1;
                        curStructure->move = SM_NONE;
                    }
//                  else
//                      environment.layout[(curStructure->y + curStructureInfo->height - 1) * mapWidth + curStructure->x + curStructureInfo->release_tile].contains_unit = curStructure->contains_unit;
                } else {
                    creditsReceived = ((MAX_ORED_BY_UNIT * (unit[curStructure->contains_unit].move_aid+1)) / TIME_TO_EXTRACT_MAX_ORED) -
                                      ((MAX_ORED_BY_UNIT * (unit[curStructure->contains_unit].move_aid)) / TIME_TO_EXTRACT_MAX_ORED);
                    changeCredits(curStructure->side, creditsReceived);
                    unit[curStructure->contains_unit].move_aid++;
                    unit[curStructure->contains_unit].ore -= creditsReceived;
                }
            }
            
            // rotation of turret
            else if (curStructure->move == SM_ROTATE_TURRET_LEFT) {
                curStructure->move_aid--;
                if ((curStructure->move_aid & 0xFFFF) == 0) {
                    if (curStructure->turret_positioned == UP) curStructure->turret_positioned = LEFT_UP;
                    else curStructure->turret_positioned -= 1;
                    curStructure->move = SM_NONE;
                    curStructure->move_aid = curStructure->move_aid >> 16; // restoring the "repairing" move_aid
                }
            }
            else if (curStructure->move == SM_ROTATE_TURRET_RIGHT) {
                curStructure->move_aid--;
                if ((curStructure->move_aid & 0xFFFF) == 0) {
                    if (curStructure->turret_positioned == LEFT_UP) curStructure->turret_positioned = UP;
                    else curStructure->turret_positioned += 1;
                    curStructure->move = SM_NONE;
                    curStructure->move_aid = curStructure->move_aid >> 16; // restoring the "repairing" move_aid
                }
            }
            
            if (curStructure->reload_time > 0 && !(curStructureInfo->double_shot && curStructure->reload_time == DOUBLE_SHOT_AT))
                curStructure->reload_time--;
            
            if (curStructure->smoke_time > 0)
                curStructure->smoke_time--;
            
            if (curStructureInfo->active_ani) {
                if (!strcmp(curStructureInfo->name, "Radar")) {
                    if (getPowerConsumation(curStructure->side) > getPowerGeneration(curStructure->side))
                        curStructure->logic_aid = 0;
                    else if (!curStructure->logic_aid)
                        curStructure->logic_aid = 1;
                }
                if (curStructure->logic_aid > 0 && !curStructureInfo->barrier && !curStructureInfo->shoot_range) {
                    if ((curStructureInfo->can_extract_ore && curStructure->move != SM_EXTRACTING_ORE) || (curStructureInfo->can_repair_unit && curStructure->move != SM_REPAIRING_UNIT))
                        curStructure->logic_aid = 0;
                    else {
                        curStructure->logic_aid++;
                        if (curStructure->logic_aid >= STRUCTURE_ANIMATION_FRAME_DURATION * curStructureInfo->active_ani)
                            curStructure->logic_aid = 1 * (curStructure->move == SM_EXTRACTING_ORE || curStructure->move == SM_REPAIRING_UNIT || !strcmp(curStructureInfo->name, "Radar"));
                    }
                }
            }
        }
    }
    
    frame++;
    if (frame == 99999) frame = 0;
    
    stopProfilingFunction();
}

void doStructureLogicHit(int nr, int projectileSourceTile) {
    int unitNrWhichShot;
    int i;
    int x, y;
    
    if (getGameType() == SINGLEPLAYER) {
        // start repair structure logic if health is below half of the maximum
        if (structure[nr].side != FRIENDLY && structure[nr].armour < structureInfo[structure[nr].info].max_armour/2 && !structureInfo[structure[nr].info].barrier && !structureInfo[structure[nr].info].foundation)
            structure[nr].logic |= 1;
    }

    unitNrWhichShot = environment.layout[projectileSourceTile].contains_unit;
    if (unitNrWhichShot >= 0) { // was a unit that shot
        
        if (!unit[unitNrWhichShot].side != !structure[nr].side) {
            x = structure[nr].x; //unit[unitNrWhichShot].x;
            y = structure[nr].y; //unit[unitNrWhichShot].y;
            // check all units in the area to see if they should attack the unit back: when GUARD/GUARD_RETREAT or ATTACK_LOCATION
            for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                if (unit[i].enabled && unit[i].side == structure[nr].side &&
                    abs(unit[i].y - y) <= MAX_RADIUS_FRIENDLY_STRUCTURE_AID && abs(unit[i].x - x) <= MAX_RADIUS_FRIENDLY_STRUCTURE_AID &&
                    unitInfo[unit[i].info].shoot_range > 0) {
                    if (unit[i].logic == UL_GUARD || unit[i].logic == UL_GUARD_RETREAT || unit[i].logic == UL_GUARD_AREA || unit[i].move == UM_MOVE_HOLD) {
                        unit[i].logic = UL_ATTACK_UNIT;
                        unit[i].logic_aid = unitNrWhichShot;
                        if (unit[i].side == FRIENDLY)
                            unit[i].group = (unit[i].group & (~UGAI_FRIENDLY_MASK)) | UGAI_GUARD;
                    }
                }
            }
        }
        
    }
}


int getStructuresSaveSize(void) {
    return sizeof(structure);
}


int getStructuresSaveData(void *dest, int max_size) {
    int size=sizeof(structure);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&structure,size);
    return (size);
}
