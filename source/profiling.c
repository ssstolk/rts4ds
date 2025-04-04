// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "profiling.h"

#ifdef PROFILING_ENABLED

#define FUNCTION_START_OUTPUT_ENABLED

#include <nds.h>
#include <string.h>
#include <stdio.h>
#include "gameticks.h"
#include "debug.h"

struct ProfilingTimer {
    char function_name[32];
/*  u16 vcount_self;
    u16 vcount_added;*/
    unsigned ticks_start;
};

struct ProfilingTimer profilingTimer[MAX_PROFILER_DEPTH];
int profilerDepth = -1;
FILE *profilingFP;
/*bool profilerBusy = 1;*/

#endif



inline void startProfilingFunction(char *name) {
    #ifdef PROFILING_ENABLED
    profilerDepth++;
    struct ProfilingTimer *current = &profilingTimer[profilerDepth];
    strncpy(current->function_name, name, 32-1);
    current->ticks_start = getGameticks();
    #ifdef FUNCTION_START_OUTPUT_ENABLED
    fprintf(profilingFP, "%i. ", profilerDepth);
    fprintf(profilingFP, current->function_name);
    fprintf(profilingFP, "\n");
    #endif
    #endif
}

inline void stopProfilingFunction() {    
    #ifdef PROFILING_ENABLED
    struct ProfilingTimer *current = &profilingTimer[profilerDepth];
    unsigned gameticks = getGameticks();
    
    fprintf(profilingFP, "%i. ", profilerDepth);
    fprintf(profilingFP, current->function_name);
    if (gameticks == 0 || current->ticks_start == 0)
        fprintf(profilingFP, " || ticks: unknown\n");
    else {
        fprintf(profilingFP, " || ticks: %u\n", gameticks - current->ticks_start);
        
        if (gameticks < current->ticks_start) {
            fprintf(profilingFP, "-- end of file\n\n");
            fclose(profilingFP);
            errorSI("Currently measured gameticks at the end of a function was smaller than the gameticks measured at the start of the function. See profiling log. Gameticks:", getGameticks());
        }
    }
    
//    int i;
//    if (gameticks - current->ticks_start > 1000000) {
//        if (!strncmp(current->function_name, "doOverlayLogic", 14)) {
//            char gameticks_string[8*sizeof(unsigned)+1];
//            gameticks_string[8*sizeof(unsigned)] = 0;
//            for (i=0; i<8*sizeof(unsigned); i++) {
//                gameticks_string[(8*sizeof(unsigned)-1)-i] = (gameticks & BIT(i)) ? '1' : '0';
//            }
//            
//            char start_string[8*sizeof(unsigned)+1];
//            start_string[8*sizeof(unsigned)] = 0;
//            for (i=0; i<8*sizeof(unsigned); i++) {
//                start_string[(8*sizeof(unsigned)-1)-i] = (current->ticks_start & BIT(i)) ? '1' : '0';
//            }
//            
//            fprintf(profilingFP, "ticks_start was %u or b%s\n", current->ticks_start, start_string);
//            fprintf(profilingFP, "gameticks   was %u or b%s\n", gameticks, gameticks_string);
//        }
//    }
    
    profilerDepth--;
    #endif
}

inline void addProfilingInformation(char *info) {
    #ifdef PROFILING_ENABLED
    fprintf(profilingFP, "--- %s\n", info);
    #endif
}

inline void addProfilingInformationInt(char *info, int value) {
    #ifdef PROFILING_ENABLED
    fprintf(profilingFP, "--- %s [value: %i]\n", info, value);
    #endif
}

inline void startProfiling() {
    #ifdef PROFILING_ENABLED
    profilingFP = fopen(PROFILER_FILEPATH, "wb");
    if (!profilingFP)
        error("Could not create profiling file", "");
/*  profilerBusy = 0;*/
    #endif
}

inline void stopProfiling() {
    #ifdef PROFILING_ENABLED
/*  profilerBusy = 1;*/
    // ignoring all functions which are still being profiled at this point
    fprintf(profilingFP, "-- end of file\n\n");
    fclose(profilingFP);
    error("Stopped profiling", "");
    #endif
}
