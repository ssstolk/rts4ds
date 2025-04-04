// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "pathfinding.h"
#ifndef REMOVE_ASTAR_PATHFINDING

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "environment.h"
#include "factions.h"
#include "structures.h"
#include "units.h"
#include "profiling.h"
#include "gameticks.h"
#include "shared.h"
#include "debug.h"
#include "profiling.h"

#define MAX_VCOUNT             (190) /* should be lower than 192, VBlank */
#define MAX_GAMETICKS_IN_FRAME (GAMETICKS_PER_FRAME - (((192-MAX_VCOUNT) * GAMETICKS_PER_FRAME) / 262))
#define MAX_QUEUED_TIME        (FPS/2) // in logic frames. currently half a second.
#define ADDITIONAL_FRAME_ALLOWED_DELAY   (FPS/4) // in logic frames. currently four times a second.

#define MAX_PATHFINDING_PATHS  (MAX_UNITS_ON_MAP)  /* MAX_UNITS_ON_MAP is the max ever required (== +- 6.5 kb) */
#define THRESHHOLD_TRAVERSABILITY_CHECK    50      // this is no longer related to GSCORE

#define PATHFINDING_REQUEUE_TIMER   2

#define THRESHHOLD_DANCE_DISTANCE   (DIAGONAL_REAL_DISTANCE*2)
#define THRESHHOLD_SETTLE_DISTANCE  (DIAGONAL_REAL_DISTANCE*3)

struct Path pathfindingPath[MAX_PATHFINDING_PATHS];

struct Path *astarProcessingPath;

unsigned maxGameticksSearch;
unsigned additionalFrameAllowedDelay;

// ********************************************************************************************
// the callback functions:

unsigned int mapCanBeTraversed_callback (unsigned int node, unsigned int g_score, void *cur_unit, void *cur_unitinfo) {
    if (((struct Unit *)cur_unit)->side == FRIENDLY && environment.layout[node].status == UNDISCOVERED)
        return (0);

    // map traversable?
    if (environment.layout[node].traversability == TRAVERSABLE) {
        return (g_score > THRESHHOLD_TRAVERSABILITY_CHECK ||           
                !isTileBlockedByOtherUnit((struct Unit *) cur_unit, node));
    }

    // map is not empty
    if (environment.layout[node].traversability == UNTRAVERSABLE)
        return (0);

    // map is definitely of a type only traversable by non-tracked vehicles
    if (((struct UnitInfo *)cur_unitinfo)->type == UT_TRACKED)
        return (0);

    // unit type is able to traverse aforementioned terrain type
    return (g_score > THRESHHOLD_TRAVERSABILITY_CHECK ||
            !isTileBlockedByOtherUnit((struct Unit *) cur_unit, node));
}

unsigned int canHitTarget_callback (unsigned int start, unsigned int goal, void *cur_unit, void *cur_unitinfo) {
    return withinShootRange(X_FROM_TILE(start), Y_FROM_TILE(start),            // start x,y 
                            ((struct UnitInfo*)cur_unitinfo)->shoot_range,     // shoot range
                            ((struct UnitInfo*)cur_unitinfo)->projectile_info, // projectile info
                            X_FROM_TILE(goal), Y_FROM_TILE(goal));             // goal x,y
}

unsigned int canContinueSearch_callback(void) {
    unsigned gameticks = getGameticks();
    return ((gameticks != 0) && (gameticks < maxGameticksSearch));
}

// ********************************************************************************************



void initPathfinding() {
    Astar_callback(&mapCanBeTraversed_callback,
                   &canHitTarget_callback,
                   &canContinueSearch_callback);
}

void initPathfindingWithScenario() {
    int i;
    
    Astar_config(environment.width, environment.height);
    astarProcessingPath = 0;
    
    for (i=0; i<MAX_PATHFINDING_PATHS; i++)
        pathfindingPath[i].unit_nr = -1;
    
    additionalFrameAllowedDelay = ADDITIONAL_FRAME_ALLOWED_DELAY;
}

struct Path *getQueuedPathfindingPathSearch() {
    struct Path *chosen=0;
    int queueMin=0;
    unsigned short int queuedTimeMin=0;
    int i;
    
