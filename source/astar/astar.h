#ifndef _ASTAR_H_
#define _ASTAR_H_
#ifndef REMOVE_ASTAR_PATHFINDING

//
// Astar - an A* implementation - by sverx
//

// length of the buffer to store path nodes
#define PATHBUFFERLEN   16

// 64x64 == 32x128 == 128x32 == 4096
#define MAPSIZE       4096

// to access node status
#define GET_NODE_STATUS(k)      ((nodeBitmap[(k)>>2]>>(((k)&0x03)<<1))&0x03)
#define SET_NODE_STATUS(k,v)     (nodeBitmap[(k)>>2]|=((v)<<(((k)&0x03)<<1)))

// node statuses (2 bits)
#define NS_NEW          0b00
#define NS_OPENSET      0b01
#define NS_UNDEFINED    0b10
#define NS_CLOSEDSET    0b11

#define MIN(a,b)           (((a)<(b))?(a):(b))

// values for the heuristic distance function.
// **** THESE SHOULD NEVER BE OVERESTIMATED for a shortest path exact search!  ****
// 1      * 3 = 3
// 1.4142 * 3 = 4.24

#define HORIZVERT_HEUR_DISTANCE  3
#define DIAGONAL_HEUR_DISTANCE   4

// values for the real distance function.
// **** THESE SHOULD NEVER BE UNDERESTIMATED for a shortest path exact search! ****

#define HORIZVERT_REAL_DISTANCE  3
#define DIAGONAL_REAL_DISTANCE   4

// the cost of turning 1/8 of a complete turn
// #define TURNING_COST             1

typedef struct openSetNode {
  unsigned short int f_score;    // total distance (real+estimated) [this is the sorting key]
  unsigned short int g_score;    // real distance from start
  
  unsigned short int heading;    // unit heading when entering this node
  unsigned short int came_from;  // previous node in the found path
} openSetNode;

#define NULLLINK                 0xFFFF

// this next line is taken from RTS4DS
//enum Positioned { UP, RIGHT_UP, RIGHT, RIGHT_DOWN, DOWN, LEFT_DOWN, LEFT, LEFT_UP };
#include "../shared.h"


// to be called ONCE before using Astar, to map function pointers to REAL functions
void Astar_callback (unsigned int (*mapCanBeTraversed_funct)(unsigned int, unsigned int, void*, void*),
                     unsigned int (*canHitTarget_funct)(unsigned int, unsigned int, void*, void*),
                     unsigned int (*canContinueSearch_funct)(void));

// to be called ONCE before using Astar, to set map size
bool Astar_config (unsigned int width, unsigned int height);

// this is what Astar_getStatus() will return:
enum Astar_Result { AS_INITIALIZED, AS_COMPLETE, AS_INCOMPLETE, AS_FAILED };
// AS_INITIALIZED : No path search run, yet -> Start a search at your needs
// AS_COMPLETE    : A path has been found -> You should then read it from nodes
// AS_INCOMPLETE  : A path hasn't been yet found, still there are chances -> You should continue searching
// AS_FAILED      : No path exists -> You can call Astar_closest() to find the closest node to goal you can reach from start

// start the path search itself
void Astar_startSearch (unsigned int start, unsigned int goal, enum Positioned start_heading, bool attack_mode, unsigned int max_attack_distance, unsigned int cruise_range, void *cur_unit, void *cur_unitinfo);
// start, goal         : start and goal tile, y and x separated on upper/lower bits
// start_heading       : 0 to 7, initial heading of the unit
// attack_mode         : TRUE if we just need to locate a spot close enough to attack the goal node
// max_attack_distance : The highest distance this unit can shoot (when attack mode is TRUE)
// cruise_range        : if >0, this limits the distance the unit can go
// cur_unit            : points to the current unit
// cur_unitinfo        : points to the current unit info

// go on with an already started path search
void Astar_continueSearch (void);

// invalidate any started path search
void Astar_invalidateSearch (void);

// get path search status
enum Astar_Result Astar_getStatus (void);

// call this when the path search fails. Returns x,y as a x + mapwidth*y single value.
unsigned int Astar_closest (void);

// turns nodes into tiles (helper)
unsigned int Astar_tile (unsigned int node);

// call this for finding the location from where it's possible to attack the goal node.
// Returns x,y as a x + mapwidth*y single value.
unsigned int Astar_attackPoint (void);

// call this for loading the first PATHBUFFERLEN nodes from start to goal
// nodebuffer is a pointer to a unsigned short int[PATHBUFFERLEN] array
unsigned int Astar_loadPath(unsigned short int *nodebuffer);

// added this so we can use the same heuristic outside astar sources
unsigned int Astar_heuristicDistance (unsigned int node_a, unsigned int node_b);

#endif
#endif
