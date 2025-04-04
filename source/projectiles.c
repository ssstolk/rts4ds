// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "projectiles.h"

#include "fileio.h"
#include "view.h"
#include "sprites.h"
#include "environment.h"
#include "overlay.h"
#include "structures.h"
#include "units.h"
#include "explosions.h"
#include "radar.h"
#include "factions.h"
#include "soundeffects.h"
#include "rumble.h"

struct ProjectileInfo projectileInfo[MAX_DIFFERENT_PROJECTILES];
struct Projectile projectile[MAX_PROJECTILES_ON_MAP];

unsigned int projectileImpassableOverEnvironment[(NUMBER_OF_ENVIRONMENT_TILE_GRAPHICS + 31) / 32];

#define MAX_RADIUS_PROJECTILE_FOR_SINGLE_STRUCTURE_HIT  10

static int projectileStructureHit[MAX_RADIUS_PROJECTILE_FOR_SINGLE_STRUCTURE_HIT * MAX_RADIUS_PROJECTILE_FOR_SINGLE_STRUCTURE_HIT];
static int projectileStructureHitAmount;



void initProjectiles() {
    char oneline[256];
    int amountOfProjectiles = 0;
    int i, j;
    FILE *fp;
    
    // get the names of all available projectiles
    fp = openFile("projectiles.ini", FS_PROJECT_FILE);
    readstr(fp, oneline);
    sscanf(oneline, "AMOUNT-OF-PROJECTILES %i", &amountOfProjectiles);
    if (amountOfProjectiles > MAX_DIFFERENT_PROJECTILES)
        errorSI("Too many types of projectiles exist. Limit is:", MAX_DIFFERENT_PROJECTILES);
    for (i=0; i<amountOfProjectiles; i++) {
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 255);
        strcpy(projectileInfo[i].name, oneline);
        projectileInfo[i].enabled = 1;
    }
    closeFile(fp);
    
    // get information on each and every available projectile
    for (i=0; i<amountOfProjectiles; i++) {
        fp = openFile(projectileInfo[i].name, FS_PROJECTILES_INFO);
        
        // GENERAL section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[GENERAL]", strlen("[GENERAL]")));
        readstr(fp, oneline);
        readstr(fp, oneline);
        if (!strncmp(oneline + strlen("Type="), "Shot", strlen("Shot")))
            projectileInfo[i].type = PT_SHOT;
        else if (!strncmp(oneline + strlen("Type="), "Aerial Shot", strlen("Aerial Shot")))
            projectileInfo[i].type = PT_AERIAL_SHOT;
        else if (!strncmp(oneline + strlen("Type="), "Bullet", strlen("Bullet")))
            projectileInfo[i].type = PT_BULLET;
        else if (!strncmp(oneline + strlen("Type="), "Aerial Bullet", strlen("Aerial Bullet")))
            projectileInfo[i].type = PT_AERIAL_BULLET;
        else if (!strncmp(oneline + strlen("Type="), "Rocket", strlen("Rocket")))
            projectileInfo[i].type = PT_ROCKET;
        else if (!strncmp(oneline + strlen("Type="), "Aerial Rocket", strlen("Aerial Rocket")))
            projectileInfo[i].type = PT_AERIAL_ROCKET;
        else if (!strncmp(oneline + strlen("Type="), "Shell", strlen("Shell")))
            projectileInfo[i].type = PT_SHELL;
        else if (!strncmp(oneline + strlen("Type="), "Aerial Shell", strlen("Aerial Shell")))
            projectileInfo[i].type = PT_AERIAL_SHELL;
        else {
            replaceEOLwithEOF(oneline, 255);
            error("Projectile had unrecognised type called:", oneline + strlen("Type="));
        }
        
        // MEASUREMENTS section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[MEASUREMENTS]", strlen("[MEASUREMENTS]")));
        readstr(fp, oneline);
        sscanf(oneline, "Size=%i", &j);
        projectileInfo[i].graphics_size = (unsigned int) j;
        
        // EXPLOSION section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[EXPLOSION]", strlen("[EXPLOSION]")));
        readstr(fp, oneline);
        projectileInfo[i].explosion_info = -1;
        for (j=0; j<MAX_DIFFERENT_EXPLOSIONS && explosionInfo[j].enabled; j++) {
            if (!strncmp(oneline + strlen("Explosion="), explosionInfo[j].name, strlen(explosionInfo[j].name))) {
                projectileInfo[i].explosion_info = j;
                break;
            }
        }
        if (projectileInfo[i].type == PT_SHELL || projectileInfo[i].type == PT_AERIAL_SHELL) {
            readstr(fp, oneline);
            sscanf(oneline, "Radius=%i", &projectileInfo[i].explosion_radius);
        }
        
        // FORCE section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[FORCE]", strlen("[FORCE]")));
        readstr(fp, oneline);
        sscanf(oneline, "Power=%i", &projectileInfo[i].power);
        
        // MOVEMENT section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[MOVEMENT]", strlen("[MOVEMENT]")));
        readstr(fp, oneline);
        sscanf(oneline, "Speed=%i", &projectileInfo[i].original_speed);
        if (projectileInfo[i].original_speed > 60)
            error("Found a projectile speed > 60", "");
        projectileInfo[i].original_speed = (projectileInfo[i].original_speed * SCREEN_REFRESH_RATE) / FPS;
        
        // EFFECT sections
        for (j=0; j<EME_COUNT; j++)
            projectileInfo[i].effectModifier[j] = EMT_NORMAL;
        
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[EFFECT", strlen("[EFFECT")));
        for (j=EMT_NIL; j<=EMT_INSTANT; j++) {
            readstr(fp, oneline);
            while (oneline[0] != '[') {
                // "Friendly", "Enemy", "Structures", "Units, Foot", "Units, Wheeled", "Units, Tracked"
                if (oneline[0] == 'F')
                    projectileInfo[i].effectModifier[EME_FRIENDLY] = j;
                else if (oneline[0] == 'E')
                    projectileInfo[i].effectModifier[EME_ENEMY] = j;
                else if (oneline[0] == 'S')
                    projectileInfo[i].effectModifier[EME_STRUCTURES] = j;
                else if (oneline[7] == 'F')
                    projectileInfo[i].effectModifier[EME_UNIT_TYPES + UT_FOOT] = j;
                else if (oneline[7] == 'W')
                    projectileInfo[i].effectModifier[EME_UNIT_TYPES + UT_WHEELED] = j;
                else if (oneline[7] == 'T')
                    projectileInfo[i].effectModifier[EME_UNIT_TYPES + UT_TRACKED] = j;
                else if (oneline[7] == 'A')
                    projectileInfo[i].effectModifier[EME_UNIT_TYPES + UT_AERIAL] = j;
                else
                    error("A flawed effect-entry found:", oneline);
                readstr(fp, oneline);
            }
        }
        
        closeFile(fp);
    }
    for (i=amountOfProjectiles; i<MAX_DIFFERENT_PROJECTILES; i++) // setting all unused ones to disabled
        projectileInfo[i].enabled = 0;
}

void initProjectilesSpeed() {
    int i;
    int gameSpeed = getGameSpeed();
    
    for (i=0; i<MAX_DIFFERENT_PROJECTILES && projectileInfo[i].enabled; i++)
        projectileInfo[i].speed = (projectileInfo[i].original_speed * gameSpeed) / 2;
}

void initProjectilesWithScenario() {
    int i;
    struct Projectile *curProjectile = projectile;
    
    for (i=0; i<MAX_PROJECTILES_ON_MAP; i++, curProjectile++)
        curProjectile->enabled = 0;
    
    initProjectilesSpeed();
}


// has some tan(X) * 1000 values hardcoded.
enum ProjectilePositioned positionProjectileToFace(int curX, int curY, int tgtX, int tgtY) {
    int diffX = tgtX - curX;
    int diffY = tgtY - curY;
    
    if (diffX == 0) {
        if (diffY >= 0)
            return PP_DOWN;
        return PP_UP;
    }
    
    int asc1000 = (abs(diffY) * 1000) / abs(diffX);
    int section = 0;
    if (asc1000 > 199) { // section = 0;
        if (asc1000 <= 668) section = 1;
        else if (asc1000 <= 1497) section = 2;
        else if (asc1000 <= 5027) section = 3;
        else section = 4;
    }
    
    if (diffY < 0) // upper half
        return (diffX < 0) ?
                ((PP_LEFT + section) % (PP_LEFT_UP_UP + 1)) : // upper-left part
                (PP_RIGHT - section); // upper-right part
    // lower half
    return (diffX < 0) ?
            (PP_LEFT - section) : // lower-left part
            (PP_RIGHT + section); // lower-right part
}

int addProjectile(enum Side side, int curX, int curY, int tgtX, int tgtY, int info, int shot) {
    int i;
    int absDX, absDY;
    struct Projectile *curProjectile = projectile;
    
    if (info < 0) // a unit or structure which has no weapon tried to create a new projectile
        return -1;
    
    for (i=0; i<MAX_PROJECTILES_ON_MAP; i++, curProjectile++) {
        if (!curProjectile->enabled) {
            curProjectile->enabled = 1;
            curProjectile->info = info;
            curProjectile->side = side;
            if (projectileInfo[info].type >= PT_ROCKET)
                curProjectile->positioned = positionProjectileToFace(curX, curY, tgtX, tgtY);
            curProjectile->src_x = curX * 16 + 8;
            curProjectile->src_y = curY * 16 + 8;
            curProjectile->tgt_x = tgtX * 16 + 8;
            curProjectile->tgt_y = tgtY * 16 + 8;
            curProjectile->x = curProjectile->src_x;
            curProjectile->y = curProjectile->src_y;
            absDX = abs(tgtX - curX) * 16;
            absDY = abs(tgtY - curY) * 16;
            curProjectile->time_required = square_root(absDX*absDX + absDY*absDY) / projectileInfo[info].speed;
            curProjectile->timer = 0;
            playSoundeffectControlled(SE_PROJECTILE + 2*info + shot, volumePercentageOfLocation(curX, curY, DISTANCE_TO_HALVE_SE_PROJECTILE), soundeffectPanningOfLocation(curX, curY));
            
            if (curX >= getViewCurrentX() && curX < getViewCurrentX() + HORIZONTAL_WIDTH &&
                curY >= getViewCurrentY() && curY < getViewCurrentY() + HORIZONTAL_HEIGHT)
                addRumble(5, 1);
            
            return i;
        }
    }
    return -1;
}



void drawProjectiles() {
    int i;
    int x, y, graphics;
    int hflip, vflip;
    int lowX, highX, lowY, highY;
    struct Projectile *curProjectile = projectile;
    struct ProjectileInfo *curProjectileInfo;
    
    lowX  = getViewCurrentX() * 16;
    highX = (getViewCurrentX() + HORIZONTAL_WIDTH) * 16;
    lowY  = getViewCurrentY() * 16;
    highY = (getViewCurrentY() + HORIZONTAL_HEIGHT) * 16;
    
    for (i=0; i<MAX_PROJECTILES_ON_MAP; i++, curProjectile++) {
        if (curProjectile->enabled && projectileInfo[curProjectile->info].graphics_size > 0 &&
            curProjectile->x >= lowX && curProjectile->x < highX &&
            curProjectile->y >= lowY && curProjectile->y < highY) {
            // bullet needs to be displayed
            curProjectileInfo = &projectileInfo[curProjectile->info];
            graphics = curProjectileInfo->graphics_offset;
            hflip = 0;
            vflip = 0;
            x =  (curProjectile->x - lowX) - (8 << curProjectileInfo->graphics_size) / 2;
            y = ((curProjectile->y - lowY) - (8 << curProjectileInfo->graphics_size) / 2) + ACTION_BAR_SIZE * 16;
            if (curProjectileInfo->type >= PT_ROCKET) {
                // different directions: up, right-up-up, right-up, right-right-up, right
                if (curProjectile->positioned <= PP_RIGHT)
                    graphics += math_power(4, (int) curProjectileInfo->graphics_size - 1) * ((int) curProjectile->positioned);
                else if (curProjectile->positioned <= PP_DOWN) {
                    vflip = 1;
                    graphics += math_power(4, (int) curProjectileInfo->graphics_size - 1) * (PP_DOWN - ((int) curProjectile->positioned));
                } else if (curProjectile->positioned <= PP_LEFT) {
                    vflip = 1;
                    hflip = 1;
                    graphics += math_power(4, (int) curProjectileInfo->graphics_size - 1) * (((int) curProjectile->positioned) - PP_DOWN);
                } else {
                    hflip = 1;
                    graphics += math_power(4, (int) curProjectileInfo->graphics_size - 1) * ((PP_LEFT_UP_UP + 1) - ((int) curProjectile->positioned));
                }
            }
            setSpritePlayScreen(y, ATTR0_SQUARE,
                                x, curProjectileInfo->graphics_size, hflip&1, vflip&1,
                                2, PS_SPRITES_PAL_PROJECTILES, graphics);
        }
    }
}


void doProjectileImpact(struct Projectile *curProjectile, struct ProjectileInfo *curProjectileInfo, struct Structure *structure, struct StructureInfo *structureInfo, struct Unit *unit, struct UnitInfo *unitInfo) {
    struct EnvironmentLayout *environmentLayout;
    enum EffectModifierType effectModifier;
    int structureId, unitId;
    int damage;
    int j;
    int tile = TILE_FROM_XY(curProjectile->x / 16, curProjectile->y / 16);
    
    environmentLayout = &environment.layout[tile];
    unitId = environmentLayout->contains_unit;
    if (unitId >= 0) { // hit unit
        if (unit[unitId].armour > 0 && unit[unitId].armour != INFINITE_ARMOUR) {
            damage = curProjectileInfo->power;
            effectModifier = curProjectileInfo->effectModifier[!unit[unitId].side != !curProjectile->side];
            if (effectModifier == EMT_NORMAL)
                effectModifier = curProjectileInfo->effectModifier[EME_UNIT_TYPES + unitInfo[unit[unitId].info].type];
            switch (effectModifier) {
                case EMT_NORMAL:
                    break;
                case EMT_NIL:
                    damage = 0;
                    break;
                case EMT_DIMINISHED:
                    damage = (damage * 70) / 100;
                    break;
                case EMT_INCREASED:
                    damage = (damage * 130) / 100;
                    break;
                case EMT_INSTANT:
                    damage *= unitInfo[unit[unitId].info].max_armour;
            }
            unit[unitId].armour -= damage;
            if (curProjectileInfo->power < 0) { // healing effect
                if (unit[unitId].armour > unitInfo[unit[unitId].info].max_armour)
                    unit[unitId].armour = unitInfo[unit[unitId].info].max_armour;
                if (unit[unitId].armour >= unitInfo[unit[unitId].info].max_armour / 4)
                    unit[unitId].smoke_time = 0;
                doUnitLogicHealing(unitId, TILE_FROM_XY(curProjectile->src_x / 16, curProjectile->src_y / 16));
            } else { // destroying effect
                
                displayHitRadar(curProjectile->x / 16, curProjectile->y / 16);
                
                if (unit[unitId].armour <= 0 && unitInfo[unit[unitId].info].type == UT_FOOT)
                    addOverlay(curProjectile->x / 16, curProjectile->y / 16, OT_DEAD_FOOT, 0);
                else {
                    if (unit[unitId].armour < unitInfo[unit[unitId].info].max_armour / 4)
                        unit[unitId].smoke_time = UNIT_SMOKE_DURATION + (unit[unitId].smoke_time % (3 * UNIT_SINGLE_SMOKE_DURATION));
                    doUnitLogicHit(unitId, TILE_FROM_XY(curProjectile->src_x / 16, curProjectile->src_y / 16));
                }
                if (curProjectile->x/16 >= getViewCurrentX() && curProjectile->x/16 < getViewCurrentX() + HORIZONTAL_WIDTH && curProjectile->y/16 >= getViewCurrentY() && curProjectile->y/16 < getViewCurrentY() + HORIZONTAL_HEIGHT)
                    playSoundeffect(SE_IMPACT_UNIT);
            }
        }
        curProjectile->enabled = 0;
    } else if (environmentLayout->contains_structure != -1) { // might have hit a structure
        structureId = environmentLayout->contains_structure;
        if (structureId < -1) // it was just a reference to a structure ID
            structureId = (environmentLayout + (structureId + 1))->contains_structure;
        if (curProjectileInfo->type >= PT_ROCKET || !structureInfo[structure[structureId].info].foundation) { // hit a structure
            if (curProjectileInfo->type < PT_ROCKET) {
                int originStructure = environment.layout[TILE_FROM_XY(curProjectile->src_x / 16, curProjectile->src_y / 16)].contains_structure;
                if (originStructure < -1)
                    originStructure = environment.layout[TILE_FROM_XY(curProjectile->src_x / 16, curProjectile->src_y / 16) + (originStructure + 1)].contains_structure;
                if (originStructure >= 0 && structure[originStructure].side == structure[structureId].side)
                    return;
            }
            if (curProjectileInfo->type >= PT_SHELL && curProjectileInfo->explosion_radius <= MAX_RADIUS_PROJECTILE_FOR_SINGLE_STRUCTURE_HIT) {
                for (j=0; j<projectileStructureHitAmount && projectileStructureHit[j] != structureId; j++);
                if (j < projectileStructureHitAmount)
                    return;
                projectileStructureHit[projectileStructureHitAmount] = structureId;
                projectileStructureHitAmount++;
            }
            if (structure[structureId].armour > 0 && structure[structureId].armour != INFINITE_ARMOUR) {
                damage = curProjectileInfo->power;
                effectModifier = curProjectileInfo->effectModifier[!structure[structureId].side != !curProjectile->side];
                if (effectModifier == EMT_NORMAL)
                    effectModifier = curProjectileInfo->effectModifier[EME_STRUCTURES];
                switch (effectModifier) {
                    case EMT_NORMAL:
                        break;
                    case EMT_NIL:
                        damage = 0;
                        break;
                    case EMT_DIMINISHED:
                        damage = (damage * 70) / 100;
                        break;
                    case EMT_INCREASED:
                        damage = (damage * 130) / 100;
                        break;
                    case EMT_INSTANT:
                        damage *= structureInfo[structure[structureId].info].max_armour;
                }
                structure[structureId].armour -= damage;
                if (curProjectileInfo->power < 0) { // healing effect
                    if (structure[structureId].armour > structureInfo[structure[structureId].info].max_armour)
                        structure[structureId].armour = structureInfo[structure[structureId].info].max_armour;
                    if (structure[structureId].armour >= structureInfo[structure[structureId].info].max_armour / 2)
                        structure[structureId].smoke_time = 0;
                } else { // destroying effect
                    
                    displayHitRadar(curProjectile->x / 16, curProjectile->y / 16);
                    
                    if (structureInfo[structure[structureId].info].foundation) { // a hack for foundations, used for optimization. not as pretty but necessary.
                        if (structure[structureId].armour <= 0) {
                            // any unit attacking this structure should stop
                            for (j=0; j<MAX_UNITS_ON_MAP; j++) {
                                if (unit[j].enabled && (unit[j].logic == UL_ATTACK_LOCATION || unit[j].logic == UL_ATTACK_AREA) && unit[j].logic_aid == tile) {
                                    unit[j].logic = UL_GUARD;
                                    if (getGameType() == SINGLEPLAYER) {
                                        if (unit[j].side != FRIENDLY) {
                                            if (unit[j].group == UGAI_HUNT) {
                                                unit[j].logic = UL_HUNT;
                                                unit[j].logic_aid = 0;
                                            } else if (unit[j].group == UGAI_KAMIKAZE)
                                                unit[j].logic = UL_KAMIKAZE;
                                        }
                                    }
                                }
                            }
                            // remove it from map
                            environment.layout[tile].contains_structure = -1;
                            setTileDirtyRadarDirtyBitmap(tile);
                            addExplosion(curProjectile->x / 16, curProjectile->y / 16, structureInfo[structure[structureId].info].destroyed_explosion_info, 0);
                            // set it back to existing again for reuse by other locations
                            structure[structureId].armour = 1;
                        }
                    } else {
                        if (!structureInfo[structure[structureId].info].barrier) { // not foundation, not barrier
                            if (structure[structureId].armour < structureInfo[structure[structureId].info].max_armour / 2)
                                structure[structureId].smoke_time = STRUCTURE_SMOKE_DURATION + (structure[structureId].smoke_time % (3 * STRUCTURE_SINGLE_SMOKE_DURATION));
                        }
                        doStructureLogicHit(structureId, TILE_FROM_XY(curProjectile->src_x / 16, curProjectile->src_y / 16));
                        if (curProjectile->x/16 >= getViewCurrentX() && curProjectile->x/16 < getViewCurrentX() + HORIZONTAL_WIDTH && curProjectile->y/16 >= getViewCurrentY() && curProjectile->y/16 < getViewCurrentY() + HORIZONTAL_HEIGHT)
                            playSoundeffect(SE_IMPACT_STRUCTURE);
                    }
                }
            }
            curProjectile->enabled = 0;
        }
    } else { // maybe sand or rock was hit?
/*        if (environmentLayout->graphics >= ROCKCHASM && environmentLayout->graphics <= ROCKCHASM16)
            curProjectile->enabled = 0;
        else*/
        if (curProjectileInfo->type == PT_ROCKET || curProjectileInfo->type == PT_AERIAL_ROCKET) {
            if (/*environmentLayout->graphics >= SAND &&*/ environmentLayout->graphics <= SANDHILL16 || (environmentLayout->graphics >= ORE && environmentLayout->graphics <= OREHILL16))
                addOverlay(curProjectile->x / 16, curProjectile->y / 16, OT_SAND_SHOT, 0);
            else if (environmentLayout->graphics >= ROCK && environmentLayout->graphics <= ROCKCUSTOM32)
                addOverlay(curProjectile->x / 16, curProjectile->y / 16, OT_ROCK_SHOT, 0);
            else if ((environmentLayout->graphics >= SANDCHASM && environmentLayout->graphics <= SANDCHASM16) || (environmentLayout->graphics >= ROCKCHASM && environmentLayout->graphics <= ROCKCHASM16) || environmentLayout->graphics >= CHASMCUSTOM)
                playSoundeffectControlled(SE_IMPACT_SAND, volumePercentageOfLocation(curProjectile->x / 16, curProjectile->y / 16, DISTANCE_TO_HALVE_SE_IMPACT_SAND), soundeffectPanningOfLocation(curProjectile->x / 16, curProjectile->y / 16));
            curProjectile->enabled = 0;
        }
    }
}


void setProjectileImpassableOverEnvironment(unsigned graphics, unsigned int impassable) {
    if (impassable)
        projectileImpassableOverEnvironment[graphics / 32] |=   BIT(graphics % 32);
    else
        projectileImpassableOverEnvironment[graphics / 32] &= (~BIT(graphics % 32));
}


inline unsigned int getProjectileImpassableOverEnvironment(enum EnvironmentTileGraphics graphics) {
    return projectileImpassableOverEnvironment[graphics / 32] & BIT(graphics % 32);
}



int isClearPathProjectile(int curX, int curY, int projectile_info, int tgtX, int tgtY) {
    // no projectile never affected code before, shouldn't now either. stating it as clear path.
    if (projectile_info <= -1)
        return 1;
    
    // aerial projectiles always have a clear path
    struct ProjectileInfo *curProjectileInfo = projectileInfo + projectile_info;
    if (curProjectileInfo->type & 1)
        return 1;
    
    int absDX = abs(tgtX - curX) * 16;
    int absDY = abs(tgtY - curY) * 16;
    
    int timer;
    int time_required = square_root(absDX*absDX + absDY*absDY) / curProjectileInfo->speed;
    
    struct EnvironmentLayout *envTile;
    int done;
    int step;
    int x, y;
    int xdiff, ydiff;
    int structureId;
    
    envTile = environment.layout + TILE_FROM_XY(tgtX, tgtY);
    int targetStructure = envTile->contains_structure;
    if (targetStructure < -1)
        targetStructure = (envTile + (targetStructure + 1))->contains_structure;
    
    envTile = environment.layout + TILE_FROM_XY(curX, curY);
    enum Side side;
    int originStructure = envTile->contains_unit < 0; // used as boolean here
    if (!originStructure)
        side = unit[envTile->contains_unit].side;
    else {
        if (envTile->contains_structure < -1)
            side = structure[(envTile + (envTile->contains_structure + 1))->contains_structure].side;
        else
            side = structure[envTile->contains_structure].side;
    }
    
    curX = curX*16 + 8;
    curY = curY*16 + 8;
    tgtX = tgtX*16 + 8;
    tgtY = tgtY*16 + 8;
    
    for (timer=1; timer<time_required; timer++) {
        
        step = 1; // multiple steps might be needed to have proper colission detection on each tile 
        do {
            done = 1; // assuming the current (colission) step will suffice for the current projectile
            xdiff = ((tgtX - curX) * timer) / time_required;
            ydiff = ((tgtY - curY) * timer) / time_required;
            if (abs(xdiff) > abs(ydiff)) {
                if (abs(xdiff) > step * PROJECTILE_MAXIMUM_COLISSION_STEP) {
                    if (ydiff >= 0)
                        ydiff = (ydiff * (step * PROJECTILE_MAXIMUM_COLISSION_STEP)) / abs(xdiff);
                    else
                        ydiff = - ((abs(ydiff) * (step * PROJECTILE_MAXIMUM_COLISSION_STEP)) / abs(xdiff));
                    if (xdiff >= 0)
                        xdiff = step * PROJECTILE_MAXIMUM_COLISSION_STEP;
                    else
                        xdiff = - step * PROJECTILE_MAXIMUM_COLISSION_STEP;
                    done = 0;
                }
            } else {
                if (abs(ydiff) > step * PROJECTILE_MAXIMUM_COLISSION_STEP) {
                    if (xdiff >= 0)
                        xdiff = (xdiff * (step * PROJECTILE_MAXIMUM_COLISSION_STEP)) / abs(ydiff);
                    else
                        xdiff = - ((abs(xdiff) * (step * PROJECTILE_MAXIMUM_COLISSION_STEP)) / abs(ydiff));
                    if (ydiff >= 0)
                        ydiff = step * PROJECTILE_MAXIMUM_COLISSION_STEP;
                    else
                        ydiff = - step * PROJECTILE_MAXIMUM_COLISSION_STEP;
                    done = 0;
                }
            }
            x = curX + xdiff;
            y = curY + ydiff;
            
            if (x/16 == tgtX/16 && y/16 == tgtY/16)
                return 1;
            
            if (timer >= time_required && done)
                return 1;
            
            envTile = environment.layout + TILE_FROM_XY(x/16, y/16);
            if (getProjectileImpassableOverEnvironment(envTile->graphics))
                return 0;
            
            if (targetStructure >= MAX_DIFFERENT_FACTIONS) { // target structure exists and is not foundation
                if (envTile->contains_structure == targetStructure)
                    return 1;
                if (envTile->contains_structure < -1 &&
                    (envTile + envTile->contains_structure + 1)->contains_structure == targetStructure)
                    return 1;
            }
            
            if ((curProjectileInfo->type == PT_SHOT || curProjectileInfo->type == PT_BULLET) &&
                (envTile->contains_unit <= -1 || (!unit[envTile->contains_unit].side != !side))) {
                
                if (envTile->contains_unit >= 0) // hit an enemy unit, we're fine with that. considered clear.
                    return 1;
                
                if (envTile->contains_structure != -1) { // might have hit a structure
                    structureId = envTile->contains_structure;
                    if (structureId < -1) // it was just a reference to a structure ID
                        structureId = (envTile + (structureId + 1))->contains_structure;
                    if (curProjectileInfo->type >= PT_ROCKET || !structureInfo[structure[structureId].info].foundation) { // hit a structure
                        if (curProjectileInfo->type < PT_ROCKET) {
                            int originStructure = environment.layout[TILE_FROM_XY(curX/16, curY/16)].contains_structure;
                            if (originStructure < -1)
                                originStructure = environment.layout[TILE_FROM_XY(curX/16 + (originStructure + 1), curY/16)].contains_structure;
                            if (!(originStructure >= 0 && structure[originStructure].side == structure[structureId].side))
                                return 0;
                        }
                    }
                }
                
            }
            
            step++;
        } while (!done);
    }
 
    return 1;
}


void doProjectilesLogic() {
    int i, j, k;
    int x, y;
    int xdiff, ydiff;
    int maxsquared;
    int done, step;
    enum EnvironmentTileGraphics environmentTileGraphics;
    struct Projectile *curProjectile = projectile;
    struct ProjectileInfo *curProjectileInfo;
    
    startProfilingFunction("doProjectilesLogic");
    
    for (i=0; i<MAX_PROJECTILES_ON_MAP; i++, curProjectile++) {
        if (curProjectile->enabled) {
            projectileStructureHitAmount = 0;
            curProjectileInfo = projectileInfo + curProjectile->info;
            if (curProjectile->timer < curProjectile->time_required)
                curProjectile->timer++;
            step = 1; // multiple steps might be needed to have proper colission detection on each tile 
            do {
                done = 1; // assuming the current (colission) step will suffice for the current projectile
                xdiff = ((curProjectile->tgt_x - curProjectile->src_x) * curProjectile->timer) / curProjectile->time_required;
                ydiff = ((curProjectile->tgt_y - curProjectile->src_y) * curProjectile->timer) / curProjectile->time_required;
                if (abs(xdiff) > abs(ydiff)) {
                    if (abs(xdiff) > step * PROJECTILE_MAXIMUM_COLISSION_STEP) {
                        if (ydiff >= 0)
                            ydiff = (ydiff * (step * PROJECTILE_MAXIMUM_COLISSION_STEP)) / abs(xdiff);
                        else
                            ydiff = - ((abs(ydiff) * (step * PROJECTILE_MAXIMUM_COLISSION_STEP)) / abs(xdiff));
                        if (xdiff >= 0)
                            xdiff = step * PROJECTILE_MAXIMUM_COLISSION_STEP;
                        else
                            xdiff = - step * PROJECTILE_MAXIMUM_COLISSION_STEP;
                        done = 0;
                    }
                } else {
                    if (abs(ydiff) > step * PROJECTILE_MAXIMUM_COLISSION_STEP) {
                        if (xdiff >= 0)
                            xdiff = (xdiff * (step * PROJECTILE_MAXIMUM_COLISSION_STEP)) / abs(ydiff);
                        else
                            xdiff = - ((abs(xdiff) * (step * PROJECTILE_MAXIMUM_COLISSION_STEP)) / abs(ydiff));
                        if (ydiff >= 0)
                            ydiff = step * PROJECTILE_MAXIMUM_COLISSION_STEP;
                        else
                            ydiff = - step * PROJECTILE_MAXIMUM_COLISSION_STEP;
                        done = 0;
                    }
                }
                curProjectile->x = curProjectile->src_x + xdiff;
                curProjectile->y = curProjectile->src_y + ydiff;
                
                environmentTileGraphics = environment.layout[TILE_FROM_XY(curProjectile->x / 16, curProjectile->y / 16)].graphics;
                if ((curProjectile->timer >= curProjectile->time_required && done) || (!(curProjectileInfo->type & 1) && getProjectileImpassableOverEnvironment(environmentTileGraphics))) {
                    // projectile reached its target position
                    doProjectileImpact(curProjectile, curProjectileInfo, structure, structureInfo, unit, unitInfo);
                    addExplosion(curProjectile->x, curProjectile->y, curProjectileInfo->explosion_info, 0);
                    if (curProjectileInfo->type >= PT_SHELL) {
                        x = curProjectile->x;
                        y = curProjectile->y;
                        for (k=1; k<=curProjectileInfo->explosion_radius; k++) {
                            if ((curProjectile->x = x - (k*16)) > 0) {
                                doProjectileImpact(curProjectile, curProjectileInfo, structure, structureInfo, unit, unitInfo);
                                addExplosion(curProjectile->x, curProjectile->y, curProjectileInfo->explosion_info, rand()%6);
                            }
                            if ((curProjectile->x = x + (k*16)) < environment.width * 16) {
                                doProjectileImpact(curProjectile, curProjectileInfo, structure, structureInfo, unit, unitInfo);
                                addExplosion(curProjectile->x, curProjectile->y, curProjectileInfo->explosion_info, rand()%6);
                            }
                        }
                        for (j=1; j<=curProjectileInfo->explosion_radius; j++) {
                            if ((curProjectile->y = y - (j*16)) > 0) {
                                curProjectile->x = x;
                                doProjectileImpact(curProjectile, curProjectileInfo, structure, structureInfo, unit, unitInfo);
                                addExplosion(curProjectile->x, curProjectile->y, curProjectileInfo->explosion_info, rand()%6);
                                maxsquared = curProjectileInfo->explosion_radius*curProjectileInfo->explosion_radius - j*j;
                                for (k=1; k*k <= maxsquared; k++) {
                                    if ((curProjectile->x = x - (k*16)) > 0) {
                                        doProjectileImpact(curProjectile, curProjectileInfo, structure, structureInfo, unit, unitInfo);
                                        addExplosion(curProjectile->x, curProjectile->y, curProjectileInfo->explosion_info, rand()%6);
                                    }
                                    if ((curProjectile->x = x + (k*16)) < environment.width * 16) {
                                        doProjectileImpact(curProjectile, curProjectileInfo, structure, structureInfo, unit, unitInfo);
                                        addExplosion(curProjectile->x, curProjectile->y, curProjectileInfo->explosion_info, rand()%6);
                                    }
                                }
                            }
                            if ((curProjectile->y = y + (j*16)) < environment.height * 16) {
                                curProjectile->x = x;
                                doProjectileImpact(curProjectile, curProjectileInfo, structure, structureInfo, unit, unitInfo);
                                addExplosion(curProjectile->x, curProjectile->y, curProjectileInfo->explosion_info, rand()%6);
                                maxsquared = curProjectileInfo->explosion_radius*curProjectileInfo->explosion_radius - j*j;
                                for (k=1; k*k <= maxsquared; k++) {
                                    if ((curProjectile->x = x - (k*16)) > 0) {
                                        doProjectileImpact(curProjectile, curProjectileInfo, structure, structureInfo, unit, unitInfo);
                                        addExplosion(curProjectile->x, curProjectile->y, curProjectileInfo->explosion_info, rand()%6);
                                    }
                                    if ((curProjectile->x = x + (k*16)) < environment.width * 16) {
                                        doProjectileImpact(curProjectile, curProjectileInfo, structure, structureInfo, unit, unitInfo);
                                        addExplosion(curProjectile->x, curProjectile->y, curProjectileInfo->explosion_info, rand()%6);
                                    }
                                }
                            }
                        }
                    }
                    curProjectile->enabled = 0;
                    done = 1;
                } else if ((curProjectileInfo->type == PT_SHOT || curProjectileInfo->type == PT_BULLET) && (abs(curProjectile->x - curProjectile->src_x) >= 16 || abs(curProjectile->y - curProjectile->src_y) >= 16) &&
                            (environment.layout[TILE_FROM_XY(curProjectile->x/16, curProjectile->y/16)].contains_unit <= -1 || (!unit[environment.layout[TILE_FROM_XY(curProjectile->x/16, curProjectile->y/16)].contains_unit].side != !curProjectile->side))) {
                    doProjectileImpact(curProjectile, curProjectileInfo, structure, structureInfo, unit, unitInfo);
                    if (!curProjectile->enabled) { // it hit something
                        addExplosion(curProjectile->x, curProjectile->y, curProjectileInfo->explosion_info, 0);
                        done = 1;
                    }
                }
                
                step++;
            } while (!done);
        }
    }
    
    stopProfilingFunction();
}

void loadProjectilesGraphicsBG(int baseBg, int *offsetBg) {
}

void loadProjectilesGraphicsSprites(int *offsetSp) {
    int i;

    for (i=0; i<MAX_DIFFERENT_PROJECTILES && projectileInfo[i].enabled; i++) {
        projectileInfo[i].graphics_offset = (*offsetSp)/(16*16);
        *offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + ((*offsetSp)>>1), projectileInfo[i].name, FS_PROJECTILES_GRAPHICS);
    }
}


int getProjectilesSaveSize(void) {
    return sizeof(projectile);
}

int getProjectilesSaveData(void *dest, int max_size) {
    int size=sizeof(projectile);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&projectile,size);
    return (size);
}
