// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _PATHFINDING_H_
#define _PATHFINDING_H_

//#define REMOVE_ASTAR_PATHFINDING
#ifndef REMOVE_ASTAR_PATHFINDING

#include "astar/astar.h"

enum PathfindingQueueStatus { PQ_UNQUEUED = -1,
                              PQ_HIGHPRIORITY = 0, PQ_NORMALPRIORITY = 1, 
                              PQ_LOWPRIORITY = 2 , PQ_QUEUED = 3 };
// ( PQ_LOWPRIORITY still unused )

struct Path {
    int unit_nr;                           // unit_nr.  -1 when not in use.
    enum PathfindingQueueStatus queue;     // whether the path is queued for a search.  if !PQ_UNQUEUED: 'size' indicates shoot_range and 'offset' indicates current tile
    unsigned short int queuedTime; // the duration (in logic frames) the path search has been queued.
//    bool queued;                // whether the path is queued for a search.  if true: 'size' indicates shoot_range and 'offset' indicates current tile
    unsigned short int dest;    // final destination tile (possibly to attack) searched for.
    bool attack;                // whether the path was searched for in order to attack or to move to
    unsigned short int tile[PATHBUFFERLEN];
    unsigned short int size;    // the number of nodes valid in 'tile'.  is > PATHBUFFERLEN when 'tile' could not store all nodes.
    unsigned short int offset;  // offset within 'tile' for the current to-move-to tile on a straight line
};

enum UnitMove getPathfindingPath(int unit_nr, int curX, int curY, int newX, int newY);
enum UnitMove getPathfindingAttackPath(int unit_nr, int curX, int curY, int shoot_range, int newX, int newY);

// should be called when: a scenario is first loaded up (for all units)
//                        a unit is destroyed
//                        a unit bumps too often (apparently in need of a new/better path)
void removePathfindingPath(int unit_nr);

void doPathfindingLogic();

void initPathfinding();
void initPathfindingWithScenario();

int getPathFindingsSaveSize();
int getPathFindingsSaveData(void *dest, int max_size);
struct Path *getPathFindings();

#endif
#endif
