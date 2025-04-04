// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "playscreen.h"

#include "fileio.h"

#include "infoscreen.h"
#include "game.h"
#include "shared.h"
#include "sprites.h"
#include "view.h"
#include "inputx.h"
#include "factions.h"
#include "environment.h"
#include "overlay.h"
#include "explosions.h"
#include "projectiles.h"
#include "structures.h"
#include "units.h"
#include "info.h"
#include "ai.h"
#include "pathfinding.h"
#include "settings.h"
#include "soundeffects.h"

#define DRAG_SELECTION_THRESHHOLD  8
#define GRAPHICAL_ACTION_DURATION       (((FPS / 2) / 5) * 5)
#define DOUBLE_TAP_THRESHHOLD_RADAR     (FPS)


enum GUIActionIcons { ACTION_ICON_SELECT, ACTION_ICON_CANCEL, ACTION_ICON_ATTACK, ACTION_ICON_MOVE, ACTION_ICON_GUARD, ACTION_ICON_RETREAT, ACTION_ICON_PLACE, ACTION_ICON_REPAIR, ACTION_ICON_PRIMARY, ACTION_ICON_SWAPSCREENS, ACTION_ICON_FOCUS, ACTION_ICON_RALLYPOINT, ACTION_ICON_SELL, ACTION_ICON_HEAL };

enum PlayScreenTouchModifier playScreenTouchModifier = PSTM_NONE;
touchPosition touchPlaceStructure;
int queueNrPlaceStructure;
int sellNrStructure;

enum GraphicalActionIssued { GAI_NONE, GAI_ATTACK_LOCATION, GAI_ATTACK_UNIT, GAI_MOVE_LOCATION, GAI_HEAL_LOCATION, GAI_HEAL_UNIT };
enum GraphicalActionIssued graphicalActionIssued = GAI_NONE;
int graphicalActionIssuedLocation;
int graphicalActionIssuedTimer;

static int base_bar;
static int base_drag_select;
static int base_place_structure;
static int base_action_icons;
static int base_action_icons_tips;
static int base_action_attack;
static int base_action_move;
static int base_action_heal;




void deselectAll() {
    int i;
    
    for (i=0; i<MAX_UNITS_ON_MAP; i++)
        unit[i].selected = 0;
    for (i=0; i<MAX_STRUCTURES_ON_MAP; i++)
        structure[i].selected = 0;
    setFocusOnUnitNr(-1);
}

void setModifierPlaceStructure(int queueNr) {
    deselectAll();
    playScreenTouchModifier = PSTM_PLACE_STRUCTURE;
    touchPlaceStructure.px = 128 - structureInfo[getStructuresQueue(FRIENDLY)[queueNrPlaceStructure].itemInfo].width/2;
    touchPlaceStructure.py = (96 + ACTION_BAR_SIZE * 8) - structureInfo[getStructuresQueue(FRIENDLY)[queueNrPlaceStructure].itemInfo].height/2;
    queueNrPlaceStructure = queueNr;
}

void setModifierNone() {
    playScreenTouchModifier = PSTM_NONE;
}

enum PlayScreenTouchModifier getModifier() {
    return playScreenTouchModifier;
}

int getDeployNrUnit() {
    return queueNrPlaceStructure;
}

int getSellNrStructure() {
    return sellNrStructure;
}



void inputPlayScreenFriendlyForcedAttack(int tile, int forceHeal) {
    int i;
    enum UnitType unitType = 255;
    
    if (environment.layout[tile].contains_unit >= 0 && (environment.layout[tile].contains_structure <= -1 || structureInfo[environment.layout[tile].contains_structure].foundation)) {
        /* attack unit */
        if (getGameType() == SINGLEPLAYER) {
            for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                if (unit[i].selected) {
                    if (!unitInfo[unit[i].info].shoot_range || (!forceHeal && unitInfo[unit[i].info].can_heal_foot)) {
                        unit[i].logic = UL_MOVE_LOCATION;
                        unit[i].logic_aid = tile;
                    } else {
                        unit[i].logic = UL_ATTACK_UNIT;
                        unit[i].logic_aid = environment.layout[tile].contains_unit;
                        unit[i].group = (unit[i].group & (~UGAI_FRIENDLY_MASK)) | UGAI_ATTACK_FORCED;
                    }
                    #ifdef REMOVE_ASTAR_PATHFINDING
                    unit[i].hugging_obstacle = 0;
                    #endif
                    if (unitType == 255 || unitInfo[unit[i].info].type > unitType)
                        unitType = unitInfo[unit[i].info].type;
                }
            }
        }
        graphicalActionIssued = (forceHeal != 0) ? GAI_HEAL_UNIT : GAI_ATTACK_UNIT;
        graphicalActionIssuedLocation = environment.layout[tile].contains_unit;
        graphicalActionIssuedTimer = 0;
    } else {
        /* attack location */
        if (getGameType() == SINGLEPLAYER) {
            for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                if (unit[i].selected) {
                    if (!unitInfo[unit[i].info].shoot_range || (!forceHeal && unitInfo[unit[i].info].can_heal_foot))
                        unit[i].logic = UL_MOVE_LOCATION;
                    else {
                        unit[i].logic = UL_ATTACK_LOCATION;
                        unit[i].group = (unit[i].group & (~UGAI_FRIENDLY_MASK)) | UGAI_ATTACK_FORCED;
                    }
                    unit[i].logic_aid = tile;
                    #ifdef REMOVE_ASTAR_PATHFINDING
                    unit[i].hugging_obstacle = 0;
                    #endif
                    if (unitType == 255 || unitInfo[unit[i].info].type > unitType)
                        unitType = unitInfo[unit[i].info].type;
                }
            }
        }
        graphicalActionIssued = (forceHeal != 0) ? GAI_HEAL_LOCATION : GAI_ATTACK_LOCATION;
        graphicalActionIssuedLocation = tile;
        graphicalActionIssuedTimer = 0;
    }
    playScreenTouchModifier = PSTM_NONE;
    if (unitType == UT_FOOT)
        playSoundeffect(SE_FOOT_ATTACK);
    else if (unitType == UT_WHEELED)
        playSoundeffect(SE_WHEELED_ATTACK);
    else if (unitType == UT_TRACKED)
        playSoundeffect(SE_TRACKED_ATTACK);
    else if (unitType == UT_AERIAL)
        playSoundeffect(SE_TRACKED_ATTACK);
}

void inputPlayScreenFriendlyForcedMove(int tile) {
    int i;
    enum UnitType unitType = 255;
    enum UnitLogic logic;
    int structure_t;
    
    logic = UL_MOVE_LOCATION;
    // if there is a ore refinery structure at the tile and all units can collect ore, they are set to retreat to this structure/tile
    structure_t = environment.layout[tile].contains_structure;
    if (structure_t < -1)
        structure_t = environment.layout[tile - (structure_t + 1)].contains_structure;
    if (structure_t >= MAX_DIFFERENT_FACTIONS && structure[structure_t].side == FRIENDLY && structure[structure_t].armour != INFINITE_ARMOUR) {
        if (structureInfo[structure[structure_t].info].can_extract_ore) {
            // if all units selected can gather ore, move them to the tile.
            for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                if (unit[i].selected && unit[i].side == FRIENDLY && unit[i].enabled && !unitInfo[unit[i].info].can_collect_ore)
                    break;
            }
            if (i == MAX_UNITS_ON_MAP)
                logic = UL_RETREAT; // it is indeed the case that the forced move should be seen as the retreat of "ore collectors"
        }
    }
    
    /* move to location */
    if (getGameType() == SINGLEPLAYER) {
        for (i=0; i<MAX_UNITS_ON_MAP; i++) {
            if (unit[i].selected) {
                unit[i].logic = logic;
                if (logic == UL_RETREAT)
                    unit[i].retreat_tile = tile;
                else
                    unit[i].logic_aid = tile;
                #ifdef REMOVE_ASTAR_PATHFINDING
                unit[i].hugging_obstacle = 0;
                #endif
                if (unitType == 255 || unitInfo[unit[i].info].type > unitType)
                    unitType = unitInfo[unit[i].info].type;
            }
        }
    }
    graphicalActionIssued = GAI_MOVE_LOCATION;
    graphicalActionIssuedLocation = tile;
    graphicalActionIssuedTimer = 0;
    playScreenTouchModifier = PSTM_NONE;
    if (unitType == UT_FOOT)
        playSoundeffect(SE_FOOT_MOVE);
    else if (unitType == UT_WHEELED)
        playSoundeffect(SE_WHEELED_MOVE);
    else if (unitType == UT_TRACKED)
        playSoundeffect(SE_TRACKED_MOVE);
    else if (unitType == UT_AERIAL)
        playSoundeffect(SE_TRACKED_MOVE);
}

