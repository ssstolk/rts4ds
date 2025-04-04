// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _UNITS_H_
#define _UNITS_H_

#include <nds.h>

#include "environment.h"
#include "projectiles.h"
#include "pathfinding.h"
#include "settings.h"
#include "shared.h"

#define MAX_DIFFERENT_UNITS   39
#define MAX_UNIT_NAME_LENGTH  30

#define MAX_UNITS_ON_MAP      150
#define MAX_REINFORCEMENTS     50

#define MAX_RADIUS_CONTINUE_ATTACK         10
#define MAX_RADIUS_FRIENDLY_UNIT_AID        5
#define MAX_RADIUS_CHASE_ENEMY_UNIT        10
#define MAX_RADIUS_FRIENDLY_STRUCTURE_AID   8
#define MAX_RADIUS_PATROLLING               5
#define MAX_RADIUS_ATTACK_AREA              2
#define UNITS_PATROLLING_MOVE_CHANCE       (3 * FPS)

#define MAX_ORED_BY_UNIT      5*(MAX_ENVIRONMENT_ORE_LEVEL/2)
#define MAX_TIME_TO_ORE_TILE  ((2*((((5*(MAX_ENVIRONMENT_ORE_LEVEL/2)) * FPS) / 60) / 2)) / getGameSpeed())

#define UNIT_SMOKE_DURATION          (10 * 3*UNIT_SINGLE_SMOKE_DURATION)
#define UNIT_SINGLE_SMOKE_DURATION   ((2*(FPS / 2)) / getGameSpeed())

#define UNITS_IDLE_TIME_THRESHHOLD   ((2*(5 * FPS)) / getGameSpeed())
//#define UNITS_IDLE_CHANCE            (15 * FPS)

enum UnitType { UT_FOOT, UT_WHEELED, UT_TRACKED, UT_AERIAL, UT_CREATURE };

enum UnitLogic { UL_ATTACK_UNIT, UL_ATTACK_LOCATION, UL_MOVE_LOCATION,
                 UL_GUARD, UL_GUARD_UNIT, UL_GUARD_AREA, UL_GUARD_RETREAT,
                 UL_AMBUSH, UL_HUNT, UL_KAMIKAZE, UL_CONTINUE_ATTACK,
                 UL_MINE_LOCATION, UL_RETREAT, UL_NOTHING,
                 UL_ATTACK_AREA };

enum UnitMove { UM_NONE, UM_MOVE_UP, UM_MOVE_RIGHT_UP, UM_MOVE_RIGHT, UM_MOVE_RIGHT_DOWN, UM_MOVE_DOWN, UM_MOVE_LEFT_DOWN, UM_MOVE_LEFT, UM_MOVE_LEFT_UP, UM_ROTATE_LEFT, UM_ROTATE_RIGHT, UM_ROTATE_TURRET_LEFT, UM_ROTATE_TURRET_RIGHT, UM_MINE_TILE, UM_IDLE1, UM_IDLE2, UM_IDLE3, UM_IDLE4, UM_IDLE_TURRET1, UM_IDLE_TURRET2, UM_IDLE_TURRET3, UM_IDLE_TURRET4,
                UM_MOVE_HOLD_INITIAL = (UM_NONE | BIT(15) | BIT(14)), UM_MOVE_HOLD = (UM_NONE | BIT(15)), UM_IDLE1_HOLD = (UM_IDLE1 | BIT(15)), UM_IDLE2_HOLD = (UM_IDLE2 | BIT(15)), UM_IDLE3_HOLD = (UM_IDLE3 | BIT(15)), UM_IDLE4_HOLD = (UM_IDLE4 | BIT(15)), UM_IDLE_TURRET1_HOLD = (UM_IDLE_TURRET1 | BIT(15)), UM_IDLE_TURRET2_HOLD = (UM_IDLE_TURRET2 | BIT(15)), UM_IDLE_TURRET3_HOLD = (UM_IDLE_TURRET3 | BIT(15)), UM_IDLE_TURRET4_HOLD = (UM_IDLE_TURRET4 | BIT(15)) };
#define UM_MASK (~(BIT(14) | BIT(15)))

