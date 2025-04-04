// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "savegame.h"

#include <time.h>

#include "game.h"
#include "radar.h"
#include "settings.h"
#include "factions.h"

#include "environment.h"
#include "overlay.h"
#include "structures.h"
#include "units.h"
#include "projectiles.h"
#include "explosions.h"
#include "ai.h"
#include "objectives.h"
#include "timedtriggers.h"
#include "view.h"
#include "pathfinding.h"
//#include "ingame_briefing.h"

#include "info.h"
#include "view.h"
#include "shared.h"

#include "playscreen.h"
#include "infoscreen.h"
#include "music.h"

#define TEMPAREA_SIZE  (64*64*20+12)

enum SaveGameBlockHeaders { 
    SGBH_ENVIRONMENT,
    SGBH_OVERLAY,
    SGBH_STRUCTURE,
    SGBH_REBUILDQUEUE,
    SGBH_UNITS, SGBH_UNITSREINFORCEMENTS,
    SGBH_PROJECTILES,
    SGBH_EXPLOSIONS, SGBH_TANKSHOTS,
    SGBH_TEAMAI,
    SGBH_TIMEDTRIGGERS,
    SGBH_STRUCTUREOBJECTIVES,SGBH_UNITOBJECTIVES,SGBH_RESOURCEOBJECTIVES,
//    SGBH_INGAMEBRIEFING,               // cannot pause (and therefore not save) when an ingame briefing is playing
    SGBH_PATHFINDINGS,
    /*  add other blocks here  */
    SGBH_STATS,
    SGBH_NUMBLOCKS
};

// returns 0 in case of FAILURE
int saveGameLoadHeader(struct SavegameHeader *header, int slot) {
    FILE *fp;
    int res;
    
    if (slot < 0) slot = 0;
    if (slot > 2) slot = 2;
    fp=openSaveFile(slot, "rb");
    
    if (!fp)
        return 0;
    
    res=fread(header,sizeof(struct SavegameHeader),1,fp);
    fclose(fp);
    if (res && !strncmp((char*)header, "EMPTY", strlen("EMPTY"))) {
        // an entire SaveGameHeader was indeed read in, but the savegame is empty
        return 0;
    }
    return(res);
}

void dataSaveGameInfo (struct SavegameHeader_info *data) {
    data->faction=getFaction(FRIENDLY);   // index of factionInfo[MAX_DIFFERENT_FACTIONS].name
    data->level=getLevel();               // 0 to ?
    data->map=getRegion();                // 0 to ?
    
    {
        time_t unixTime = time(NULL);
        struct tm* timeStruct = gmtime((const time_t *)&unixTime);
        
        data->date_year   = (timeStruct->tm_year + 1900) - 2000;  // 0 to 255 (2000 to 2255)
        data->date_month  = timeStruct->tm_mon;   // 1 to 12
        data->date_day    = timeStruct->tm_mday;  // 1 to 31
        data->date_hour   = timeStruct->tm_hour;  // 0 to 23
        data->date_minute = timeStruct->tm_min;   // 0 to 59
    }
    data->matchtime=getMatchTime();
    data->view_X=getViewCurrentX();
    data->view_Y=getViewCurrentY();
    data->gamespeed=getGameSpeed();
    data->level_loaded_via_level_select=isLevelLoadedViaLevelSelect();
}