    for (i=0; i<MAX_PATHFINDING_PATHS; i++) {
        if (pathfindingPath[i].unit_nr != -1 && pathfindingPath[i].queue != PQ_UNQUEUED
            && ((!chosen) || 
                (pathfindingPath[i].queue < queueMin) ||
                (pathfindingPath[i].queue == queueMin && pathfindingPath[i].queuedTime < queuedTimeMin))) {
            chosen = pathfindingPath + i;
            queueMin      = chosen->queue;
            queuedTimeMin = chosen->queuedTime;
        }
    }
    
    return chosen;
}


void queuePathfindingPathSearch(int unit_nr, int curX, int curY, bool attack, int shoot_range, int newX, int newY, bool priority) {
    int i;
    struct Path *path = 0;

    removePathfindingPath(unit_nr);
   
    for (i=0; i<MAX_PATHFINDING_PATHS; i++) {
        if (pathfindingPath[i].unit_nr == -1) {
            path = pathfindingPath + i;
            break;
        }
    }  
   
    if (!path)
        return;
   
    path->unit_nr = unit_nr;
    path->queue   = priority?PQ_HIGHPRIORITY:PQ_NORMALPRIORITY;
    path->dest    = TILE_FROM_XY(newX, newY);
    path->attack  = attack;
    path->size    = shoot_range;
    path->offset  = TILE_FROM_XY(curX, curY);
    path->queuedTime = 0;
    
    #ifdef DEBUG_BUILD
    if (path->dest < 0 || path->dest >= environment.width * environment.height) errorSI("dest@queuePathfindingPathSearch", path->dest);
    if (path->offset < 0 || path->offset >= environment.width * environment.height) errorSI("cur@queuePathfindingPathSearch", path->offset);
    #endif
}

enum UnitMove obtainNextMove(struct Path *path, int cur_tile) {
    if (path->tile[path->offset] == cur_tile) {
        path->offset++;
        if (path->offset >= path->size)
            return (!path->attack && cur_tile != path->dest) ?
                    UM_MOVE_HOLD :
                    UM_NONE;
        if (path->offset >= PATHBUFFERLEN)
            return UM_MOVE_HOLD;
    }
    
    int dest_tile = path->tile[path->offset];
    
    return UM_MOVE_UP + (int) positionedToFaceXY(X_FROM_TILE(cur_tile), Y_FROM_TILE(cur_tile),
                                                 X_FROM_TILE(dest_tile), Y_FROM_TILE(dest_tile));
}

struct Path *locatePathfindingPath(int unit_nr, bool attack, int dest) {
    int i;
    
    for (i=0; i<MAX_PATHFINDING_PATHS; i++) {
        if (pathfindingPath[i].unit_nr == unit_nr &&
            pathfindingPath[i].attack  == attack &&
            pathfindingPath[i].dest    == dest)
        return &pathfindingPath[i];
    }
    
    return 0;
}

enum UnitMove searchPathfindingPath(int unit_nr, int curX, int curY, bool attack, int shoot_range, int newX, int newY, bool priority) {
    struct Path *path = locatePathfindingPath(unit_nr, attack, TILE_FROM_XY(newX, newY));
    if (!path) {
        #ifdef DEBUG_BUILD
        if (TILE_FROM_XY(newX, newY) < 0 || TILE_FROM_XY(newX, newY) >= environment.width * environment.height) errorSI("dest@searchPathfindingPath-1", TILE_FROM_XY(newX, newY));
        #endif
        queuePathfindingPathSearch(unit_nr, curX, curY, attack, shoot_range, newX, newY, priority); // add to queue for searching new path
        return UM_MOVE_HOLD_INITIAL;
    }
    
