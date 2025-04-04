// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _PROJECTILES_H_
#define _PROJECTILES_H_

#include <nds.h>

#include "settings.h"
#include "shared.h"

#define MAX_DIFFERENT_PROJECTILES   22
#define MAX_PROJECTILE_NAME_LENGTH  30

#define MAX_PROJECTILES_ON_MAP      150 /* usually approximately the number of units */

#define DOUBLE_SHOT_AT               ((2*(FPS/10)) / getGameSpeed())
#define SHOOT_ANIMATION_DURATION    (((2*(FPS/10)) / getGameSpeed()) - 1)
#define SHOOT_ANIMATION_DURATION_FAKED    (((2*(FPS/10)) / ( (getGameSpeed()<3)?getGameSpeed():2 )) - 1)

#define PROJECTILE_MAXIMUM_COLISSION_STEP  16 /* in pixels */


enum ProjectileType { PT_SHOT, PT_AERIAL_SHOT, PT_BULLET, PT_AERIAL_BULLET, PT_ROCKET, PT_AERIAL_ROCKET, PT_SHELL, PT_AERIAL_SHELL };
enum ProjectilePositioned { PP_UP, PP_RIGHT_UP_UP, PP_RIGHT_UP, PP_RIGHT_RIGHT_UP, PP_RIGHT, PP_RIGHT_RIGHT_DOWN, PP_RIGHT_DOWN, PP_RIGHT_DOWN_DOWN, PP_DOWN, PP_LEFT_DOWN_DOWN, PP_LEFT_DOWN, PP_LEFT_LEFT_DOWN, PP_LEFT, PP_LEFT_LEFT_UP, PP_LEFT_UP, PP_LEFT_UP_UP };
enum EffectModifierEntity { EME_FRIENDLY, EME_ENEMY, EME_STRUCTURES, EME_UNIT_TYPES,           EME_COUNT = EME_UNIT_TYPES+4 /* 4 == number of unit types*/};
enum EffectModifierType { EMT_NORMAL, EMT_NIL, EMT_DIMINISHED, EMT_INCREASED, EMT_INSTANT };


struct ProjectileInfo {
    int enabled;
    char name[MAX_PROJECTILE_NAME_LENGTH+1];
    enum ProjectileType type;
    int power;
    int speed;
    int explosion_info;
    int explosion_radius;
    uint16 graphics_size;
    int graphics_offset;
    
    int effectModifier[EME_COUNT];
    
    int original_speed;
};

struct Projectile {
    int enabled;
    int info;
    enum Side side;
    enum ProjectilePositioned positioned;
    int x; // be aware! pixel-precision here for x and y, not tile-precision.
    int y;
    int src_x;
    int src_y;
    int tgt_x;
    int tgt_y;
    int timer;
    int time_required;
};

void setProjectileImpassableOverEnvironment(unsigned graphics, unsigned int impassable);
void initProjectiles();
void initProjectilesWithScenario();
int addProjectile(enum Side side, int curX, int curY, int tgtX, int tgtY, int info, int shot);

void drawProjectiles();
void loadProjectilesGraphicsBG(int baseBg, int *offsetBg);
void loadProjectilesGraphicsSprites(int *offsetSp);

void doProjectilesLogic();

int isClearPathProjectile(int curX, int curY, int projectile_info, int tgtX, int tgtY);

extern struct ProjectileInfo projectileInfo[];
extern struct Projectile projectile[];

int getProjectilesSaveSize(void);
int getProjectilesSaveData(void *dest, int max_size);

#endif