// returns 0 in case of FAILURE
int saveGameWriteBlock(int head, void *temp, FILE *fp) {
    int size=0;
    int res;
    switch (head) {
        case SGBH_ENVIRONMENT:
            size=getEnvironmentSaveData(temp,TEMPAREA_SIZE);
            break;
        case SGBH_OVERLAY:
            size=getOverlaySaveData(temp,TEMPAREA_SIZE);
            break;
        case SGBH_STRUCTURE:
            size=getStructuresSaveData(temp,TEMPAREA_SIZE);
            break;
        case SGBH_REBUILDQUEUE:
            size=getRebuildQueueSaveData(temp,TEMPAREA_SIZE);
            break;
        case SGBH_UNITS:
            size=getUnitsSaveData(temp,TEMPAREA_SIZE);
            break;
        case SGBH_UNITSREINFORCEMENTS:
            size=getUnitsReinforcementsSaveData(temp,TEMPAREA_SIZE);
            break;
        case SGBH_PROJECTILES:
            size=getProjectilesSaveData(temp,TEMPAREA_SIZE);
            break;
        case SGBH_EXPLOSIONS:
            size=getExplosionsSaveData(temp,TEMPAREA_SIZE);
            break;
        case SGBH_TANKSHOTS:
            size=getTankShotsSaveData(temp,TEMPAREA_SIZE);
            break;
        case SGBH_TEAMAI:    
            size=getTeamAISaveData(temp,TEMPAREA_SIZE);
            break;
        case SGBH_TIMEDTRIGGERS:
            size=getTimedTriggersSaveData(temp,TEMPAREA_SIZE);
            break;
        case SGBH_STRUCTUREOBJECTIVES:
            size=getStructureObjectivesSaveData(temp,TEMPAREA_SIZE);
            break;
        case SGBH_UNITOBJECTIVES:
            size=getUnitObjectivesSaveData(temp,TEMPAREA_SIZE);
            break;
        case SGBH_RESOURCEOBJECTIVES:
            size=getResourceObjectivesSaveData(temp,TEMPAREA_SIZE);
            break;
        case SGBH_PATHFINDINGS:
            size=getPathFindingsSaveData(temp,TEMPAREA_SIZE);
            break;
        
        // add others here...
        
        case SGBH_STATS:
            size=getStatsSaveData(temp,TEMPAREA_SIZE);
            break;
    }
    
    if (size==0)
        return (0);   // error!

    // write header and size of the block
    res=fwrite(&head,sizeof(int),1,fp);
    if (res==0)
        return(0);
    res=fwrite(&size,sizeof(int),1,fp);
    if (res==0)
        return(0);
    // save the block
    res=fwrite(temp,size,1,fp);
    return(res);
}

// returns 0 in case of FAILURE
int saveGame(int slot) {
    FILE *fp;
    int res,i;
    struct SavegameHeader *header;
    void *temparea;
    temparea=malloc(TEMPAREA_SIZE);
    
    // check if malloc failed
    if (temparea==NULL) {
        #ifdef DEBUG_BUILD
//        error("can't allocate temp area", "");   // TROUBLE!!!!
        #endif
        return 0;
    }
    
    if (slot < 0) slot = 0;
    if (slot > 2) slot = 2;
    fp=openSaveFile(slot, "wb");

    if (fp) {
        
        // header
        header=temparea;
        dataSaveGameRadar(header->radar.image);
        dataSaveGameInfo(&header->info);
        res=fwrite(header,sizeof(struct SavegameHeader),1,fp);
        if (res==0) {
            free(temparea);
            #ifdef DEBUG_BUILD
//            error("can't correctly save savegame", "");   // BETA!
            #endif
            return(0);
        }
        
        for (i=SGBH_ENVIRONMENT;i<SGBH_NUMBLOCKS;i++) {
            if (!saveGameWriteBlock(i,temparea, fp)) {
                res=0;
                break;
            }
        }
        
        free(temparea);
        fclose(fp);
        
        //  if res==0 then SAVE ERROR!
        if (res==0) {
            #ifdef DEBUG_BUILD
//            error("can't correctly save savegame", "");   // BETA!
            #endif
            return(0);
        }
       
        return(1);
    }
    
    free(temparea);
    return(0);
}


void clearSaveGame(int slot) {
    FILE *fp;
    
    if (slot < 0) slot = 0;
    if (slot > 2) slot = 2;
    fp=openSaveFile(slot, "wb");
    
    if (fp)
        fclose(fp);
}


