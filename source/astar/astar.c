
//
// Astar - an A* implementation - by sverx
//

#include <nds.h>
#include "astar.h"
#ifndef REMOVE_ASTAR_PATHFINDING

extern void memset32(u32 *dest, u32 word, u32 size);

#define TOTALDIRECTIONS    8

// the chunk for initializing the hash table when needed
#define HASHT_CHUNK         256

// max REAL distance. This should be about 2111*HORIZVERT_REAL_DISTANCE according to my
// rehearsals, but here needs to be multiple of 32 so, well, 6400 should be ok.
#define MAXDISTANCE        6400

// the node set (openSet)
__attribute__((aligned(32))) openSetNode openSet[MAPSIZE]; // 4096 * sizeof(openSetNode) = 32KB

// this array stores the status of the nodes (2 bit per status: 4 statuses per byte)
__attribute__((aligned(32))) u8 nodeBitmap[MAPSIZE/4];    // 1024 bytes

// Hash Table and ...
__attribute__((aligned(32))) unsigned short int hashT [MAXDISTANCE] ;     // 12800 bytes
// ...  separate chaining (simple list)
__attribute__((aligned(32))) unsigned short int hashT_next [MAPSIZE];     // 8192 bytes

// the other variables...
__attribute__((aligned(32))) 
// The minimum value we've got in the table
unsigned int hashT_min;
// the number of nodes in the openSet Hash
unsigned int hashT_count;
// the initialized part of the array in the openSet Hash
unsigned int hashT_initialized;
// the best node in the closedSet
unsigned int closedSet_best;
// the min distance in the closedSet
unsigned int closedSet_min;

// 'global' values (initialized @ 64x64)
unsigned int WidthMask = 0x3F;
unsigned int HeightShift = 6;
unsigned int MapWidth  = 64;
unsigned int MapHeight = 64;

// statics
enum Astar_Result A_status = AS_INITIALIZED;
unsigned int A_start;
unsigned int A_goal;
unsigned int A_attackPoint;  // the tile from where we can shoot the goal node:
enum Positioned A_start_heading;
bool A_attack_mode;
unsigned int A_max_attack_distance;
unsigned int A_cruise_range;

void *A_cur_unit;
void *A_cur_unitinfo;

// the function pointers:
unsigned int (*mapCanBeTraversed)(unsigned int, unsigned int, void* /* cur_unit */, void* /* cur_unitinfo*/) = 0;
unsigned int (*canHitTarget)(unsigned int, unsigned int, void* /* cur_unit */, void* /* cur_unitinfo*/) = 0;
unsigned int (*canContinueSearch)(void) = 0;

// these short arrays go in DTCM  (8*2 = 16 bytes)
DTCM_DATA s8 Dirs_x[TOTALDIRECTIONS]  = { 0, +1, +1, +1, 0, -1, -1, -1 };
DTCM_DATA s8 Dirs_y[TOTALDIRECTIONS]  = {-1, -1, 0, +1, +1, +1,  0, -1 };

// this is another short array that goes in DTCM  (2 bytes)
DTCM_DATA u8 RealDistances[2] = { HORIZVERT_REAL_DISTANCE, DIAGONAL_REAL_DISTANCE};

//
// functions
//

void reset(void) {
  // reset the node bitmap
  memset32((u32*)nodeBitmap, 0x00000000, sizeof(nodeBitmap));
  
  // reset the openset Hash Table (first chunk) to all 'NULLLINK'
  memset32((u32*)hashT, NULLLINK|(NULLLINK<<16), HASHT_CHUNK*sizeof(unsigned short int));

  // reset counters
  hashT_count = 0;
  hashT_min   = (MAXDISTANCE-1);
  hashT_initialized = HASHT_CHUNK;
  
  // reset closedSet var
  closedSet_min = (MAXDISTANCE-1);
}

// ****************************************************************************