enum UnitGroupAI { UGAI_AMBUSH = BIT(0), UGAI_GUARD_AREA = BIT(1), UGAI_HUNT = BIT(2), UGAI_KAMIKAZE = BIT(3), UGAI_PATROL = BIT(4), UGAI_FORMING = BIT(5), UGAI_STAGING = BIT(6), UGAI_NOTHING = BIT(7), // enemy group AI
                   UGAI_ATTACK_FORCED = BIT(20), UGAI_ATTACK = BIT(21), UGAI_GUARD = BIT(22) }; // friendly group AI. caution! need to be higher than any key values!
#define UGAI_FRIENDLY_MASK  (UGAI_ATTACK_FORCED | UGAI_ATTACK | UGAI_GUARD)


struct UnitInfo {
    int enabled;
    char name[MAX_UNIT_NAME_LENGTH+1];
    enum UnitType type;
    char description[MAX_DESCRIPTION_LENGTH+1];
    int build_time; /* --seconds-- frames */
    int build_cost;
    int repair_time;
    int repair_cost;
    int flag_type;
    int tracks_type;
    int max_armour;
    int speed; /* --tiles per second--  frames to move one tile */
    int rotate_speed; /* frames to rotate chassis and/or turret */
    int view_range; /* tiles */
    int projectile_info;
    int shoot_range; /* tiles */
    int reload_time; /* --seconds-- frames */
    int double_shot; /* whether a salvo of two shots can be fired or not */
    int tank_shot;   /* whether a tankshot explosion needs adding or not */
    int explosion_info;
    int death_soundeffect;
    int can_rotate_turret;
    int contains_structure; // mobile structure ready to deploy
    int can_collect_ore; // ore collector
    int can_heal_foot; // medic, not read in as such but can be determined via projectile
    int graphics_offset;
    uint16 graphics_size;
    int rotation_ani;
    int shoot_ani;
    int move_ani;
    unsigned idle_ani;
    
    int original_build_time;
    int original_repair_time;
    int original_speed;
    int original_rotate_speed;
    int original_reload_time;
};

struct Unit {
    int enabled;
    int info; /* used as a reference to a UnitInfo */
    int group;
    int selected;
    enum Side side; /* friendly or enemy */
    int x;
    int y;
    short int guard_tile;
    short int retreat_tile;
    int armour;
    short int ore;
    short int contains; /* used as a reference to either a unit or structure */
    enum Positioned unit_positioned;
    enum Positioned turret_positioned;
    enum UnitLogic logic; /* long-term logic, so to speak */
    int logic_aid; /* position or unit for long-term logic */
    enum UnitMove move; /* specifies the move the unit needs to make - needs to be done before other actions can be performed */
    int move_aid;
    #ifdef REMOVE_ASTAR_PATHFINDING
    int hugging_obstacle; /* < 0 means keeping the obstacle on the right, > 0 on the left */
    #endif
    int reload_time; /* the amount of time (in frames) left until next shot can be made */
    int misfire_time; // frames
    int smoke_time; /* the amount of time (in frames) left until smoke stops being emitted. doubles as idle_time! :O */
    int multiplayer_id;
};


void initUnits();
void initUnitsWithScenario();

int getUnitNameInfo(char *buffer);

int freeToMoveUnitViaTile(struct Unit *current, int tile);
int getNextTile(int curX, int curY, enum UnitMove move);
int unitMovingOntoTileBesides(int tile, int exceptionUnitNr);
int isTileBlockedByOtherUnit(struct Unit *current, int tile);

int dropUnit(int unitnr, int x, int y);
int addUnit(enum Side side, int x, int y, int info, int forced);

void drawUnitWithShift(int unitnr, int x_add, int y_add);
void drawUnit(int unitnr);
void drawUnits();
void loadUnitsGraphicsBG(int baseBg, int *offsetBg);
void loadUnitsGraphicsSprites(int *offsetSp);

void doUnitDeath(int unitnr);
void doUnitsLogic();
void doUnitLogicHealing(int nr, int projectileSourceTile);
void doUnitLogicHit(int nr, int projectileSourceTile);

void doUnitsSelectAll();

int unitTouchableOnTileCoordinates(int unitnr, int xtouch, int ytouch);

int getFocusOnUnitNr();
void setFocusOnUnitNr(int nr);

extern struct UnitInfo unitInfo[];
extern struct Unit unit[];

struct UnitReinforcement *getUnitsReinforcements();
int getUnitsSaveSize(void);
int getUnitsReinforcementsSaveSize(void);
int getUnitsSaveData(void *dest, int max_size);
int getUnitsReinforcementsSaveData(void *dest, int max_size);

#endif