void inputPlayScreen() {
    int tile, structure_t, unit_t, intendedTouchTile;
    int i, j/*=-1*/, k;
    int box_width, box_height;
    enum UnitType unitType = 255;
    enum UnitLogic logic;
    int unitnr;
    int forcedAttackAvailable, deployAvailable, healAvailable;
    int xWithinTile, yWithinTile;
    int containsOreCollector = -1;
    int containsOtherUnits   = -1;
    int swapScreens = 0;
    touchPosition touchXY = touchReadLast();
    
    startProfilingFunction("inputPlayScreen");
    
    /* code for touch screen input */
    if (touchReadOrigin().py < ACTION_BAR_SIZE * 16) {
        if (touchXY.py < ACTION_BAR_SIZE * 16) {
            /* control bar input */
            if (getKeysOnUp() & KEY_TOUCH) {
                if (touchXY.px >= 120 && touchXY.px < 136)
                    swapScreens = 1;
                else if (playScreenTouchModifier == PSTM_PLACE_STRUCTURE || playScreenTouchModifier == PSTM_DEPLOY_STRUCTURE) {
                    if (touchXY.px >= SCREEN_WIDTH - 32 && touchXY.px < SCREEN_WIDTH - 16) { // cancel placing structure
                        playScreenTouchModifier = PSTM_NONE;
                        playSoundeffect(SE_BUTTONCLICK);
                    } else if (touchXY.px >= 16 && touchXY.px < 32) { // place structure
                        if (getGameType() == SINGLEPLAYER) { // immediately place, be nice
                            if (playScreenTouchModifier == PSTM_PLACE_STRUCTURE) {
                                if (addStructure(FRIENDLY, getViewCurrentX() + touchPlaceStructure.px/16, getViewCurrentY() + touchPlaceStructure.py/16, getStructuresQueue(FRIENDLY)[queueNrPlaceStructure].itemInfo, 0) != -1) {
                                    removeItemStructuresQueueNr(FRIENDLY, queueNrPlaceStructure, 0, 0);
                                    playScreenTouchModifier = PSTM_NONE;
                                } else
                                    playSoundeffect(SE_CANNOT);
                            } else {
                                environment.layout[TILE_FROM_XY(unit[queueNrPlaceStructure].x, unit[queueNrPlaceStructure].y)].contains_unit = -1;
                                if ((i = addStructure(FRIENDLY, unit[queueNrPlaceStructure].x, unit[queueNrPlaceStructure].y, unitInfo[unit[queueNrPlaceStructure].info].contains_structure, 1)) != -1) {
                                    if (structure[i].armour != INFINITE_ARMOUR) {
                                        structure[i].armour = (structure[i].armour * 3) / 4;
                                        structure[i].selected = 1;
                                    }
                                    unit[queueNrPlaceStructure].armour = 0;
                                    unit[queueNrPlaceStructure].selected = 0;
                                    playScreenTouchModifier = PSTM_NONE;
                                } else {
                                    environment.layout[TILE_FROM_XY(unit[queueNrPlaceStructure].x, unit[queueNrPlaceStructure].y)].contains_unit = queueNrPlaceStructure;
                                    playSoundeffect(SE_CANNOT);
                                }
                            }
                        }/* else { // queue it for placing
                            requestAddStructure(FRIENDLY, getViewCurrentX() + touchPlaceStructure.px/16, getViewCurrentY() + touchPlaceStructure.py/16, getStructuresQueue(FRIENDLY)[queueNrPlaceStructure].itemInfo, 0);
                            freezeItemStructuresQueueNr(FRIENDLY, queueNrPlaceStructure);
                            playScreenTouchModifier = PSTM_NONE;
                        }*/
                    }
                } else if (playScreenTouchModifier == PSTM_SELL_STRUCTURE) {
                    if (touchXY.px >= SCREEN_WIDTH - 32 && touchXY.px < SCREEN_WIDTH - 16) { // cancel selling structure
                        playScreenTouchModifier = PSTM_NONE;
                        playSoundeffect(SE_BUTTONCLICK);
                    } else if (touchXY.px >= 16 && touchXY.px < 32) { // sell structure
                        if (getGameType() == SINGLEPLAYER) { // immediately sell, be nice
                            if (structure[sellNrStructure].armour > 0) {
                                changeCredits(FRIENDLY, ((structure[sellNrStructure].armour * structureInfo[structure[sellNrStructure].info].build_cost) / structureInfo[structure[sellNrStructure].info].max_armour) / 2);
                                structure[sellNrStructure].armour = 0;
                            }
                            playScreenTouchModifier = PSTM_NONE;
    /*TODO: MULTIPLAYER CODE!!! */
    //                    } else { // queue it for selling
    //                        requestAddStructure(FRIENDLY, getViewCurrentX() + touchPlaceStructure.px/16, getViewCurrentY() + touchPlaceStructure.py/16, getStructuresQueue(FRIENDLY)[queueNrPlaceStructure].itemInfo, 0);
    //                        freezeItemStructuresQueueNr(FRIENDLY, queueNrPlaceStructure);
    //                        playScreenTouchModifier = PSTM_NONE;
                        }
                    }
                } else {
                    for (i=0; i<MAX_STRUCTURES_ON_MAP; i++) {
                        if (structure[i].selected && structure[i].enabled && structure[i].side == FRIENDLY) {
                            if (touchXY.px >= 16 && touchXY.px < 32) { // toggle repairing
                                if (!structureInfo[structure[i].info].barrier) {
                                    if (getGameType() == SINGLEPLAYER) {
                                        if (structure[i].logic & 1) // repairing
                                            structure[i].logic--;
                                        else
                                            structure[i].logic++;
                                    }
                                    playSoundeffect(SE_BUTTONCLICK);
                                }
                            } else if (touchXY.px >= 48 && touchXY.px < 64) { // set as primary
                                //if (structureInfo[structure[i].info].release_tile >= 0) {
                                if (structure[i].rally_tile >= 0) {
                                    structure[i].primary = 1;
                                    for (j=0; j<MAX_STRUCTURES_ON_MAP; j++) {
                                        if (j!=i && structure[j].primary && structure[j].info == structure[i].info && structure[j].side == structure[i].side && structure[j].enabled) {
                                            structure[j].primary = 0;
                                            break;
                                        }
                                    }
                                    playSoundeffect(SE_BUTTONCLICK);
                                }
                            } else if (touchXY.px >= 80 && touchXY.px < 96) { // rally point
                                if (structure[i].rally_tile >= 0) {
                                    if (playScreenTouchModifier == PSTM_RALLYPOINT_STRUCTURE)
                                        playScreenTouchModifier = PSTM_NONE;
                                    else
                                        playScreenTouchModifier = PSTM_RALLYPOINT_STRUCTURE;
                                    playSoundeffect(SE_BUTTONCLICK);
                                }
                            } else if (touchXY.px >= (SCREEN_WIDTH - 48) - 16 && touchXY.px < (SCREEN_WIDTH - 48)) { // sell
                                if (!structureInfo[structure[i].info].barrier) {
                                    playScreenTouchModifier = PSTM_SELL_STRUCTURE;
                                    sellNrStructure = i;
                                    playSoundeffect(SE_CANNOT);
                                }
                            } else if (touchXY.px >= (SCREEN_WIDTH - ACTION_BAR_SIZE * 32) && touchXY.px < (SCREEN_WIDTH - ACTION_BAR_SIZE * 16)) { // cancel selection
                                structure[i].selected = 0;
                                playSoundeffect(SE_BUTTONCLICK);
                            }
                            break;
                        }
                    }
                    if (i >= MAX_STRUCTURES_ON_MAP) { // no structure was selected, maybe one ore more units are?
                        for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                            if (unit[i].selected && unit[i].enabled && unit[i].side == FRIENDLY) {
                                for (j=1; j<=3; j++) {
                                    if (touchXY.px >= -16 + 16 * 2 * j && touchXY.px < 16 * 2 * j)
                                        break;
                                }
                                if (j > 3) {
                                    for (j=1; j<=3; j++) {
                                        if (touchXY.px >= SCREEN_WIDTH - 16 * 2 * j && touchXY.px < (SCREEN_WIDTH + 16) - 16 * 2 * j) {
                                            j = 8-j;
                                            break;
                                        }
                                    }
                                }
                                
                                forcedAttackAvailable = 0; // in this case, it equals healAvailable because it activates PSTM_ATTACK (== PSTM_HEAL)
                                deployAvailable = -1;
                                for (k=i; k<MAX_UNITS_ON_MAP; k++) {
                                    if (unit[k].selected) {
                                        if (unitInfo[unit[k].info].shoot_range) {
                                            forcedAttackAvailable = 1;
                                            deployAvailable = -1;
                                            break;
                                        }
                                        if (unitInfo[unit[k].info].contains_structure >= 0 &&
                                            (unit[k].move == UM_MOVE_HOLD || (unit[k].move == UM_NONE && unit[k].logic == UL_GUARD)) &&
                                            unit[k].x >= getViewCurrentX() && unit[k].x < getViewCurrentX() + HORIZONTAL_WIDTH &&
                                            unit[k].y >= getViewCurrentY() && unit[k].y < getViewCurrentY() + HORIZONTAL_HEIGHT)
                                            deployAvailable = k;
                                        if (k > i)
                                            deployAvailable = -1;
                                    }
                                }
                                
                                if (j == 1 && forcedAttackAvailable) { // toggle force attack
                                    if (playScreenTouchModifier == PSTM_ATTACK)
                                        playScreenTouchModifier = PSTM_NONE;
                                    else
                                        playScreenTouchModifier = PSTM_ATTACK;
                                    playSoundeffect(SE_BUTTONCLICK);
                                } else if (j == 1 && deployAvailable != -1) { // deploy structure. unit transform request
                                    playScreenTouchModifier = PSTM_DEPLOY_STRUCTURE;
                                    queueNrPlaceStructure = deployAvailable;
                                    touchPlaceStructure.px = (unit[queueNrPlaceStructure].x - getViewCurrentX()) * 16;
                                    touchPlaceStructure.py = (unit[queueNrPlaceStructure].y - getViewCurrentY()) * 16;
                                    playSoundeffect(SE_BUTTONCLICK);
                                } else if (j == 2) { // toggle force move
                                    if (playScreenTouchModifier == PSTM_MOVE)
                                        playScreenTouchModifier = PSTM_NONE;
                                    else
                                        playScreenTouchModifier = PSTM_MOVE;
                                    playSoundeffect(SE_BUTTONCLICK);
                                } else if (j == 3) { // make unit selection guard
                                    if (getGameType() == SINGLEPLAYER) {
                                        for (j=i; j<MAX_UNITS_ON_MAP; j++) {
                                            if (unit[j].selected && unit[j].enabled && unit[j].side == FRIENDLY)
                                                unit[j].logic = UL_GUARD;
                                        }
                                    }
                                    playSoundeffect(SE_BUTTONCLICK);
                                } else if (j == 5) { // make unit selection retreat
                                    if (getGameType() == SINGLEPLAYER) {
                                        for (j=i; j<MAX_UNITS_ON_MAP; j++) {
                                            if (unit[j].selected && unit[j].enabled && unit[j].side == FRIENDLY)
                                                unit[j].logic = UL_RETREAT;
                                        }
                                    }
                                    playSoundeffect(SE_BUTTONCLICK);
                                } else if (j == 6) { // focus
                                    for (j=i+1; j<MAX_UNITS_ON_MAP && (!unit[j].selected || !unit[j].enabled); j++);
                                    if (j == MAX_UNITS_ON_MAP) {
                                        if (getFocusOnUnitNr() == i)
                                            setFocusOnUnitNr(-1);
                                        else
                                            setFocusOnUnitNr(i);
                                    }
                                } else if (j == 7) { // cancel selection
                                    for (j=i; j<MAX_UNITS_ON_MAP; j++)
                                        unit[j].selected = 0;
                                    setFocusOnUnitNr(-1);
                                    playSoundeffect(SE_BUTTONCLICK);
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
    } else if (playScreenTouchModifier == PSTM_PLACE_STRUCTURE) {
        /* placing structure input on actual playfield */
        if ((getKeysOnUp() & KEY_TOUCH) && getKeysHeldTimeMax(KEY_TOUCH) < (FPS/4)) { // could be a tap to place structure here
            if (getGameType() == SINGLEPLAYER) { // immediately place, be nice
                if (addStructure(FRIENDLY, getViewCurrentX() + touchPlaceStructure.px/16, getViewCurrentY() + touchPlaceStructure.py/16, getStructuresQueue(FRIENDLY)[queueNrPlaceStructure].itemInfo, 0) != -1) {
                    removeItemStructuresQueueNr(FRIENDLY, queueNrPlaceStructure, 0, 0);
                    playScreenTouchModifier = PSTM_NONE;
                } else
                    playSoundeffect(SE_CANNOT);
            }/* else { // queue it for placing
                requestAddStructure(FRIENDLY, getViewCurrentX() + touchPlaceStructure.px/16, getViewCurrentY() + touchPlaceStructure.py/16, getStructuresQueue(FRIENDLY)[queueNrPlaceStructure].itemInfo, 0);
                freezeItemStructuresQueueNr(FRIENDLY, queueNrPlaceStructure);
                playScreenTouchModifier = PSTM_NONE;
            }*/
        } else {
            touchPlaceStructure = touchXY;
            if (touchPlaceStructure.px / 16 > (HORIZONTAL_WIDTH - 1) - structureInfo[getStructuresQueue(FRIENDLY)[queueNrPlaceStructure].itemInfo].width)
                touchPlaceStructure.px = 16 * (HORIZONTAL_WIDTH - structureInfo[getStructuresQueue(FRIENDLY)[queueNrPlaceStructure].itemInfo].width);
            if (touchPlaceStructure.py < ACTION_BAR_SIZE * 16)
                touchPlaceStructure.py = 0;
            else {
                touchPlaceStructure.py -= ACTION_BAR_SIZE * 16;
                if (touchPlaceStructure.py / 16 > (HORIZONTAL_HEIGHT - 1) - structureInfo[getStructuresQueue(FRIENDLY)[queueNrPlaceStructure].itemInfo].height)
                    touchPlaceStructure.py = 16 * (HORIZONTAL_HEIGHT - structureInfo[getStructuresQueue(FRIENDLY)[queueNrPlaceStructure].itemInfo].height);
            }
        }
    } else if (playScreenTouchModifier == PSTM_DEPLOY_STRUCTURE) {
        /* placing structure input on actual playfield */
        if ((getKeysOnUp() & KEY_TOUCH) && getKeysHeldTimeMax(KEY_TOUCH) < (FPS/4)) {
            if (touchXY.px/16 == touchPlaceStructure.px/16 && touchXY.py/16 == (touchPlaceStructure.py/16 + ACTION_BAR_SIZE)) { // could be a tap to place structure here
                if (getGameType() == SINGLEPLAYER) { // immediately place, be nice
                    environment.layout[TILE_FROM_XY(unit[queueNrPlaceStructure].x, unit[queueNrPlaceStructure].y)].contains_unit = -1;
                    if ((i = addStructure(FRIENDLY, unit[queueNrPlaceStructure].x, unit[queueNrPlaceStructure].y, unitInfo[unit[queueNrPlaceStructure].info].contains_structure, 1)) != -1) {
                        structure[i].armour = (structure[i].armour * 3) / 4;
                        if (structure[i].armour != INFINITE_ARMOUR)
                            structure[i].selected = 1;
                        unit[queueNrPlaceStructure].armour = 0;
                        unit[queueNrPlaceStructure].selected = 0;
                        playScreenTouchModifier = PSTM_NONE;
                    } else {
                        environment.layout[TILE_FROM_XY(unit[queueNrPlaceStructure].x, unit[queueNrPlaceStructure].y)].contains_unit = queueNrPlaceStructure;
                        playSoundeffect(SE_CANNOT);
                    }
                }
            } else
                playScreenTouchModifier = PSTM_NONE;
        }
    } else if (getKeysOnUp() & KEY_TOUCH) {
        /* cancel selling a structure */
        if (playScreenTouchModifier == PSTM_SELL_STRUCTURE)
            playScreenTouchModifier = PSTM_NONE;
        
        /* taps and drags on actual playfield */
        if (touchXY.py < ACTION_BAR_SIZE * 16)
            touchXY.py = ACTION_BAR_SIZE * 16;
        tile = TILE_FROM_XY(getViewCurrentX() + min(touchXY.px, touchReadOrigin().px) / 16, (getViewCurrentY() + min(touchXY.py, touchReadOrigin().py) / 16) - ACTION_BAR_SIZE);
        if (touchXY.px >= touchReadOrigin().px + DRAG_SELECTION_THRESHHOLD || touchXY.px <= touchReadOrigin().px - DRAG_SELECTION_THRESHHOLD ||
            touchXY.py >= touchReadOrigin().py + DRAG_SELECTION_THRESHHOLD || touchXY.py <= touchReadOrigin().py - DRAG_SELECTION_THRESHHOLD) {
            /* a drag-selection has been made by the user */
            box_width = abs(touchXY.px / 16 - touchReadOrigin().px / 16) + 1;
            box_height = abs(touchXY.py / 16 - touchReadOrigin().py / 16) + 1;
            if (box_width <= 1) box_width++;
            
            deselectAll();
            playScreenTouchModifier = PSTM_NONE;
            for (i=0; i<box_height; i++) {
                for (j=0; j<box_width; j++) {
                    unitnr = environment.layout[tile + j].contains_unit;
                    if (unitnr >= 0 && unit[unitnr].armour != INFINITE_ARMOUR && unit[unitnr].side == FRIENDLY &&
                        (environment.layout[tile + j].contains_structure == -1 || (environment.layout[tile + j].contains_structure >= 0 && structureInfo[structure[environment.layout[tile + j].contains_structure].info].foundation))) {
                        unit[unitnr].selected = 1;
                        
                        if (unitInfo[unit[unitnr].info].can_collect_ore)
                            containsOreCollector = unitnr;
                        else {
                            containsOtherUnits = unitnr;
                            
                            if (unitType == 255 || unitInfo[unit[unitnr].info].type > unitType)
                                unitType = unitInfo[unit[unitnr].info].type;
                        }
                    }
                }
                tile += environment.width;
            }
            
            if (containsOreCollector >= 0) {
                if (containsOtherUnits >= 0) { // deselect all Ore Collectors in selection
                    for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                      if (unit[i].selected && unitInfo[unit[i].info].can_collect_ore)
                        unit[i].selected = 0;
                    }
                } else { // also make unitType sound adjust according to Ore Collectors in selection
                    if (unitType == 255 || unitInfo[unit[containsOreCollector].info].type > unitType)
                        unitType = unitInfo[unit[containsOreCollector].info].type;
                }
            }
            
            if (unitType == UT_FOOT)
                playSoundeffect(SE_FOOT_SELECT);
            else if (unitType == UT_WHEELED)
                playSoundeffect(SE_WHEELED_SELECT);
            else if (unitType == UT_TRACKED)
                playSoundeffect(SE_TRACKED_SELECT);
            else if (unitType == UT_AERIAL)
                playSoundeffect(SE_TRACKED_SELECT);
            else { /* maybe select a structure or one enemy unit */
                tile = TILE_FROM_XY(getViewCurrentX() + min(touchXY.px, touchReadOrigin().px) / 16, (getViewCurrentY() + min(touchXY.py, touchReadOrigin().py) / 16) - ACTION_BAR_SIZE);
                k = -1;
                j = 0;
                for (i=0; i<box_height; i++) {
                    for (j=0; j<box_width; j++) {
                        if (environment.layout[tile + j].contains_structure >= MAX_DIFFERENT_FACTIONS) { // not considering foundation
                            if (structure[environment.layout[tile + j].contains_structure].armour != INFINITE_ARMOUR) {
                                if (k == -1)
                                    k = environment.layout[tile + j].contains_structure;
                                else if (k != environment.layout[tile + j].contains_structure)
                                    break;
                            }
                        } else if (environment.layout[tile + j].contains_structure < -1) {
                            if (structure[environment.layout[tile + j + (environment.layout[tile + j].contains_structure + 1)].contains_structure].armour != INFINITE_ARMOUR) {
                                if (k == -1)
                                    k = environment.layout[tile + j + (environment.layout[tile + j].contains_structure + 1)].contains_structure;
                                else if (k != environment.layout[tile + j + (environment.layout[tile + j].contains_structure + 1)].contains_structure)
                                    break;
                            }
                        }
                    }
                    if (j < box_width) {
                        k = -1;
                        break;
                    }
                    tile += environment.width;
                }
                if (k >= 0) { /* one single structure found in the selection */
                    tile = TILE_FROM_XY(getViewCurrentX() + min(touchXY.px, touchReadOrigin().px) / 16, (getViewCurrentY() + min(touchXY.py, touchReadOrigin().py) / 16) - ACTION_BAR_SIZE);
                    for (i=0; i<box_height; i++) {
                        for (j=0; j<box_width; j++) {
                            if (environment.layout[tile + j].contains_unit >= 0 && unit[environment.layout[tile + j].contains_unit].armour != INFINITE_ARMOUR)
                                break;
                        }
                        if (j < box_width) {
                            k = -1;
                            break;
                        }
                        tile += environment.width;
                    }
                    if (k >= 0) { /* no (enemy) units found in the selection either, so select the structure */
                        structure[k].selected = 1;
                        if (structure[k].side == FRIENDLY)
                            playSoundeffect(SE_STRUCTURE_SELECT);
                    }
                } else if (i == box_height && j == box_width) { /* no structures found in the selection */
                    tile = TILE_FROM_XY(getViewCurrentX() + min(touchXY.px, touchReadOrigin().px) / 16, (getViewCurrentY() + min(touchXY.py, touchReadOrigin().py) / 16) - ACTION_BAR_SIZE);
                    for (i=0; i<box_height; i++) {
                        for (j=0; j<box_width; j++) {
                            if (environment.layout[tile + j].contains_unit >= 0 && unit[environment.layout[tile + j].contains_unit].armour != INFINITE_ARMOUR) {
                                if (k == -1)
                                    k = environment.layout[tile + j].contains_unit;
                                else if (k != environment.layout[tile + j].contains_unit)
                                    break;
                            }
                        }
                        if (j < box_width) {
                            k = -1;
                            break;
                        }
                        tile += environment.width;
                    }
                    if (k >= 0) /* a single enemy unit has been found in the selection */
                        unit[k].selected = 1;
                }
            }
        } else {
            /* a single tile has been tapped by the user */
            if (playScreenTouchModifier == PSTM_RALLYPOINT_STRUCTURE) {
                for (i=0; i<MAX_STRUCTURES_ON_MAP; i++) {
                    if (structure[i].selected && structure[i].side == FRIENDLY && structure[i].enabled) {
                        structure[i].rally_tile = tile;
                        playSoundeffect(SE_BUTTONCLICK);
                        break;
                    }
                }
                playScreenTouchModifier = PSTM_NONE;
            } else {
                /* this single tile tap might need to be improved upon, location wise.
                   although units in the engine are said to be located on a tile, they might be moving from it
                   and thus more precision is needed to determine what the user meant (and adjust the
                   tile tapped accordingly) before further processing
                   2 special situations can occur: 1) the actual intention was to tap a unit that was drawn over this position
                                                   2) the actual intention was to tap on the tile the unit seemed not to belong to (anymore)
                   we will only take care of the first, as this is the most pressing and will require no heavy alterations in code
                   important to realise with this is that units are drawn on top of another unit if:
                      - it has a larger Y-pos
                      - it has the same Y-pos and a smaller X-pos 
                 */
                intendedTouchTile = tile;
                xWithinTile = min(touchXY.px, touchReadOrigin().px) % 16;
                yWithinTile = min(touchXY.py, touchReadOrigin().py) % 16;
                for (i=-1; i<=1; i++) { // from smaller Y to larger Y
                    for (j=1; j>=-1; j--) {  // from larger X to smaller X
                        if ((Y_FROM_TILE(tile) + i < 0) || (Y_FROM_TILE(tile) + i >= environment.height)) continue;
                        if ((X_FROM_TILE(tile) + j < 0) || (X_FROM_TILE(tile) + j >= environment.width)) continue;
                        
                        // if a unit on this tile was displayed over the touched coordinates, this tile is deemed more appropriate as touched tile
                        unit_t = environment.layout[tile + TILE_FROM_XY(j,i)].contains_unit;
                        if (unit_t <= -1) continue;
                        
                        if (unitTouchableOnTileCoordinates(unit_t, xWithinTile - j*16, yWithinTile - i*16))
                            intendedTouchTile = tile + TILE_FROM_XY(j,i);
                    }
                }
                tile = intendedTouchTile;
                
                /* further processing of the (now) proper tile tap */
                for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                    if (unit[i].selected && unit[i].side == FRIENDLY && unit[i].enabled)
                        break;
                }
                if (i == MAX_UNITS_ON_MAP) {
                    playScreenTouchModifier = PSTM_NONE;
                    if (environment.layout[tile].contains_structure < -1 && structure[environment.layout[tile + environment.layout[tile].contains_structure + 1].contains_structure].armour != INFINITE_ARMOUR) {
                        if (structure[environment.layout[tile + environment.layout[tile].contains_structure + 1].contains_structure].selected) {
                            if (structure[environment.layout[tile + environment.layout[tile].contains_structure + 1].contains_structure].side == FRIENDLY)
                                setInfoScreenBuildInfo(1, structure[environment.layout[tile + environment.layout[tile].contains_structure + 1].contains_structure].info);
                        } else {
                            deselectAll();
                            structure[environment.layout[tile + environment.layout[tile].contains_structure + 1].contains_structure].selected = 1;
                        }
                        if (structure[environment.layout[tile + environment.layout[tile].contains_structure + 1].contains_structure].side == FRIENDLY)
                            playSoundeffect(SE_STRUCTURE_SELECT);
                    } else if (environment.layout[tile].contains_structure >= 0 && !structureInfo[structure[environment.layout[tile].contains_structure].info].foundation && structure[environment.layout[tile].contains_structure].armour != INFINITE_ARMOUR) {
                        if (structure[environment.layout[tile].contains_structure].selected) {
                            if (structure[environment.layout[tile].contains_structure].side == FRIENDLY)
                                setInfoScreenBuildInfo(1, structure[environment.layout[tile].contains_structure].info);
                        } else {
                            deselectAll();
                            structure[environment.layout[tile].contains_structure].selected = 1;
                        }
                        if (structure[environment.layout[tile].contains_structure].side == FRIENDLY)
                            playSoundeffect(SE_STRUCTURE_SELECT);
                    } else if (environment.layout[tile].contains_unit >= 0 && unit[environment.layout[tile].contains_unit].armour != INFINITE_ARMOUR) {
                        deselectAll();
                        unit[environment.layout[tile].contains_unit].selected = 1;
                        if (unit[environment.layout[tile].contains_unit].side == FRIENDLY) {
                            unitType = unitInfo[unit[environment.layout[tile].contains_unit].info].type;
                            if (unitType == UT_FOOT)
                                playSoundeffect(SE_FOOT_SELECT);
                            else if (unitType == UT_WHEELED)
                                playSoundeffect(SE_WHEELED_SELECT);
                            else if (unitType == UT_TRACKED)
                                playSoundeffect(SE_TRACKED_SELECT);
                            else if (unitType == UT_AERIAL)
                                playSoundeffect(SE_TRACKED_SELECT);
                        }
                    } else {
                        if (graphicalActionIssued == GAI_NONE && graphicalActionIssuedTimer > 0 && graphicalActionIssuedLocation == tile) {
                            // a double tap is registered: show the radar on infoscreen
                            setInfoScreenRadar();
                            playSoundeffect(SE_BUTTONCLICK);
                        } else {
                            // a single tap is registered; start the timer
                            graphicalActionIssued         = GAI_NONE;
                            graphicalActionIssuedLocation = tile;
                            graphicalActionIssuedTimer    = 1;
                        }
                    }
                } else {
                    /* friendly unit(s) were selected when the tap took place */
                    if (playScreenTouchModifier == PSTM_ATTACK) {
                        
                        forcedAttackAvailable = 0;
                        healAvailable = 0;
                        for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                            if (unit[i].selected && unitInfo[unit[i].info].shoot_range) { // forced attack does not exist when no units are selected that can shoot
                                if (!unitInfo[unit[i].info].can_heal_foot) {
                                    forcedAttackAvailable = 1;
                                    inputPlayScreenFriendlyForcedAttack(tile, 0);
                                    break;
                                } else
                                    healAvailable = 1;
                            }
                        }
                        if (!forcedAttackAvailable && healAvailable)
                            inputPlayScreenFriendlyForcedAttack(tile, 1);
                        
                    } else if (playScreenTouchModifier == PSTM_MOVE)
                        
                        inputPlayScreenFriendlyForcedMove(tile);
                        
                    else {
                        structure_t = tile;
                        if (environment.layout[tile].contains_structure < -1)
                            structure_t += environment.layout[tile].contains_structure + 1;
                        structure_t = environment.layout[structure_t].contains_structure;
                        unit_t = environment.layout[tile].contains_unit;
                        if (environment.layout[tile].status != UNDISCOVERED && structure_t != -1 && structure[structure_t].side != FRIENDLY && !structureInfo[structure[structure_t].info].foundation) {
                            /* attack location: structure */
                            graphicalActionIssued = GAI_MOVE_LOCATION;
                            if (getGameType() == SINGLEPLAYER) {
                                for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                                    if (unit[i].selected) {
                                        if (!unitInfo[unit[i].info].shoot_range)
                                            unit[i].logic = UL_MOVE_LOCATION;
                                        else {
                                            graphicalActionIssued = GAI_ATTACK_LOCATION;
                                            unit[i].logic = UL_ATTACK_LOCATION;
                                            unit[i].group = (unit[i].group & (~UGAI_FRIENDLY_MASK)) | UGAI_ATTACK_FORCED;
                                        }
                                        unit[i].logic_aid = tile;
                                        #ifdef REMOVE_ASTAR_PATHFINDING
                                        unit[i].hugging_obstacle = 0;
                                        #endif
                                        if (unitType == 255 || unitInfo[unit[i].info].type > unitType)
                                            unitType = unitInfo[unit[i].info].type;
                                    }
                                }
                            }
                            if (graphicalActionIssued == GAI_MOVE_LOCATION) {
                                if (unitType == UT_FOOT)
                                    playSoundeffect(SE_FOOT_MOVE);
                                else if (unitType == UT_WHEELED)
                                    playSoundeffect(SE_WHEELED_MOVE);
                                else if (unitType == UT_TRACKED)
                                    playSoundeffect(SE_TRACKED_MOVE);
                                else if (unitType == UT_AERIAL)
                                    playSoundeffect(SE_TRACKED_MOVE);
                            } else {
                                graphicalActionIssuedLocation = unit_t;
                                if (unitType == UT_FOOT)
                                    playSoundeffect(SE_FOOT_ATTACK);
                                else if (unitType == UT_WHEELED)
                                    playSoundeffect(SE_WHEELED_ATTACK);
                                else if (unitType == UT_TRACKED)
                                    playSoundeffect(SE_TRACKED_ATTACK);
                                else if (unitType == UT_AERIAL)
                                    playSoundeffect(SE_TRACKED_ATTACK);
                            }
                            graphicalActionIssuedLocation = tile;
                            graphicalActionIssuedTimer = 0;
                            if (unitType == UT_FOOT)
                                playSoundeffect(SE_FOOT_ATTACK);
                            else if (unitType == UT_WHEELED)
                                playSoundeffect(SE_WHEELED_ATTACK);
                            else if (unitType == UT_TRACKED)
                                playSoundeffect(SE_TRACKED_ATTACK);
                            else if (unitType == UT_AERIAL)
                                playSoundeffect(SE_TRACKED_ATTACK);
                        } else if (environment.layout[tile].status != UNDISCOVERED && unit_t >= 0 && unit[unit_t].armour != INFINITE_ARMOUR) {
                            if (unit[unit_t].side != FRIENDLY) {
                                /* attack unit */
                                graphicalActionIssued = GAI_MOVE_LOCATION;
                                if (getGameType() == SINGLEPLAYER) {
                                    for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                                        if (unit[i].selected) {
                                            if (!unitInfo[unit[i].info].shoot_range) {
                                                unit[i].logic = UL_MOVE_LOCATION;
                                                unit[i].logic_aid = tile;
                                            } else {
                                                graphicalActionIssued = GAI_ATTACK_UNIT;
                                                unit[i].logic = UL_ATTACK_UNIT;
                                                unit[i].logic_aid = unit_t;
                                                unit[i].group = (unit[i].group & (~UGAI_FRIENDLY_MASK)) | UGAI_ATTACK_FORCED;
                                            }
                                            #ifdef REMOVE_ASTAR_PATHFINDING
                                            unit[i].hugging_obstacle = 0;
                                            #endif
                                            if (unitType == 255 || unitInfo[unit[i].info].type > unitType)
                                                unitType = unitInfo[unit[i].info].type;
                                        }
                                    }
                                }
                                if (graphicalActionIssued == GAI_MOVE_LOCATION) {
                                    graphicalActionIssuedLocation = tile;
                                    if (unitType == UT_FOOT)
                                        playSoundeffect(SE_FOOT_MOVE);
                                    else if (unitType == UT_WHEELED)
                                        playSoundeffect(SE_WHEELED_MOVE);
                                    else if (unitType == UT_TRACKED)
                                        playSoundeffect(SE_TRACKED_MOVE);
                                    else if (unitType == UT_AERIAL)
                                        playSoundeffect(SE_TRACKED_MOVE);
                                } else {
                                    graphicalActionIssuedLocation = unit_t;
                                    if (unitType == UT_FOOT)
                                        playSoundeffect(SE_FOOT_ATTACK);
                                    else if (unitType == UT_WHEELED)
                                        playSoundeffect(SE_WHEELED_ATTACK);
                                    else if (unitType == UT_TRACKED)
                                        playSoundeffect(SE_TRACKED_ATTACK);
                                    else if (unitType == UT_AERIAL)
                                        playSoundeffect(SE_TRACKED_ATTACK);
                                }
                                graphicalActionIssuedTimer = 0;
                            } else if (structure_t == -1 || structureInfo[structure[structure_t].info].foundation) {
                                /* if this unit was selected, and no others were selected, select the same type of units */
                                i = 0;
                                if (unit[unit_t].selected && unit[unit_t].side == FRIENDLY) {
                                    for (i=0; i<MAX_UNITS_ON_MAP && (i == unit_t || !unit[i].selected); i++);
                                    if (i == MAX_UNITS_ON_MAP) {
                                        /* select all units of the same type on the current view */
                                        for (i=0; i<HORIZONTAL_HEIGHT; i++) {
                                            tile = TILE_FROM_XY(getViewCurrentX(), getViewCurrentY() + i);
                                            for (j=0; j<HORIZONTAL_WIDTH; j++, tile++) {
                                                k = environment.layout[tile].contains_unit;
                                                if (k >= 0 && unit[k].side == FRIENDLY && unit[k].info == unit[unit_t].info)
                                                    unit[k].selected = 1;
                                            }
                                        }
                                        i = MAX_UNITS_ON_MAP;
                                        setInfoScreenBuildInfo(0, unit[unit_t].info);
                                    }
                                }
                                /* if the above didn't happen, just select this unit */
                                if (i < MAX_UNITS_ON_MAP) {
                                    deselectAll();
                                    unit[unit_t].selected = 1;
                                }
                                /* play unit select sound */
                                unitType = unitInfo[unit[unit_t].info].type;
                                if (unitType == UT_FOOT)
                                    playSoundeffect(SE_FOOT_SELECT);
                                else if (unitType == UT_WHEELED)
                                    playSoundeffect(SE_WHEELED_SELECT);
                                else if (unitType == UT_TRACKED)
                                    playSoundeffect(SE_TRACKED_SELECT);
                                else if (unitType == UT_AERIAL)
                                    playSoundeffect(SE_TRACKED_SELECT);
                            }
                        } else {
                            /* possibly a friendly building or just a location */
                            j = 0; // used as temp variable; 0 means perform move
                            if (structure_t >= MAX_DIFFERENT_FACTIONS && structure[structure_t].side == FRIENDLY && structure[structure_t].armour != INFINITE_ARMOUR) {
                                j = 1;
                                if (structureInfo[structure[structure_t].info].can_extract_ore) {
                                    // if all units selected can gather ore, move them to the tile.
                                    j = 0;
                                    for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                                        if (unit[i].selected && unit[i].side == FRIENDLY && unit[i].enabled && !unitInfo[unit[i].info].can_collect_ore) {
                                            j = 1; // 1 means select structure
                                            break;
                                        }
                                    }
                                } else if (structureInfo[structure[structure_t].info].can_repair_unit) {
                                    // if a unit is selected which can move into this structure, move them to the tile.
                                    j = 1;
                                    for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                                        if (unit[i].selected && unit[i].side == FRIENDLY && unit[i].enabled && unitInfo[unit[i].info].type != UT_FOOT) {
                                            j = 0;
                                            break;
                                        }
                                    }
                                }
                            }
                            
                            if (j == 0) {
                                /* move to location */
                                logic = UL_MOVE_LOCATION;
                                if (getGameType() == SINGLEPLAYER) {
                                    for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                                        if (unit[i].selected && !unitInfo[unit[i].info].can_collect_ore)
                                            break;
                                    }
                                    if (i >= MAX_UNITS_ON_MAP) {
                                        if (isEnvironmentTileBetween(tile, ORE, OREHILL16))
                                            logic = UL_MINE_LOCATION;
                                        else if (structure_t >= MAX_DIFFERENT_FACTIONS && structure[structure_t].side == FRIENDLY && structure[structure_t].armour != INFINITE_ARMOUR) {
                                            logic = UL_RETREAT;
                                            
                                        }
                                    }
                                    
                                    for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                                        if (unit[i].selected) {
                                            unit[i].logic = logic;
                                            if (logic == UL_RETREAT)
                                                unit[i].retreat_tile = tile;
                                            else
                                                unit[i].logic_aid = tile;
                                            #ifdef REMOVE_ASTAR_PATHFINDING
                                            unit[i].hugging_obstacle = 0;
                                            #endif
                                            if (unitType == 255 || unitInfo[unit[i].info].type > unitType)
                                                unitType = unitInfo[unit[i].info].type;
                                        }
                                    }
                                }
                                if (logic == UL_MINE_LOCATION && environment.layout[tile].status != UNDISCOVERED) {
                                    graphicalActionIssued = GAI_ATTACK_LOCATION;
                                    if (unitType == UT_FOOT)
                                        playSoundeffect(SE_FOOT_ATTACK);
                                    else if (unitType == UT_WHEELED)
                                        playSoundeffect(SE_WHEELED_ATTACK);
                                    else if (unitType == UT_TRACKED)
                                        playSoundeffect(SE_TRACKED_ATTACK);
                                    else if (unitType == UT_AERIAL)
                                        playSoundeffect(SE_TRACKED_ATTACK);
                                } else {
                                    graphicalActionIssued = GAI_MOVE_LOCATION;
                                    if (unitType == UT_FOOT)
                                        playSoundeffect(SE_FOOT_MOVE);
                                    else if (unitType == UT_WHEELED)
                                        playSoundeffect(SE_WHEELED_MOVE);
                                    else if (unitType == UT_TRACKED)
                                        playSoundeffect(SE_TRACKED_MOVE);
                                    else if (unitType == UT_AERIAL)
                                        playSoundeffect(SE_TRACKED_MOVE);
                                }
                                graphicalActionIssuedLocation = tile;
                                graphicalActionIssuedTimer = 0;
                            } else {
                                /* select structure */
                                deselectAll();
                                structure[structure_t].selected = 1;
                                if (structure[structure_t].side == FRIENDLY)
                                    playSoundeffect(SE_STRUCTURE_SELECT);
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (graphicalActionIssued == GAI_NONE && graphicalActionIssuedTimer > 0)
        graphicalActionIssuedTimer = (graphicalActionIssuedTimer+1) % DOUBLE_TAP_THRESHHOLD_RADAR;
    
    if (playScreenTouchModifier != PSTM_DEPLOY_STRUCTURE)
        inputButtonScrolling();
    if (playScreenTouchModifier <= PSTM_MOVE || playScreenTouchModifier == PSTM_PLACE_STRUCTURE)
        inputHotKeys();
    
    /* code for swapping screens */
    if (swapScreens || 
/*
        (getPrimaryHand() == RIGHT_HANDED && (keysDown() & KEY_L)) ||
        (getPrimaryHand() == LEFT_HANDED && (keysDown() & KEY_R))
*/
        (keysDown() & (KEY_L | KEY_R))
    ) {
//      lcdSwap();
        setScreenSetUp(MAIN_TO_SUB_TOUCH);
        initKeysHeldTime(~0);
        if (playScreenTouchModifier == PSTM_PLACE_STRUCTURE || playScreenTouchModifier == PSTM_DEPLOY_STRUCTURE)
            playScreenTouchModifier = PSTM_NONE;
        playSoundeffect(SE_BUTTONCLICK);
    }
    
    stopProfilingFunction();
}

void doPlayScreenLogic() {
    startProfilingFunction("doPlayScreenLogic");
    
    doEnvironmentLogic();
    doOverlayLogic();
    doExplosionsLogic();
    doProjectilesLogic();
    doStructuresLogic();
    doUnitsLogic();
    
    // if the player was currently busy to make the unit deploy and that unit's dead/gone, make sure the playscreen doesn't require that function anymore
    if (playScreenTouchModifier == PSTM_DEPLOY_STRUCTURE && !unit[getDeployNrUnit()].enabled)
        playScreenTouchModifier = PSTM_NONE;
    
    // if the player was currently busy attacking a unit and that unit's dead/gone, make sure the playscreen's action issued doesn't go haywire
    if (graphicalActionIssued == GAI_ATTACK_UNIT && !unit[graphicalActionIssuedLocation].enabled) {
        graphicalActionIssued = GAI_NONE;
        graphicalActionIssuedTimer = 0;
    }
    
    if (getGameType() == SINGLEPLAYER)
        doAILogic();
    
    if (graphicalActionIssued != GAI_NONE) {
        if (++graphicalActionIssuedTimer >= GRAPHICAL_ACTION_DURATION) {
            graphicalActionIssued = GAI_NONE;
            graphicalActionIssuedTimer = 0;
        }
    } else if (graphicalActionIssuedTimer > 0) {
        if (++graphicalActionIssuedTimer >= DOUBLE_TAP_THRESHHOLD_RADAR)
            graphicalActionIssuedTimer = 0;
    }
    
    if (getFocusOnUnitNr() >= 0) {
        if ((getPrimaryHand() == RIGHT_HANDED && (keysHeld() & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN))) ||
            (getPrimaryHand() == LEFT_HANDED && (keysHeld() & (KEY_A | KEY_B | KEY_X | KEY_Y)))) // scrolling took place, focus now lost
            setFocusOnUnitNr(-1);
        else
            setViewCurrentXY(unit[getFocusOnUnitNr()].x - (HORIZONTAL_WIDTH / 2), unit[getFocusOnUnitNr()].y - (HORIZONTAL_HEIGHT / 2));
    }
    
    stopProfilingFunction();
}

void drawPlayScreenBG() {
    int i;
    
    REG_DISPCNT = (
                  MODE_0_3D |
                  DISPLAY_BG_EXT_PALETTE |
                  DISPLAY_BG0_ACTIVE |  /* sprites and more, using 3D */
                  DISPLAY_BG1_ACTIVE |  /* structure health */
                  DISPLAY_BG2_ACTIVE |  /* structures, overlay */
                  DISPLAY_BG3_ACTIVE |  /* bg for environment */
                  DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_256
                 );
    
    REG_BG0CNT = BG_PRIORITY_0;
    REG_BG1CNT = BG_PRIORITY_1 | BG_TILE_BASE(0) | BG_COLOR_256 | BG_MAP_BASE(29) | BG_32x32;
    REG_BG2CNT = BG_PRIORITY_2 | BG_TILE_BASE(4) | BG_COLOR_256 | BG_MAP_BASE(30) | BG_32x32;
    REG_BG3CNT = BG_PRIORITY_3 | BG_TILE_BASE(0) | BG_COLOR_256 | BG_MAP_BASE(31) | BG_32x32;
    
    for (i=0; i<768; i++) {
        mapBG1[i] = 0;
        mapBG2[i] = 0;
        mapBG3[i] = 0;
    }
}


void drawPlayScreen() {
    touchPosition touchXY, touchOrigin;
    int tile, tileOk, info, box_width, box_height;
    struct EnvironmentLayout *envLayout;
    int structure_t;
    int i, j, k;
    int x, y;
    int guiBarTouched;
    int forcedAttackAvailable, deployAvailable, healAvailable;
    
    startProfilingFunction("drawPlayScreen");
    
    initSpritesUsedPlayScreen();
    
    MATRIX_PUSH = 0;
    glTranslate3f32(0, getScreenScrollingPlayScreen(), 0);
    
    GFX_BEGIN = GL_QUADS; // assuming we only use QUADS
    
    /* zero out bg1 and bg2 */
    for (i=0; i<768; i++) {
        mapBG1[i] = 0;
        mapBG2[i] = 0;
    }
    
    touchXY = touchReadLast();
    touchOrigin = touchReadOrigin();
    guiBarTouched = getGameState() == INGAME && getScreenSetUp() == MAIN_TOUCH && (getKeysDown() & KEY_TOUCH) && touchReadOrigin().py < (ACTION_BAR_SIZE * 16) && touchXY.py < (ACTION_BAR_SIZE * 16);
    
    setPlayScreenSpritesMode(PSSM_EXPANDED);
    /* drag selection first thing displayed */
    if (!(playScreenTouchModifier == PSTM_PLACE_STRUCTURE || playScreenTouchModifier == PSTM_DEPLOY_STRUCTURE)) {
        if (getGameState() == INGAME && getScreenSetUp() == MAIN_TOUCH && getKeysHeldTimeMin(KEY_TOUCH) > 0) {
            if (touchOrigin.py >= ACTION_BAR_SIZE * 16 &&
                (touchXY.px >= touchOrigin.px + DRAG_SELECTION_THRESHHOLD || touchXY.px <= touchOrigin.px - DRAG_SELECTION_THRESHHOLD ||
                 touchXY.py >= touchOrigin.py + DRAG_SELECTION_THRESHHOLD || touchXY.py <= touchOrigin.py - DRAG_SELECTION_THRESHHOLD)) {
                /* going to count in 8px tiles instead of 16px as usual */
                if (touchXY.py < ACTION_BAR_SIZE * 16)
                    touchXY.py = ACTION_BAR_SIZE * 16;
                x = (min(touchXY.px, touchOrigin.px) / 8) * 8;
                y = (min(touchXY.py, touchOrigin.py) / 8) * 8;
                box_width = abs(touchXY.px / 8 - touchOrigin.px / 8) + 1;
                box_height = abs(touchXY.py / 8 - touchOrigin.py / 8) + 1;
                if (box_width <= 1) box_width++;
                
                /* draw top of the drag selection */
                setSpritePlayScreen(y, ATTR0_SQUARE,
                                    x, SPRITE_SIZE_8, 0, 0,
                                    0, PS_SPRITES_PAL_GUI, base_drag_select);
                for (i=1; i<box_width-1; i++)
                    setSpritePlayScreen(y, ATTR0_SQUARE,
                                        x + i*8, SPRITE_SIZE_8, 0, 0,
                                        0, PS_SPRITES_PAL_GUI, base_drag_select + 2);
                setSpritePlayScreen(y, ATTR0_SQUARE,
                                    x + (box_width-1) * 8, SPRITE_SIZE_8, 1, 0,
                                    0, PS_SPRITES_PAL_GUI, base_drag_select);
                /* draw sides of the drag selection */
                for (i=1; i<box_height-1; i++) {
                    y += 8;
                    setSpritePlayScreen(y, ATTR0_SQUARE,
                                        x, SPRITE_SIZE_8, 0, 0,
                                        0, PS_SPRITES_PAL_GUI, base_drag_select + 1);
                    setSpritePlayScreen(y, ATTR0_SQUARE,
                                        x + (box_width-1) * 8, SPRITE_SIZE_8, 1, 0,
                                        0, PS_SPRITES_PAL_GUI, base_drag_select + 1);
                }
                /* draw bottom of the drag selection */
                y += 8;
                setSpritePlayScreen(y, ATTR0_SQUARE,
                                    x, SPRITE_SIZE_8, 0, 1,
                                    0, PS_SPRITES_PAL_GUI, base_drag_select);
                for (i=1; i<box_width-1; i++)
                    setSpritePlayScreen(y, ATTR0_SQUARE,
                                        x + i*8, SPRITE_SIZE_8, 0, 1,
                                        0, PS_SPRITES_PAL_GUI, base_drag_select + 2);
                setSpritePlayScreen(y, ATTR0_SQUARE,
                                    x + (box_width-1) * 8, SPRITE_SIZE_8, 1, 1,
                                    0, PS_SPRITES_PAL_GUI, base_drag_select);
            }
        }
    }
    
    setPlayScreenSpritesMode(PSSM_EXPANDED);
    drawEnvironment();
    drawOverlay();
    
    /* everything needs to be in this order to
       ensure the sprites are properly layered */
    
    setPlayScreenSpritesMode(PSSM_EXPANDED);
    drawExplosions();
    drawProjectiles();
    drawUnits();
    drawStructures();
    
    // draw black borders around the BGs where needed (i.e. left, right, bottom) to hide BGs' wrap around
    /*if (getExplosionShiftX() > 0)
        setSpriteBorderPlayScreen(0, 0, getExplosionShiftX(), SCREEN_HEIGHT);
    else if (getExplosionShiftX() < 0)
        setSpriteBorderPlayScreen((SCREEN_WIDTH-1) + getExplosionShiftX(), 0, SCREEN_WIDTH, SCREEN_HEIGHT);*/
    if (getExplosionShiftX() != 0) { // B45, replacing the above two because 2D bgs and 3D layer are drawn independently
        setSpriteBorderPlayScreen(0, 0, MAX_EXPLOSION_SHAKE_X, SCREEN_HEIGHT);
        setSpriteBorderPlayScreen((SCREEN_WIDTH-1) + MAX_EXPLOSION_SHAKE_X, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    if (getExplosionShiftY() < 0)
        setSpriteBorderPlayScreen(0, (SCREEN_HEIGHT-1) + getExplosionShiftY(), SCREEN_WIDTH, SCREEN_HEIGHT);
    
    
    
    if (playScreenTouchModifier == PSTM_PLACE_STRUCTURE || playScreenTouchModifier == PSTM_DEPLOY_STRUCTURE || playScreenTouchModifier == PSTM_SELL_STRUCTURE) {
        setPlayScreenSpritesMode(PSSM_NORMAL);
        if (playScreenTouchModifier == PSTM_SELL_STRUCTURE)
            setSpritePlayScreen(0, ATTR0_SQUARE,
                                16, SPRITE_SIZE_16, 0, 0,
                                0, 0, base_action_icons + ACTION_ICON_SELL);
        else
            setSpritePlayScreen(0, ATTR0_SQUARE,
                                16, SPRITE_SIZE_16, 0, 0,
                                0, 0, base_action_icons + ACTION_ICON_PLACE);
        setSpritePlayScreen(0, ATTR0_SQUARE,
                            SCREEN_WIDTH - 32, SPRITE_SIZE_16, 0, 0,
                            0, 0, base_action_icons + ACTION_ICON_CANCEL);
        if (guiBarTouched) {
            if (touchXY.px >= 16 && touchXY.px < 32) {
                setSpritePlayScreen(0, ATTR0_SQUARE,
                                    16, SPRITE_SIZE_16, 0, 0,
                                    0, 0, base_action_icons + ACTION_ICON_SELECT);
                if (playScreenTouchModifier == PSTM_PLACE_STRUCTURE)
                    setSpritePlayScreen(0, ATTR0_WIDE,
                                        48 - 8, SPRITE_SIZE_32, 0, 0,
                                        0, 0, base_action_icons_tips + (ACTION_ICON_PLACE - 1) * 2);
                else
                    setSpritePlayScreen(0, ATTR0_WIDE,
                                        48 - 8, SPRITE_SIZE_32, 0, 0,
                                        0, 0, base_action_icons_tips + (ACTION_ICON_SELL - 1) * 2);
            } else if (touchXY.px >= SCREEN_WIDTH - 32 && touchXY.px < SCREEN_WIDTH - 16) {
                setSpritePlayScreen(0, ATTR0_SQUARE,
                                    (SCREEN_WIDTH - 16) - 16, SPRITE_SIZE_16, 0, 0,
                                    0, 0, base_action_icons + ACTION_ICON_SELECT);
                setSpritePlayScreen(0, ATTR0_WIDE,
                                    ((SCREEN_WIDTH - 48) - 16) - 8, SPRITE_SIZE_32, 0, 0,
                                    0, 0, base_action_icons_tips + (ACTION_ICON_CANCEL - 1) * 2);
            }
        }
    }
    if (playScreenTouchModifier == PSTM_PLACE_STRUCTURE || playScreenTouchModifier == PSTM_DEPLOY_STRUCTURE) {
        setPlayScreenSpritesMode(PSSM_EXPANDED);
        
        if (playScreenTouchModifier == PSTM_PLACE_STRUCTURE)
            info = getStructuresQueue(FRIENDLY)[queueNrPlaceStructure].itemInfo;
        else
            info = unitInfo[unit[queueNrPlaceStructure].info].contains_structure;
        tile = TILE_FROM_XY(getViewCurrentX() + touchPlaceStructure.px/16, getViewCurrentY() + touchPlaceStructure.py/16);
        
        for (i=0; i<structureInfo[info].height; i++) {
            for (j=0; j<structureInfo[info].width; j++) {
                tileOk = 1;
                envLayout = &environment.layout[tile + TILE_FROM_XY(j,i)];
                if ((envLayout->contains_unit != -1 && !(playScreenTouchModifier == PSTM_DEPLOY_STRUCTURE && envLayout->contains_unit == queueNrPlaceStructure)) ||
                    envLayout->graphics < ROCK || envLayout->graphics > ROCKCUSTOM32)
                    tileOk = 0;
                else {
                    structure_t = envLayout->contains_structure;
                    if (structure_t < -1)
                        structure_t = environment.layout[tile + TILE_FROM_XY(j + (structure_t + 1), i)].contains_structure;
                    if (structure_t >= 0) {
                        if (structureInfo[info].foundation || !structureInfo[structure[structure_t].info].foundation)
                            tileOk = 0;
                    } else { // no structure at this specific tile: is the buildable structure foundation?
                        if (foundationRequired() && !structureInfo[info].foundation && playScreenTouchModifier == PSTM_PLACE_STRUCTURE)
                            tileOk = 0;
                    }
                }
                if (tileOk && (playScreenTouchModifier != PSTM_PLACE_STRUCTURE || getDistanceToNearestStructure(FRIENDLY, X_FROM_TILE(tile) + j, Y_FROM_TILE(tile) + i) <= MAX_BUILDING_DISTANCE)) {
                    setSpritePlayScreen((touchPlaceStructure.py/16 + i + 1) * 16, ATTR0_SQUARE,
                                        (touchPlaceStructure.px/16 + j) * 16, SPRITE_SIZE_16, 0, 0,
                                        0, PS_SPRITES_PAL_GUI, base_place_structure);
                }
                else {
                    setSpritePlayScreen((touchPlaceStructure.py/16 + i + 1) * 16, ATTR0_SQUARE,
                                        (touchPlaceStructure.px/16 + j) * 16, SPRITE_SIZE_16, 0, 0,
                                        0, PS_SPRITES_PAL_GUI, base_place_structure + 1);
                }
            }
        }
    } else {
        
        if (playScreenTouchModifier != PSTM_SELL_STRUCTURE) {
            for (i=0; i<MAX_STRUCTURES_ON_MAP; i++) {
                if (structure[i].selected && structure[i].enabled && structure[i].side == FRIENDLY) {
                    j = 0;
                    if (guiBarTouched) {
                        if (!structureInfo[structure[i].info].barrier) {
                            for (j=1; j<=3; j++) {
                                if (touchXY.px >= -16 + 16 * 2 * j && touchXY.px < 16 * 2 * j)
                                    break;
                            }
                            if (j > 3) {
                                for (j=1; j<=3; j++) {
                                    if (touchXY.px >= SCREEN_WIDTH - 16 * 2 * j && touchXY.px < (SCREEN_WIDTH + 16) - 16 * 2 * j) {
                                        j = 8-j;
                                        break;
                                    }
                                }
                            }
                            
                        }
                    }
                    setPlayScreenSpritesMode(PSSM_NORMAL);
                    if (j == 7)
                        setSpritePlayScreen(0, ATTR0_WIDE, // cancel icon tooltip
                                            ((SCREEN_WIDTH - 48) - 16) - 8, SPRITE_SIZE_32, 0, 0,
                                            0, 0, base_action_icons_tips + (ACTION_ICON_CANCEL - 1) * 2);
                    if (!structureInfo[structure[i].info].barrier) {
                        setSpritePlayScreen(0, ATTR0_SQUARE, // repair icon
                                            16, SPRITE_SIZE_16, 0, 0,
                                            0, 0, base_action_icons + ACTION_ICON_REPAIR);
                        if ((j == 1 && !(structure[i].logic & 1)) || (j != 1 && (structure[i].logic & 1))) // repair select/active
                            setSpritePlayScreen(0, ATTR0_SQUARE,
                                                16, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_action_icons + ACTION_ICON_SELECT);
                        if (j == 1)
                            setSpritePlayScreen(0, ATTR0_WIDE,
                                                48 - 8, SPRITE_SIZE_32, 0, 0,
                                                0, 0, base_action_icons_tips + (ACTION_ICON_REPAIR - 1) * 2);
//                        if (structureInfo[structure[i].info].release_tile >= 0) {
                        if (structure[i].rally_tile >= 0) {
                            setSpritePlayScreen(0, ATTR0_SQUARE, // primary icon
                                                48, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_action_icons + ACTION_ICON_PRIMARY);
                            if ((j == 2 && !structure[i].primary) || (j != 2 && structure[i].primary)) // primary select/active
                                setSpritePlayScreen(0, ATTR0_SQUARE,
                                                    48, SPRITE_SIZE_16, 0, 0,
                                                    0, 0, base_action_icons + ACTION_ICON_SELECT);
                            if (j == 2)
                                setSpritePlayScreen(0, ATTR0_WIDE,
                                                    80 - 8, SPRITE_SIZE_32, 0, 0,
                                                    0, 0, base_action_icons_tips + (ACTION_ICON_PRIMARY - 1) * 2);
//                        }
//                        if (structure[i].rally_tile >= 0) {
                            setSpritePlayScreen(0, ATTR0_SQUARE, // rallypoint icon
                                                80, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_action_icons + ACTION_ICON_RALLYPOINT);
                            if ((j == 3 && playScreenTouchModifier != PSTM_RALLYPOINT_STRUCTURE) || (j != 3 && playScreenTouchModifier == PSTM_RALLYPOINT_STRUCTURE))
                                setSpritePlayScreen(0, ATTR0_SQUARE, // rallypoint select/active
                                                    80, SPRITE_SIZE_16, 0, 0,
                                                    0, 0, base_action_icons + ACTION_ICON_SELECT);
                            if (j == 3) {
                                setSpritePlayScreen(0, ATTR0_WIDE,
                                                    112 - 8, SPRITE_SIZE_32, 0, 0,
                                                    0, 0, base_action_icons_tips + (ACTION_ICON_RALLYPOINT - 1) * 2);
                            }
                        }
                        setSpritePlayScreen(0, ATTR0_SQUARE, // sell icon
                                            (SCREEN_WIDTH - 48) - 16, SPRITE_SIZE_16, 0, 0,
                                            0, 0, base_action_icons + ACTION_ICON_SELL);
                        if (j == 6) { // sell select
                            setSpritePlayScreen(0, ATTR0_SQUARE,
                                                (SCREEN_WIDTH - 48) - 16, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_action_icons + ACTION_ICON_SELECT);
                            setSpritePlayScreen(0, ATTR0_WIDE,
                                                ((SCREEN_WIDTH - 80) - 16) - 8, SPRITE_SIZE_32, 0, 0,
                                                0, 0, base_action_icons_tips + (ACTION_ICON_SELL - 1) * 2);
                        }
                    }
                    setSpritePlayScreen(0, ATTR0_SQUARE, // cancel icon
                                        SCREEN_WIDTH - 32, SPRITE_SIZE_16, 0, 0,
                                        0, 0, base_action_icons + ACTION_ICON_CANCEL);
                    if (j == 7)
                        setSpritePlayScreen(0, ATTR0_SQUARE, // cancel icon select
                                            (SCREEN_WIDTH - 16) - 16, SPRITE_SIZE_16, 0, 0,
                                            0, 0, base_action_icons + ACTION_ICON_SELECT);
                    break;
                }
            }
            if (i == MAX_STRUCTURES_ON_MAP) {
                for (i=0; i<MAX_UNITS_ON_MAP; i++) {
                    if (unit[i].selected && unit[i].enabled && unit[i].side == FRIENDLY) {
                        j = 0;
                        if (guiBarTouched) {
                            for (j=1; j<=3; j++) {
                                if (touchXY.px >= -16 + 16 * 2 * j && touchXY.px < 16 * 2 * j)
                                    break;
                            }
                            if (j > 3) {
                                for (j=1; j<=3; j++) {
                                    if (touchXY.px >= SCREEN_WIDTH - 16 * 2 * j && touchXY.px < (SCREEN_WIDTH + 16) - 16 * 2 * j) {
                                        j = 8-j;
                                        break;
                                    }
                                }
                            }
                        }
                        setPlayScreenSpritesMode(PSSM_NORMAL);
                        
                        forcedAttackAvailable = 0;
                        deployAvailable = 0;
                        healAvailable = 1;
                        for (k=0; k<MAX_UNITS_ON_MAP; k++) {
                            if (unit[k].selected) {
                                if (unitInfo[unit[k].info].shoot_range) {
                                    if (!unitInfo[unit[k].info].can_heal_foot) {
                                        healAvailable = 0;
                                        forcedAttackAvailable = 1;
                                        deployAvailable = 0;
                                        break;
                                    }
                                    deployAvailable = -1;
                                } else
                                    healAvailable = 0;
                                if (deployAvailable == 0 && unitInfo[unit[k].info].contains_structure >= 0 &&
                                    (unit[k].move == UM_MOVE_HOLD || (unit[k].move == UM_NONE && unit[k].logic == UL_GUARD)) &&
                                    unit[k].x >= getViewCurrentX() && unit[k].x < getViewCurrentX() + HORIZONTAL_WIDTH &&
                                    unit[k].y >= getViewCurrentY() && unit[k].y < getViewCurrentY() + HORIZONTAL_HEIGHT)
                                    deployAvailable = 1;
                                else
                                    deployAvailable = -1;
                            }
                        }
                        if (deployAvailable == -1)
                            deployAvailable = 0;
                        
                        if (j == 1 && forcedAttackAvailable)
                            setSpritePlayScreen(0, ATTR0_WIDE, // attack icon tooltip
                                                48 - 8, SPRITE_SIZE_32, 0, 0,
                                                0, 0, base_action_icons_tips + (ACTION_ICON_ATTACK - 1) * 2);
                        else if (j == 1 && healAvailable)
                            setSpritePlayScreen(0, ATTR0_WIDE, // heal icon tooltip
                                                48 - 8, SPRITE_SIZE_32, 0, 0,
                                                0, 0, base_action_icons_tips + (ACTION_ICON_HEAL - 1) * 2);
                        else if (j == 1 && deployAvailable)
                            setSpritePlayScreen(0, ATTR0_WIDE, // place icon tooltip
                                                48 - 8, SPRITE_SIZE_32, 0, 0,
                                                0, 0, base_action_icons_tips + (ACTION_ICON_PLACE - 1) * 2);
                        else if (j == 2)
                            setSpritePlayScreen(0, ATTR0_WIDE, // move icon tooltip
                                                80 - 8, SPRITE_SIZE_32, 0, 0,
                                                0, 0, base_action_icons_tips + (ACTION_ICON_MOVE - 1) * 2);
                        else if (j == 3)
                            setSpritePlayScreen(0, ATTR0_WIDE, // guard icon tooltip
                                                112 - 8, SPRITE_SIZE_32, 0, 0,
                                                0, 0, base_action_icons_tips + (ACTION_ICON_GUARD - 1) * 2);
                        else if (j == 5)
                            setSpritePlayScreen(0, ATTR0_WIDE, // retreat icon tooltip
                                                ((SCREEN_WIDTH - 112) - 16) - 8, SPRITE_SIZE_32, 0, 0,
                                                0, 0, base_action_icons_tips + (ACTION_ICON_RETREAT - 1) * 2);
                        else if (j == 6) {
                            for (k=i+1; k<MAX_UNITS_ON_MAP && (!unit[k].selected || !unit[k].enabled); k++);
                            if (k == MAX_UNITS_ON_MAP)
                                setSpritePlayScreen(0, ATTR0_WIDE, // focus icon tooltip
                                                    ((SCREEN_WIDTH - 80) - 16) - 8, SPRITE_SIZE_32, 0, 0,
                                                    0, 0, base_action_icons_tips + (ACTION_ICON_FOCUS - 1) * 2);
                        }
                        else if (j == 7)
                            setSpritePlayScreen(0, ATTR0_WIDE, // cancel icon tooltip
                                                ((SCREEN_WIDTH - 48) - 16) - 8, SPRITE_SIZE_32, 0, 0,
                                                0, 0, base_action_icons_tips + (ACTION_ICON_CANCEL - 1) * 2);
                        
                        if (forcedAttackAvailable) {
                            setSpritePlayScreen(0, ATTR0_SQUARE, // attack icon
                                                16, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_action_icons + ACTION_ICON_ATTACK);
                            if ((j == 1 && playScreenTouchModifier != PSTM_ATTACK) || (j != 1 && playScreenTouchModifier == PSTM_ATTACK)) // attack select/active
                                setSpritePlayScreen(0, ATTR0_SQUARE,
                                                    16, SPRITE_SIZE_16, 0, 0,
                                                    0, 0, base_action_icons + ACTION_ICON_SELECT);
                        } else if (healAvailable) {
                            setSpritePlayScreen(0, ATTR0_SQUARE, // heal icon
                                                16, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_action_icons + ACTION_ICON_HEAL);
                            if ((j == 1 && playScreenTouchModifier != PSTM_ATTACK) || (j != 1 && playScreenTouchModifier == PSTM_ATTACK)) // attack select/active
                                setSpritePlayScreen(0, ATTR0_SQUARE,
                                                    16, SPRITE_SIZE_16, 0, 0,
                                                    0, 0, base_action_icons + ACTION_ICON_SELECT);
                        } else if (deployAvailable) {
                            setSpritePlayScreen(0, ATTR0_SQUARE, // place icon
                                                16, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_action_icons + ACTION_ICON_PLACE);
                            if (j == 1)
                                setSpritePlayScreen(0, ATTR0_SQUARE,
                                                    16, SPRITE_SIZE_16, 0, 0,
                                                    0, 0, base_action_icons + ACTION_ICON_SELECT);
                        }
                        setSpritePlayScreen(0, ATTR0_SQUARE, // move icon
                                            48, SPRITE_SIZE_16, 0, 0,
                                            0, 0, base_action_icons + ACTION_ICON_MOVE);
                        if ((j == 2 && playScreenTouchModifier != PSTM_MOVE) || (j != 2 && playScreenTouchModifier == PSTM_MOVE)) // move select/active
                            setSpritePlayScreen(0, ATTR0_SQUARE,
                                                48, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_action_icons + ACTION_ICON_SELECT);
                        setSpritePlayScreen(0, ATTR0_SQUARE, // guard icon
                                            80, SPRITE_SIZE_16, 0, 0,
                                            0, 0, base_action_icons + ACTION_ICON_GUARD);
                        if (j == 3) // guard select
                            setSpritePlayScreen(0, ATTR0_SQUARE,
                                                80, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_action_icons + ACTION_ICON_SELECT);
                        setSpritePlayScreen(0, ATTR0_SQUARE, // retreat icon
                                            (SCREEN_WIDTH - 80) - 16, SPRITE_SIZE_16, 0, 0,
                                            0, 0, base_action_icons + ACTION_ICON_RETREAT);
                        if (j == 5) // retreat select
                            setSpritePlayScreen(0, ATTR0_SQUARE,
                                                (SCREEN_WIDTH - 80) - 16, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_action_icons + ACTION_ICON_SELECT);
                        for (k=i+1; k<MAX_UNITS_ON_MAP && (!unit[k].selected || !unit[k].enabled); k++);
                        if (k == MAX_UNITS_ON_MAP) {
                            setSpritePlayScreen(0, ATTR0_SQUARE, // focus icon
                                                (SCREEN_WIDTH - 48) - 16, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_action_icons + ACTION_ICON_FOCUS);
                            if ((j == 6 && getFocusOnUnitNr() == -1) || (j != 6 && getFocusOnUnitNr() == i))
                                setSpritePlayScreen(0, ATTR0_SQUARE, // focus icon select
                                                    (SCREEN_WIDTH - 48) - 16, SPRITE_SIZE_16, 0, 0,
                                                    0, 0, base_action_icons + ACTION_ICON_SELECT);
                            
                        }
                        setSpritePlayScreen(0, ATTR0_SQUARE, // cancel icon
                                            (SCREEN_WIDTH - 16) - 16, SPRITE_SIZE_16, 0, 0,
                                            0, 0, base_action_icons + ACTION_ICON_CANCEL);
                        if (j == 7)
                            setSpritePlayScreen(0, ATTR0_SQUARE, // cancel icon select
                                                (SCREEN_WIDTH - 16) - 16, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_action_icons + ACTION_ICON_SELECT);
                        break;
                    }
                }
            }
        }
    }
    
    setPlayScreenSpritesMode(PSSM_NORMAL);
    setSpritePlayScreen(0, ATTR0_SQUARE,
                        120, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_action_icons + ACTION_ICON_SWAPSCREENS);
    if (guiBarTouched && touchXY.px >= 120 && touchXY.px < 136) {
        setSpritePlayScreen(0, ATTR0_SQUARE,
                            120, SPRITE_SIZE_16, 0, 0,
                            0, 0, base_action_icons + ACTION_ICON_SELECT);
        setSpritePlayScreen(20, ATTR0_WIDE,
                            120 - 8, SPRITE_SIZE_32, 0, 0,
                            0, 0, base_action_icons_tips + (ACTION_ICON_SWAPSCREENS - 1) * 2);
    }
    
    // draw gui bar bg
    setPlayScreenSpritesMode(PSSM_NORMAL);
    
    setSpritePlayScreen(0, ATTR0_SQUARE,
                        0, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_bar);
    setSpritePlayScreen(0, ATTR0_SQUARE,
                        SCREEN_WIDTH - 16, SPRITE_SIZE_16, 1, 0,
                        0, 0, base_bar);
    for (i=0; i<3; i++) {
        setSpritePlayScreen(0, ATTR0_SQUARE,
                            16 + i*32, SPRITE_SIZE_16, 0, 0,
                            0, 0, base_bar + 2);
        setSpritePlayScreen(0, ATTR0_SQUARE,
                            32 + i*32, SPRITE_SIZE_16, 0, 0,
                            0, 0, base_bar + 1);
        setSpritePlayScreen(0, ATTR0_SQUARE,
                            (SCREEN_WIDTH - 32) - i*32, SPRITE_SIZE_16, 0, 0,
                            0, 0, base_bar + 2);
        setSpritePlayScreen(0, ATTR0_SQUARE,
                            (SCREEN_WIDTH - 48) - i*32, SPRITE_SIZE_16, 0, 0,
                            0, 0, base_bar + 1);
    }
    setSpritePlayScreen(0, ATTR0_SQUARE,
                        SCREEN_WIDTH/2 - 24, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_bar + 1);
    setSpritePlayScreen(0, ATTR0_SQUARE,
                        SCREEN_WIDTH/2 + 8, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_bar + 1);
    setSpritePlayScreen(0, ATTR0_SQUARE,
                        SCREEN_WIDTH/2 - 8, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_bar + 2);
    
    
/*  for (i=0; i<(256/16)/2; i++) {
        setSpritePlayScreen(0, ATTR0_SQUARE,
                            i * 16, SPRITE_SIZE_16, 0, 0,
                            0, 0, base_bar + i);
        setSpritePlayScreen(0, ATTR0_SQUARE,
                            (256-16) - (i * 16), SPRITE_SIZE_16, 1, 0,
                            0, 0, base_bar + i);
    }
*/
    
    // draw graphical action issued
    if (graphicalActionIssued != GAI_NONE) {
        if (graphicalActionIssued == GAI_MOVE_LOCATION) {
            i = base_action_move;
            i += (graphicalActionIssuedTimer / (GRAPHICAL_ACTION_DURATION / 5));
        } else if (graphicalActionIssued >= GAI_HEAL_LOCATION) {
            i = base_action_heal;
            i += ((graphicalActionIssuedTimer / (GRAPHICAL_ACTION_DURATION / 5)) % 2);
        } else {
            i = base_action_attack;
            i += (graphicalActionIssuedTimer / (GRAPHICAL_ACTION_DURATION / 5));
        }
        
        if (graphicalActionIssued == GAI_ATTACK_UNIT || graphicalActionIssued == GAI_HEAL_UNIT) {
            x = unit[graphicalActionIssuedLocation].x * 16;
            y = unit[graphicalActionIssuedLocation].y * 16;
            alterXYAccordingToUnitMovement(unit[graphicalActionIssuedLocation].move, unit[graphicalActionIssuedLocation].move_aid, unitInfo[unit[graphicalActionIssuedLocation].info].speed, &x, &y);
        } else {
            x = X_FROM_TILE(graphicalActionIssuedLocation) * 16;
            y = Y_FROM_TILE(graphicalActionIssuedLocation) * 16;
        }
        x = (x - (getViewCurrentX() * 16));
        y = (y - (getViewCurrentY() * 16)) + (ACTION_BAR_SIZE * 16);
        if (x > -16 && x < SCREEN_WIDTH && y > -16 && y < SCREEN_HEIGHT)
            setSpritePlayScreen(y, ATTR0_SQUARE,
                                x, SPRITE_SIZE_16, 0, 0,
                                0, 0, i);
    }
    
    GFX_END = 0; // assuming we only use QUADS
    glFlush(0);
    
    MATRIX_POP = 1;
    
    stopProfilingFunction();
}

void loadPlayScreenGraphics() {
    char filename[320];
    char oneline[256];
    char *filePosition;
    int offsetBg, offsetSp;
    int i;
    FILE *fp;
    
    // first loading in palettes
    
    BG_PALETTE[0] = 0;
    
    VRAM_E_CR = VRAM_ENABLE | VRAM_E_LCD;
    VRAM_F_CR = VRAM_ENABLE | VRAM_F_LCD;
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    // GRAPHICS section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[GRAPHICS]", strlen("[GRAPHICS]")));
    
    readstr(fp, oneline);
    replaceEOLwithEOF(oneline, 255);
    if (oneline[strlen("Environment=")] != 0)
        sprintf(filename, "ingamePlayscreenBGs_%s_environment", oneline + strlen("Environment="));
    else
        strcpy(filename, "ingamePlayscreenBGs_environment");
    copyFileVRAM(BG_EXPANDED_PAL + (8192/2)*PS_BG_PAL_SLOT_ENVIRONMENT, filename, FS_PALETTES);
    
    filePosition = filename + strlen(filename);
    for (i=1; i<16; i++) {
        sprintf(filePosition, "%i", i+1);
        if (!copyFileVRAM(BG_EXPANDED_PAL + (8192/2)*PS_BG_PAL_SLOT_ENVIRONMENT + 256 * i, filename, FS_OPTIONAL_ENVIRONMENT_PALETTES))
        //{
            //error("could not open palette", filename);
            break;
        //}
    }
    
    
    readstr(fp, oneline);
    replaceEOLwithEOF(oneline, 255);
    if (oneline[strlen("Overlay=")] != 0)
        sprintf(filename, "ingamePlayscreenBGs_%s_overlay", oneline + strlen("Overlay="));
    else
        strcpy(filename, "ingamePlayscreenBGs_overlay");
    copyFileVRAM(BG_EXPANDED_PAL + (8192/2)*PS_BG_PAL_SLOT_OVERLAY_AND_STRUCTURES + 256*PS_BG_PAL_OVERLAY, filename, FS_PALETTES);
    
    readstr(fp, oneline);
    replaceEOLwithEOF(oneline, 255);
    if (oneline[strlen("Structures=")] != 0)
        sprintf(filename, "ingamePlayscreenBGs_%s_structures", oneline + strlen("Structures="));
    else
        strcpy(filename, "ingamePlayscreenBGs_structures");
    copyFileVRAM(BG_EXPANDED_PAL + (8192/2)*PS_BG_PAL_SLOT_OVERLAY_AND_STRUCTURES + 256*PS_BG_PAL_STRUCTURES, filename, FS_PALETTES);
    
    readstr(fp, oneline);
    replaceEOLwithEOF(oneline, 255);
    if (oneline[0] == 'S' && oneline[strlen("Shroud=")] != 0)
        sprintf(filename, "ingamePlayscreenSprites_%s_shroud", oneline + strlen("Shroud="));
    else
        strcpy(filename, "ingamePlayscreenSprites_shroud");
    copyFileVRAM(SPRITE_EXPANDED_PAL + 256*PS_SPRITES_PAL_SHROUD, filename, FS_PALETTES);
    
    closeFile(fp);
    
    copyFileVRAM(SPRITE_PALETTE, "ingamePlayscreenSprites_gui", FS_PALETTES);
    
    VRAM_E_CR = VRAM_ENABLE | VRAM_E_BG_EXT_PALETTE;
    VRAM_F_CR = VRAM_ENABLE | VRAM_F_TEX_PALETTE;
    
    // now loading in actual graphics
    
    for (i=0; i<16*16/2; i++) { // transparent tiles
        ((uint16*)CHAR_BASE_BLOCK(0))[i] = 0;
        ((uint16*)CHAR_BASE_BLOCK(4))[i] = 0;
    }
    
    offsetBg = 16*16; // offsetBg for char_base_block(4), which is for BG2 only
    loadStructuresGraphicsBG(CHAR_BASE_BLOCK(4), &offsetBg);
    loadOverlayGraphicsBG(CHAR_BASE_BLOCK(4), &offsetBg);
    
/*        
// HACK!
if ((keysHeld() & (KEY_L | KEY_R)) == (KEY_L | KEY_R)) {
    char oneline[256];
    sprintf(oneline, "Sprites VRAM total: 128 kb\n            in use: %i b, which is %i kb\n              free: %i b, which is %i kb\n"
                     "\n"
                     "Backgr. VRAM total:  64 kb\n            in use: %i b, which is %i kb\n              free: %i b, which is %i kb\n",
                     offsetSp, offsetSp/1024, (128*1024)-offsetSp, ((128*1024)-offsetSp)/1024,
                     offsetBg, offsetBg/1024, (64*1024)-offsetBg, ((64*1024)-offsetBg)/1024);
    error("memory usage", oneline);
}
*/
    
    if (offsetBg > 64 * 1024)
        errorSI("BG for playscreen exceeding VRAM limit,\nstructures and overlay,\nmeasured in bytes:", offsetBg - 64 * 1024);

    offsetBg = 16*16; // offsetBg for char_base_block(0), which is for BG0, BG1 and BG3
    loadStructuresSelectionGraphicsBG(CHAR_BASE_BLOCK(0), &offsetBg);
    loadEnvironmentGraphicsBG(CHAR_BASE_BLOCK(0), &offsetBg);
    loadExplosionsGraphicsBG(CHAR_BASE_BLOCK(0), &offsetBg);  // doesn't have BG graphics actually
    loadProjectilesGraphicsBG(CHAR_BASE_BLOCK(0), &offsetBg); // doesn't have BG graphics actually
    loadUnitsGraphicsBG(CHAR_BASE_BLOCK(0), &offsetBg);      // doesn't have BG graphics actually

    offsetSp = 0;
    base_bar = offsetSp/(16*16);
    strcpy(filename, "bar_");
    strcat(filename, factionInfo[getFaction(FRIENDLY)].name);
    offsetSp += copyFileVRAM(SPRITE_GFX + (offsetSp>>1), filename, FS_GUI_GRAPHICS);
    
    base_action_icons = offsetSp/(16*16);
    offsetSp += copyFileVRAM(SPRITE_GFX + (offsetSp>>1), "action_icons", FS_GUI_GRAPHICS);
    base_action_icons_tips = offsetSp/(16*16);
    offsetSp += copyFileVRAM(SPRITE_GFX + (offsetSp>>1), "action_icons_tips", FS_GUI_GRAPHICS);
    
    base_action_attack = offsetSp/(16*16);
    offsetSp += copyFileVRAM(SPRITE_GFX + (offsetSp>>1), "action_attack", FS_GUI_GRAPHICS);
    base_action_move = offsetSp/(16*16);
    offsetSp += copyFileVRAM(SPRITE_GFX + (offsetSp>>1), "action_move", FS_GUI_GRAPHICS);
    base_action_heal = offsetSp/(16*16);
    offsetSp += copyFileVRAM(SPRITE_GFX + (offsetSp>>1), "action_heal", FS_GUI_GRAPHICS);
    
    loadStructureRallyGraphicsSprites(&offsetSp);
    
    if (offsetSp > 16*1024)
        errorSI("2D Sprites for playscreen exceeding VRAM limit. Measured in bytes:", offsetSp - 16*1024);
    
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_LCD;
    loadEnvironmentGraphicsSprites(0);
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_TEXTURE_SLOT0;
}

void preloadPlayScreen3DGraphics() {
    uint16 buffer[3 * 8*8 / 2];
    char filename[256];
    int offsetSp;
    int i;
    
    // first loading in palettes
    
    VRAM_E_CR = VRAM_ENABLE | VRAM_E_LCD;
    VRAM_F_CR = VRAM_ENABLE | VRAM_F_LCD;
    
    copyFileVRAM(BG_EXPANDED_PAL + (8192/2)*PS_BG_PAL_SLOT_GUI, "ingamePlayscreenBGs_gui", FS_PALETTES);
    
    copyFileVRAM(SPRITE_EXPANDED_PAL + 256*PS_SPRITES_PAL_PROJECTILES, "ingamePlayscreenSprites_projectiles", FS_PALETTES);
    copyFileVRAM(SPRITE_EXPANDED_PAL + 256*PS_SPRITES_PAL_EXPLOSIONS,  "ingamePlayscreenSprites_explosions", FS_PALETTES);
    copyFileVRAM(SPRITE_EXPANDED_PAL + 256*PS_SPRITES_PAL_SMOKE,       "ingamePlayscreenSprites_smoke", FS_PALETTES);
    copyFileVRAM(SPRITE_EXPANDED_PAL + 256*PS_SPRITES_PAL_GUI,         "ingamePlayscreenSprites_gui", FS_PALETTES);
    
    strcpy(filename, "ingamePlayscreenSprites_");
    for (i=0; i<MAX_DIFFERENT_FACTIONS && factionInfo[i].enabled; i++) {
        strcpy(filename + strlen("ingamePlayscreenSprites_"), factionInfo[i].name);
        copyFileVRAM(SPRITE_EXPANDED_PAL + 256*(PS_SPRITES_PAL_FACTIONS+i), filename, FS_PALETTES);
    }
    
    VRAM_E_CR = VRAM_ENABLE | VRAM_E_BG_EXT_PALETTE;
    VRAM_F_CR = VRAM_ENABLE | VRAM_F_TEX_PALETTE;
    
    // now loading in actual graphics
    
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_LCD;
    
    offsetSp = 0;
    offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX, "Smoke", FS_SMOKE_GRAPHICS); // used by both structures and units, needs to be at offset 0
    loadOverlayGraphicsSprites(&offsetSp); // doesn't have Sprites graphics actually
    loadStructuresGraphicsSprites(&offsetSp);
    loadExplosionsGraphicsSprites(&offsetSp);
    loadProjectilesGraphicsSprites(&offsetSp);
    loadUnitsGraphicsSprites(&offsetSp);
    
    copyFile(buffer, "drag_select", FS_GUI_GRAPHICS);
    base_drag_select = offsetSp/(16*16);
    for (i=0; i<32; i++) {
        *(SPRITE_EXPANDED_GFX + ((offsetSp)>>1) + i) = buffer[i];
        *(SPRITE_EXPANDED_GFX + ((offsetSp + (16*16))>>1) + i) = buffer[32 + i];
        *(SPRITE_EXPANDED_GFX + ((offsetSp + (2*16*16))>>1) + i) = buffer[64 + i];
    }
    offsetSp += 3 * 16*16;
    
    base_place_structure = offsetSp/(16*16);
    offsetSp += copyFileVRAM(SPRITE_EXPANDED_GFX + (offsetSp>>1), "place_structure", FS_GUI_GRAPHICS);
    
    loadEnvironmentGraphicsSprites(&offsetSp); // should be last due to it being different per scenario.
    
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_TEXTURE_SLOT0;
    
    if (offsetSp > 128 * 1024)
        errorSI("Sprites for playscreen exceeding VRAM limit,\nmeasured in bytes:", offsetSp - 128 * 1024);
}

void initPlayScreen() {
    int i;
    int done = 0;
    
    REG_DISPCNT = MODE_0_2D;
    
    playScreenTouchModifier = PSTM_NONE;
    graphicalActionIssued = GAI_NONE;
    graphicalActionIssuedTimer = 0;
    
    initFactionsWithScenario();
    initEnvironmentWithScenario();
    initOverlayWithScenario();
    initExplosionsWithScenario();
    initProjectilesWithScenario();
    initUnitsWithScenario();
    initStructuresWithScenario();
    initSoundeffectsWithScenario();
    #ifndef REMOVE_ASTAR_PATHFINDING
    initPathfindingWithScenario();
    #endif
    
    if (getGameType() == SINGLEPLAYER)
        initAI();
    
    // readjust view to center around first friendly structure or unit
    for (i=MAX_DIFFERENT_FACTIONS; i<MAX_STRUCTURES_ON_MAP && structure[i].enabled; i++) {
        if (structure[i].side == FRIENDLY) {
            setViewCurrentXY(structure[i].x + structureInfo[structure[i].info].width/2 - HORIZONTAL_WIDTH/2, structure[i].y + structureInfo[structure[i].info].height/2 - HORIZONTAL_HEIGHT/2);
            done = 1;
            break;
        }
    }
    if (!done) {
        for (i=0; i<MAX_UNITS_ON_MAP && unit[i].enabled; i++) {
            if (unit[i].side == FRIENDLY) {
                setViewCurrentXY(unit[i].x - HORIZONTAL_WIDTH/2, unit[i].y - HORIZONTAL_HEIGHT/2);
                done = 1;
                break;
            }
        }
    }
}