// add a node to the openSet
// the node contains the f_score, and this is used to find the entry in the
// hash table. Here, there's a simple list (nodes with same f_value) and 
// we'll add in the beginning.
// We could also need to update the hashT_min value
void openSet_add(unsigned int new_node, unsigned int seed) {

  // check if we need to initialize some other chunks of the hash table
  while (seed>=hashT_initialized) {
    memset32((u32*)&hashT[hashT_initialized], NULLLINK|(NULLLINK<<16), HASHT_CHUNK*sizeof(unsigned short int));
    hashT_initialized+=HASHT_CHUNK;
  }
  
  // insert in the hash table list (as first item)
  hashT_next[new_node]=hashT[seed];
  hashT[seed]=new_node;
  
  // we've got 1 more item
  hashT_count++;
  
  // update hashT_min if needed
  if (seed<hashT_min)
    hashT_min=seed;
    
  // update the bitmap
  SET_NODE_STATUS(new_node, NS_OPENSET);
}

// ****************************************************************************

// retrieve the first node from the openSet, and removes it from the list.
unsigned int openSet_popFirst(void) {

  // openSet empty? Exit! Otherwise just decrease it
  if (hashT_count--==0)
    return (NULLLINK);
  
  // find the first NOT NULL list in the table (make sure we're actually pointing to a node)
  while (hashT[hashT_min]==NULLLINK) {
    hashT_min++;
  }

  unsigned int first=hashT[hashT_min];
  hashT[hashT_min]=hashT_next[first];
  
  return (first);
}

// ****************************************************************************

// remove a node from the openSet
void openSet_del(unsigned int node, unsigned int seed) {

  if (hashT[seed]==node) {
    hashT[seed]=hashT_next[node];
  } else {
    unsigned int prev = hashT[seed];
    seed=hashT_next[prev];
    while (seed!=node) {
      prev=seed;
      seed=hashT_next[seed];
    }
    hashT_next[prev]=hashT_next[seed];
  }
  
  // we've got 1 less item now
  hashT_count--;
}

// ****************************************************************************

// add a node to the closedSet
void closedSet_add(unsigned int node) {

  unsigned int h_score = openSet[node].f_score-openSet[node].g_score;

  if (h_score<closedSet_min) {
    closedSet_best=node;
    closedSet_min=h_score;
  }
    
  SET_NODE_STATUS(node, NS_CLOSEDSET);             
}


//
//  functions - distance, costs, moves
//


/*
// this calculates the time spent turning to the new direction
// 0=no turning required, n*TURNING_COST=how many turning required*cost
unsigned int TurningCost(int heading, int new_dir) {
  
  unsigned int r=abs(new_dir - heading);
  if (r<=4)
    return(r*TURNING_COST);
  else
    return ((8-r)*TURNING_COST);
}
*/

// this calculates the heuristic distance from node_a to node_b
// it's a 'modified' manhattan distance
// simple approach: doesn't consider initial heading and turning costs
unsigned int Astar_heuristicDistance (unsigned int node_a, unsigned int node_b) {

  int dx=abs((node_b & WidthMask)    - (node_a & WidthMask));
  int dy=abs((node_b >> HeightShift) - (node_a >> HeightShift));
  int dg=MIN(dx,dy) * DIAGONAL_HEUR_DISTANCE;
  int hv=abs(dx-dy) * HORIZVERT_HEUR_DISTANCE;
  return (dg+hv);
}

// this calculates the destination node of a 1-step move towards the choosen direction.
// returns NULLLINK if there's no node there (if we would fall out of the map)
unsigned int moveTo(unsigned int start, unsigned int dir) {

  unsigned int dx=(start & WidthMask) + Dirs_x[dir];
  if (dx>=MapWidth)
    return (NULLLINK);

  unsigned int dy=(start >> HeightShift) + Dirs_y[dir];
  if (dy>=MapHeight)
    return (NULLLINK);

  return ((dy << HeightShift) | dx);
  
}

//
//   configuration functions
//

