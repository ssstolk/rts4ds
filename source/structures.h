// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_

#include <nds.h>

#include "settings.h"
#include "shared.h"

#define MAX_DIFFERENT_STRUCTURES   34
#define MAX_STRUCTURE_NAME_LENGTH  30

#define MAX_STRUCTURES_ON_MAP      300

#define MAX_NUMBER_OF_FLAGS        4

#define MAX_BUILDING_DISTANCE      5

#define TIME_TO_EXTRACT_MAX_ORED   ((2*(10*FPS)) / getGameSpeed())

#define STRUCTURE_ANIMATION_FRAME_DURATION   ((2*(FPS)) / getGameSpeed())

#define STRUCTURE_SMOKE_DURATION          (25 * 3*STRUCTURE_SINGLE_SMOKE_DURATION)
#define STRUCTURE_SINGLE_SMOKE_DURATION   ((2*(FPS)) / getGameSpeed())


enum StructureLogic { SL_NONE, SL_REPAIRING,
                       SL_GUARD, SL_GUARD_N_REPAIRING,
                       SL_GUARD_UNIT, SL_GUARD_UNIT_N_REPAIRING
                     };
enum StructureMove { SM_NONE, SM_EXTRACTING_ORE, SM_REPAIRING_UNIT, /*SM_REPAIRING,*/ SM_ROTATE_TURRET_LEFT, SM_ROTATE_TURRET_RIGHT };


struct Flag {
    int x; // specified in pixels here
    int y;
};

struct StructureInfo {
    int enabled;
    char name[MAX_STRUCTURE_NAME_LENGTH+1];
    char description[MAX_DESCRIPTION_LENGTH+1];
    int build_time; /* --seconds-- frames */
    int build_cost;
    int repair_time;
    int repair_cost;
    
    int class_mask;
    
    int width; /* tiles */
    int height; /* tiles */
    int flag_amount;
    struct Flag flag[MAX_NUMBER_OF_FLAGS];
    int max_armour;
    int view_range; /* tiles */
    int foundation; /* is it something to build on top of? */
    int barrier; /* is it a wall */
    
    int projectile_info;
    int rotate_speed; /* frames to rotate turret */
    int shoot_range; /* tiles */
    int reload_time; /* --seconds-- frames */
    int double_shot; /* whether a salvo of two shots is fired or not */
    
    int created_explosion_info;
    int destroyed_explosion_info;
    int power_consuming;
    int power_generating;
    int ore_storage;
    int release_tile;
    int can_extract_ore;
    int can_repair_unit;
    int can_rotate_turret;
    int no_sound_on_destruction;
    int no_overlay_on_destruction;
    int idle_ani;
    int active_ani;
    int graphics_offset;
    
    int original_build_time;
    int original_repair_time;
    int original_rotate_speed;
    int original_reload_time;
};

struct Structure {
    int enabled;
    int info; /* used as a reference to a StructureInfo */
    int primary; /* do created units roll out of this structure? */
    int group; /* yes, hotkeys for structures do exist ;) */
    int selected;
    enum Side side; /* friendly or enemy */
    int x;
    int y;
    int armour;
    enum Positioned turret_positioned;
    enum StructureLogic logic;
    int logic_aid; /* used for turret guarding unit, and for barrier form, and for structure 'active' timer */
    enum StructureMove move;
    int move_aid;
    int smoke_time;
    int reload_time;
    int misfire_time; // frames
    short int contains_unit; /* used as a reference to a unit being repaired or a unit with ore being extracted from it */
    short int rally_tile;
    int multiplayer_id;
};

void initStructures();
void initStructuresWithScenario();

int getStructureNameInfo(char *buffer);

int foundationRequired();
int getDistanceToNearestStructure(enum Side side, int x, int y);

int addStructure(enum Side side, int x, int y, int info, int forced);
void doStructuresLogic();
void doStructureLogicHit(int nr, int projectileSourceTile);

void drawStructures();
void loadStructuresSelectionGraphicsBG(int baseBg, int *offsetBg);
void loadStructuresGraphicsBG(int baseBg, int *offsetBg);
void loadStructureRallyGraphicsSprites(int *offsetSp);
void loadStructuresGraphicsSprites(int *offsetSp);

extern struct StructureInfo structureInfo[];
extern struct Structure structure[];

int getStructuresSaveSize(void);
int getStructuresSaveData(void *dest, int max_size);

#endif