    if (path->queue!=PQ_UNQUEUED) {
        if (path->attack != attack ||
            path->offset != TILE_FROM_XY(curX, curY) ||
            path->dest   != TILE_FROM_XY(newX, newY)) {
            #ifdef DEBUG_BUILD
            if (TILE_FROM_XY(newX, newY) < 0 || TILE_FROM_XY(newX, newY) >= environment.width * environment.height) errorSI("dest@searchPathfindingPath-2", TILE_FROM_XY(newX, newY));
            #endif
            queuePathfindingPathSearch(unit_nr, curX, curY, attack, shoot_range, newX, newY, priority); // add to queue for searching new path
        }
        return UM_MOVE_HOLD_INITIAL;
    }

    enum UnitMove move;
    
    if (path->size==0)
        move = UM_NONE;  // size 0 means stop there
    else
        move = obtainNextMove(path, TILE_FROM_XY(curX, curY));
    
    if (move == UM_NONE) {
        removePathfindingPath(unit_nr);
        return move;
    }
    
    #ifdef DEBUG_BUILD
    if (move != UM_MOVE_HOLD && (getNextTile(curX, curY, move) < 0 || getNextTile(curX, curY, move) >= environment.width * environment.height)) errorSI("nextTile@searchPathfindingPath", getNextTile(curX, curY, move));
    #endif
    if (move == UM_MOVE_HOLD || !freeToMoveUnitViaTile(unit + unit_nr, getNextTile(curX, curY, move)) || isTileBlockedByOtherUnit(unit + unit_nr, getNextTile(curX, curY, move))) {
        // requeue pathfinding but with cur_tile as starting point
        queuePathfindingPathSearch(unit_nr, curX, curY, attack, shoot_range, newX, newY, true);
        return UM_MOVE_HOLD;
    }
    
    return move;
}

inline enum UnitMove getPathfindingPath(int unit_nr, int curX, int curY, int newX, int newY) {
    bool priority = (unit[unit_nr].side == FRIENDLY);
    return searchPathfindingPath(unit_nr, curX, curY, false, 0, newX, newY, priority);
}

inline enum UnitMove getPathfindingAttackPath(int unit_nr, int curX, int curY, int shoot_range, int newX, int newY) {
    bool priority = (unit[unit_nr].side == FRIENDLY);
    return searchPathfindingPath(unit_nr, curX, curY, true, shoot_range, newX, newY, priority);
}

// should be called when: a unit stops moving but hasn't reached its destination, i.e. the unit is destroyed, or guards
// TODO: the latter should still be done, but as there is currently enough memory reserved for all units' paths, I haven't yet 
void removePathfindingPath(int unit_nr) {
    int i;
    
    for (i=0; i<MAX_PATHFINDING_PATHS; i++) {
        if (pathfindingPath[i].unit_nr == unit_nr) {
            pathfindingPath[i].unit_nr = -1;
            if (astarProcessingPath == &pathfindingPath[i]) {
                Astar_invalidateSearch();
                astarProcessingPath = 0;
            }
        }
    }
}

void setDestinationStructureTraversability(int structureNr, enum Traversability t) {
    int i,j;
    int x = structure[structureNr].x;
    int y = structure[structureNr].y;
    int w = structureInfo[structure[structureNr].info].width;
    int h = structureInfo[structure[structureNr].info].height;
    
    for (i=0; i<h; i++) {
        for (j=0; j<w; j++) {
            environment.layout[TILE_FROM_XY(x+j, y+i)].traversability = t;
        }
    }
}

int tileDistance(int tile1, int tile2) {
    int diffX = abs(X_FROM_TILE(tile1) - X_FROM_TILE(tile2));
    int diffY = abs(Y_FROM_TILE(tile1) - Y_FROM_TILE(tile2));
    int diagonalTiles = min(diffX, diffY);
    return (diffX - diagonalTiles) + (diffY - diagonalTiles);
}