// returns 0 in case of FAILURE
int saveGameReadBlock(int head, int blocksize, FILE *fp) {
    int size,res;
    void *dest;
    switch (head) {
        case SGBH_ENVIRONMENT:
            size=getEnvironmentSaveSize();
            dest=&environment;
            break;
        case SGBH_OVERLAY:
            size=getOverlaySaveSize();
            dest=getOverlay();
            break;
        case SGBH_STRUCTURE:
            size=getStructuresSaveSize();
            dest=structure;
            break;        
        case SGBH_REBUILDQUEUE:
            size=getRebuildQueueSaveSize();
            dest=getRebuildQueue();
            break;
        case SGBH_UNITS:
            size=getUnitsSaveSize();
            dest=unit;
            break;
        case SGBH_UNITSREINFORCEMENTS:
            size=getUnitsReinforcementsSaveSize();
            dest=getUnitsReinforcements();
            break;
        case SGBH_PROJECTILES:
            size=getProjectilesSaveSize();
            dest=projectile;
            break;
        case SGBH_EXPLOSIONS:
            size=getExplosionsSaveSize();
            dest=explosion;
            break;
        case SGBH_TANKSHOTS:
            size=getTankShotsSaveSize();
            dest=getTankShots();
            break;
        case SGBH_TEAMAI:
            size=getTeamAISaveSize();
            dest=getTeamAI();
            break;
        case SGBH_TIMEDTRIGGERS:
            size=getTimedTriggersSaveSize();
            dest=getTimedTriggers();
            break;
        case SGBH_STRUCTUREOBJECTIVES:
            size=getStructureObjectivesSaveSize();
            dest=getStructureObjectives();
            break;
        case SGBH_UNITOBJECTIVES:
            size=getUnitObjectivesSaveSize();
            dest=getUnitObjectives();
            break;
        case SGBH_RESOURCEOBJECTIVES:
            size=getResourceObjectivesSaveSize();
            dest=getResourceObjectives();
            break;
        case SGBH_PATHFINDINGS:
            size=getPathFindingsSaveSize();
            dest=getPathFindings();
            break;    
        
        // add others here...
        case SGBH_STATS:
            size=getStatsSaveSize();
            dest=getStats();
            break;
        default:
            // no other header is expected, it means something has gone wrong...
            return 0;
            break;
    }
    
    // if size is not as expected, it means something has gone wrong...
    if (size!=blocksize)
        return 0;

    // load the block
    res=fread(dest,size,1,fp);
    return res;      
}

// returns 0 in case of FAILURE
int loadGame(int slot) {
    FILE *fp;
    int res,i,j,size;
    struct SavegameHeader *header;
    
    if (slot < 0) slot = 0;
    if (slot > 2) slot = 2;
    fp=openSaveFile(slot, "rb");

    if (!fp)
        return 0;
    
    header=malloc(sizeof(struct SavegameHeader));
    
    // check if malloc failed
    if (header==NULL) {
        #ifdef DEBUG_BUILD
//        error("can't allocate temp area", "");   // TROUBLE!!!!
        #endif
        return 0;
    }
    
    // read header
    res=fread(header,sizeof(struct SavegameHeader),1,fp);
    if (res==0) {
        free(header);
        #ifdef DEBUG_BUILD
//        error("can't correctly load savegame header", "");      // BETA!
        #endif
        return(0);
    }
    
    // set current faction, level and region, needed for inits
    setFaction(FRIENDLY,header->info.faction);
    setLevel(header->info.level);
    setRegion(header->info.map);
    
    // set the variable which determines if a won or lost game should return player to the Level Select menu
    setLevelLoadedViaLevelSelect(header->info.level_loaded_via_level_select);
    
    // inits (these were copied from game.c)
    initView(HORIZONTAL, 0, 0);
    initPlayScreen();
    initInfoScreen();
    initTimedtriggers();
    initObjectives();
    initMusicWithScenario();
    
    setMatchTime(header->info.matchtime);
    initView(HORIZONTAL, header->info.view_X, header->info.view_Y);
    setGameSpeed(header->info.gamespeed);
    
    // header no longer needed
    free(header);

    
    // read blocks from file (copy memory from snapshot)
    for (i=SGBH_ENVIRONMENT;i<SGBH_NUMBLOCKS;i++) {
        // read block header ID
        res=fread(&j,sizeof(int),1,fp);
        // enforce order: this should be == i
        if ((res==0) || (j!=i))  {
            #ifdef DEBUG_BUILD
//            error("can't correctly load savegame (block order mismatch)", "");  // BETA!
            #endif
            return(0);
        }
        // read block size
        res=fread(&size,sizeof(int),1,fp);
        if (res==0) {
            #ifdef DEBUG_BUILD
//            error("can't correctly load savegame (incomplete file)", "");   // BETA!
            #endif
            return(0);
        }
        // finally read block
        if (!saveGameReadBlock(i,size, fp)) {
            #ifdef DEBUG_BUILD
//            error("can't correctly load savegame (block size mismatch)", "");   // BETA!
            #endif
            return(0);
        }
    }
    fclose(fp);
    
    createBarStructures(); // make sure to recreate this.
    
    return 1;   // savegame loaded successfully!
}