// to be called ONCE before using Astar, to set map size
bool Astar_config (unsigned int width, unsigned int height) {

  // limit both values from 32 to 128
  if ((width<32) || (width>128))
    return (false);
    
  if ((height<32) || (height>128))
    return (false);
    
  // limit number of bits used to 12 (2^12=MAPSIZE)
  if ((width>64) && (height>32))
    return (false);
    
  if ((width>32) && (height>64))
    return (false);
    
  // values are valid
  MapWidth  = width;
  MapHeight = height;
    
  if (MapWidth<=32)
    // last 5 bit for width (upper bits are for height)
    HeightShift = 5;
  else if (MapWidth<=64)
    // last 6 bit for width (upper bits are for height)
    HeightShift = 6;
  else
    // last 7 bit for width (upper bits are for height)
    HeightShift = 7;

  // set mask for width (the lower bits)
  WidthMask = BIT(HeightShift)-1;
  
  // set status to AS_INITIALIZED, since no search has been yet performed
  A_status = AS_INITIALIZED;
  
  return (true);
}


// to be called ONCE before using Astar, to map function pointers to REAL functions
void Astar_callback (unsigned int (*mapCanBeTraversed_funct)(unsigned int, unsigned int, void*, void*),
                     unsigned int (*canHitTarget_funct)(unsigned int, unsigned int, void*, void*),
                     unsigned int (*canContinueSearch_funct)(void)) {

  mapCanBeTraversed = mapCanBeTraversed_funct;
  canHitTarget      = canHitTarget_funct;
  canContinueSearch = canContinueSearch_funct;

}


// 
//  The A* search algo. Based on the code found at http://en.wikipedia.org/wiki/A*#Pseudocode
//

// sets parameters for the path search and starts it
void Astar_startSearch (unsigned int start, unsigned int goal, enum Positioned start_heading, bool attack_mode, unsigned int max_attack_distance, unsigned int cruise_range, void *cur_unit, void *cur_unitinfo) {

  // reset the search
  reset();
  
  // copy all the params into statics
  A_start = start;
  A_goal = goal;
  A_start_heading = start_heading;
  A_attack_mode = attack_mode;
  A_max_attack_distance = max_attack_distance;
  A_cruise_range = cruise_range;
  A_cur_unit=cur_unit;
  A_cur_unitinfo=cur_unitinfo;
  
  // initialize the start node (f=g+h)
  openSet[start].f_score=Astar_heuristicDistance(A_start, A_goal);
  openSet[start].g_score=0;
  openSet[start].heading=A_start_heading;

  // add the start node to the openset
  openSet_add(A_start, openSet[start].f_score);
  
  A_status = AS_INCOMPLETE;
  Astar_continueSearch();

}