int allowDestinationStructureTraversability (unsigned int goal, struct UnitInfo *cur_unitinfo)  {
    #ifdef DEBUG_BUILD
    if (goal < 0 || goal >= environment.width * environment.height) errorSI("tile@allowDestinationStructureTraversability", goal);
    #endif
    
    // check whether traversability of destination structure should temporarily be set to traversable for special cases
    int goalStructureNr = environment.layout[goal].contains_structure;
    if (goalStructureNr < -1) {
        #ifdef DEBUG_BUILD
        if ((goal + goalStructureNr + 1) < 0 || (goal + goalStructureNr + 1) >= environment.width * environment.height) errorSI("goalStructureNr@allowDestinationStructureTraversability", goalStructureNr);
        #endif
        goalStructureNr = environment.layout[goal + (goalStructureNr + 1)].contains_structure;
    }
    if (goalStructureNr >= MAX_DIFFERENT_FACTIONS) {
        if ((cur_unitinfo->can_collect_ore && structureInfo[structure[goalStructureNr].info].can_extract_ore) ||
            (cur_unitinfo->type != UT_FOOT && structureInfo[structure[goalStructureNr].info].can_repair_unit)) {
             setDestinationStructureTraversability(goalStructureNr, TRAVERSABLE);
        } else
            goalStructureNr = -1;
    } else
    goalStructureNr = -1;
    
    return goalStructureNr;
}

void disallowDestinationStructureTraversability (int StructureNr) {
    if (StructureNr!=-1)
        setDestinationStructureTraversability(StructureNr, UNTRAVERSABLE);
}

