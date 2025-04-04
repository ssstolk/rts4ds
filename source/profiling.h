// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _PROFILING_H_
#define _PROFILING_H_

#include "shared.h"
#ifdef DEBUG_BUILD
//#define PROFILING_ENABLED
#endif

#define MAX_PROFILER_DEPTH  100
#define PROFILER_FILEPATH   "/rts4ds_profiling.txt"

void startProfilingFunction(char *name);
void stopProfilingFunction();

void addProfilingInformation(char *info);
void addProfilingInformationInt(char *info, int value);

void startProfiling();
void stopProfiling();

#endif