// go on with an already started path search
void Astar_continueSearch (void) {

  // make sure there's a search going on
  if (A_status != AS_INCOMPLETE)
    return;

  // pop out a node from the openSet
  unsigned int curr_node = openSet_popFirst();              
  
  // while openSet is not empty
  while (curr_node!=NULLLINK) {
  
    // check if we are in attack_mode and we could shoot the goal from the current tile
    // or if we are NOT in attack_mode and we reached the goal node
    if (A_attack_mode) {
      if ((openSet[curr_node].f_score-openSet[curr_node].g_score)<=A_max_attack_distance) {
        if (canHitTarget(curr_node, A_goal, A_cur_unit, A_cur_unitinfo)) {
          A_attackPoint=curr_node;
          A_status=AS_COMPLETE;
          return;
        }
      }
    } else {
      if (curr_node==A_goal) {
        A_status = AS_COMPLETE;
        return;
      }
    }
    
    // add curr_node to closedset
    closedSet_add(curr_node);

    // for each one of the 8 directions    
    unsigned int dir = openSet[curr_node].heading;
    do {
     
      // check every direction, keeping current heading as last one
      dir = (dir+1) % TOTALDIRECTIONS;
     
      // check if a node exists in this direction
      unsigned int new_node=moveTo(curr_node,dir);
      
      // if this new node doesn't exist (there's no node in this direction), just skip it
      if (new_node==NULLLINK)
        continue;
     
      // if this node is already in the closedSet just skip it
      unsigned int node_status = GET_NODE_STATUS(new_node);
      if (node_status==NS_CLOSEDSET)
        continue;
      
      // here we check if the node in this game can be traversed
      // if (!mapCanBeTraversed(new_node, tentative_g_score, A_cur_unit, A_cur_unitinfo))
      if (!mapCanBeTraversed(new_node, Astar_heuristicDistance(A_start,new_node), A_cur_unit, A_cur_unitinfo))
        continue;
        
      // distance from start to curr_node (g_score) and from curr_node to new_node
      unsigned int tentative_g_score = openSet[curr_node].g_score + RealDistances[dir & 0x01];
      //                               + TurningCost(openSet[curr_node].heading, dir);
      
      // check if 'cruise_range' (if in use) is enough to reach this new_node
      // if tentative_g_score>range then skip that
      if ((A_cruise_range>0) && (tentative_g_score>A_cruise_range))
        continue;
      
      // if node isn't yet in the openset OR needs to be updated then go on, otherwise leave
      if ((node_status!=NS_OPENSET) || (tentative_g_score < openSet[new_node].g_score)) ;
      else continue;
      
      // if we need to update it, remove it from the openSet (before changing the f_score!)
      if (node_status==NS_OPENSET)
        openSet_del(new_node, openSet[new_node].f_score);
     
      openSet[new_node].f_score=tentative_g_score+Astar_heuristicDistance(new_node,A_goal);
      openSet[new_node].g_score=tentative_g_score;
      openSet[new_node].heading=dir;
      openSet[new_node].came_from=curr_node;
        
      openSet_add(new_node, openSet[new_node].f_score);
      
    } while (dir!=openSet[curr_node].heading);   // end ['Directions']
    
    // check if it's not too 'late' to process another node
    if (!canContinueSearch()) {
      A_status = AS_INCOMPLETE;
      return;
    }
    
    // get next node from the openSet
    curr_node = openSet_popFirst();
    
  } // end [while openSet is not empty]
  
  // if we get here, it means the search failed
  A_status = AS_FAILED;
  return;
}

// invalidate any started path search
void Astar_invalidateSearch (void) {
  A_status = AS_INITIALIZED;
}

// returns current path search status
enum Astar_Result Astar_getStatus (void) {
  return (A_status);
}

// turns nodes into tiles
unsigned int Astar_tile (unsigned int node) {
  return ((node >> HeightShift)*MapWidth + (node & WidthMask));
}

unsigned int Astar_closest (void) {
  return (Astar_tile(closedSet_best));
}

unsigned int Astar_attackPoint (void) {
  return (Astar_tile(A_attackPoint));
}

unsigned int Astar_loadPath(unsigned short int *nodebuffer) {
  unsigned short int path[PATHBUFFERLEN];
  int index=PATHBUFFERLEN-1;
  int wrap=0;
  
  // we make sure a complete search has been performed
  if (A_status != AS_COMPLETE)
    return(0);
    
  // select where to build the backward 'elbows' list
  unsigned int goal_node = A_attack_mode?A_attackPoint:A_goal;

  // store the goal node as last path node
  path[index]=goal_node;
  index--;
  
  // go back in the path storing all the 'elbows'
  unsigned int curr=goal_node;
  while (curr!=A_start) {
    // go back searching the 'elbows'
    if (openSet[curr].heading!=openSet[openSet[curr].came_from].heading) {
      // avoid storing the start node
      if (openSet[curr].came_from!=A_start) {
        path[index]=openSet[curr].came_from;
        index--;
        if (index<0) {
          index=PATHBUFFERLEN-1;
          wrap++;
        }
      }
    }
    curr=openSet[curr].came_from;
  }
  
  // copy path
  int i=index+1;
  while (i<PATHBUFFERLEN) {
    *nodebuffer++=path[i++];
  }
  
  if (wrap) {
    int i=0;
    while (i<=index) {
      *nodebuffer++=path[i++];
    }
  }
  
  return (wrap * PATHBUFFERLEN + ((PATHBUFFERLEN-1) - index));
}


#endif