void doPathfindingLogic() {
    struct Unit *cur_unit=0;
    int i,goalStructureNr;
    int maxQueuedTime = 0;

    startProfilingFunction("doPathfindingLogic");
    
    // if there's a current unit in process, check if it still exists
    if (astarProcessingPath && astarProcessingPath->unit_nr==-1) {
        // unit is no longer alive, kill any running pathsearch
        Astar_invalidateSearch();
        astarProcessingPath = 0;
    }

    // diminish delay timers for queued paths until they are ready to initiate a new search
    for (i=0; i<MAX_PATHFINDING_PATHS; i++) {
        if (pathfindingPath[i].queue >= PQ_QUEUED) {
            if (pathfindingPath[i].queue == PQ_QUEUED)
              pathfindingPath[i].queue=PQ_NORMALPRIORITY;
            else
              pathfindingPath[i].queue--;
        }
        if (pathfindingPath[i].unit_nr != -1 && pathfindingPath[i].queue != PQ_UNQUEUED) {
            pathfindingPath[i].queuedTime++;
            if (pathfindingPath[i].queuedTime > maxQueuedTime) {
                maxQueuedTime = pathfindingPath[i].queuedTime;
            }
        }
    }
    
    maxGameticksSearch = MAX_GAMETICKS_IN_FRAME;
    while (getGameticks() > maxGameticksSearch) {
        maxGameticksSearch += GAMETICKS_PER_FRAME;
        addProfilingInformationInt("pathfinding maxGameticksSearch increased (1).", maxGameticksSearch);
    }
    // Using up an additional frame is not to happen every cycle. Hence the additionalFrameAllowedDelay.
    if (additionalFrameAllowedDelay > 0)
        additionalFrameAllowedDelay--;
    if (maxQueuedTime > MAX_QUEUED_TIME && additionalFrameAllowedDelay == 0) {
        maxGameticksSearch += GAMETICKS_PER_FRAME;
        addProfilingInformationInt("pathfinding maxGameticksSearch increased (2).", maxGameticksSearch);
        additionalFrameAllowedDelay = ADDITIONAL_FRAME_ALLOWED_DELAY;
    }
    
    while (canContinueSearch_callback()) {
        // check if we have a current unit
        if (astarProcessingPath) {
            if (Astar_getStatus()==AS_INCOMPLETE) {
                // an incompleted search requires continuation. This is easy.
                cur_unit = unit + astarProcessingPath->unit_nr;
                #ifdef DEBUG_BUILD
                if (astarProcessingPath->dest < 0 || astarProcessingPath->dest >= environment.width * environment.height) errorSI("AS_INCOMPLETE-path->dest@doPathfindingLogic", astarProcessingPath->dest);
                #endif
                goalStructureNr=allowDestinationStructureTraversability(astarProcessingPath->dest, unitInfo + cur_unit->info);
                Astar_continueSearch();
                disallowDestinationStructureTraversability(goalStructureNr);
            } else if (Astar_getStatus()==AS_FAILED) {
                // the previous search failed, we need to run a search to the closed
                // check if we're already on closest tile
                if (Astar_closest()==astarProcessingPath->offset) {
                    
                    // we're already on the closest tile, check if we're close enough to dest to settle definately
                    if (Astar_heuristicDistance(astarProcessingPath->offset,astarProcessingPath->dest)<THRESHHOLD_SETTLE_DISTANCE) {
                      astarProcessingPath->queue = PQ_UNQUEUED;
                      astarProcessingPath->size = 0;
                    } else {
                      astarProcessingPath->queue = PQ_QUEUED + ((PATHFINDING_REQUEUE_TIMER*FPS)*2)/getGameSpeed();
                    }
                
                    // set free for next one
                    Astar_invalidateSearch();
                    astarProcessingPath=0;
                } else {
                    // check if there's enough time to start
                    if (!canContinueSearch_callback()) {
                        stopProfilingFunction();
                        return;
                    }
                    // start!
                    cur_unit = unit + astarProcessingPath->unit_nr;
                    #ifdef DEBUG_BUILD
                    if (astarProcessingPath->dest < 0 || astarProcessingPath->dest >= environment.width * environment.height) errorSI("AS_FAILED-path->dest@doPathfindingLogic", astarProcessingPath->dest);
                    #endif
                    goalStructureNr=allowDestinationStructureTraversability(astarProcessingPath->dest, unitInfo + cur_unit->info);
                    Astar_startSearch (astarProcessingPath->offset, Astar_closest(),
                                       cur_unit->unit_positioned, false, 0, 0,
                                       cur_unit, unitInfo + cur_unit->info);
                    disallowDestinationStructureTraversability(goalStructureNr);
                }
            } else if (Astar_getStatus() == AS_COMPLETE) {
                // store first PATHBUFFERLEN turning points ('elbows') as the path
                astarProcessingPath->size = Astar_loadPath(astarProcessingPath->tile);
                astarProcessingPath->queue = PQ_UNQUEUED;
                astarProcessingPath->offset = 0;
                // set free for next one
                Astar_invalidateSearch();
                astarProcessingPath=0;
            } else {  // Astar_getStatus() == AS_INITIALIZED
                // start a new search, if there's enough time
                if (!canContinueSearch_callback()) {
                    stopProfilingFunction();
                    return;
                }
                // start!
                cur_unit = unit + astarProcessingPath->unit_nr;
                #ifdef DEBUG_BUILD
                if (astarProcessingPath->dest < 0 || astarProcessingPath->dest >= environment.width * environment.height) errorSI("AS_INITIALIZED-path->dest@doPathfindingLogic", astarProcessingPath->dest);
                #endif
                goalStructureNr=allowDestinationStructureTraversability(astarProcessingPath->dest, unitInfo + cur_unit->info);
                Astar_startSearch (astarProcessingPath->offset, astarProcessingPath->dest,
                                   cur_unit->unit_positioned, 
                                   astarProcessingPath->attack, astarProcessingPath->size,
                                   (Astar_heuristicDistance(astarProcessingPath->offset,astarProcessingPath->dest)<THRESHHOLD_DANCE_DISTANCE)?Astar_heuristicDistance(astarProcessingPath->offset,astarProcessingPath->dest)*3/2:0,
                                   cur_unit, unitInfo + cur_unit->info);
                disallowDestinationStructureTraversability(goalStructureNr);
            }
        } else {
            // no current unit selected
            // get a new unit from the queue, if any
            Astar_invalidateSearch(); // make sure we start
            astarProcessingPath = getQueuedPathfindingPathSearch();
            // if queue is empty, return
            if (!astarProcessingPath) {
                stopProfilingFunction();
                return;
            }
        }
    } // end while
    stopProfilingFunction();
}


int getPathFindingsSaveSize(void) {
    return sizeof(pathfindingPath);
}

int getPathFindingsSaveData(void *dest, int max_size) {
    int size=sizeof(pathfindingPath);
    
    if (size>max_size)
        return(0);
        
    memcpy (dest,&pathfindingPath,size);
    return (size);
}

inline struct Path *getPathFindings() {
    return pathfindingPath;
}

#endif
