// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "shared.h"
#include "profiling.h"

void breakpointSI(char *string, int nr);
void breakpoint(char *string1, char *string2);

void errorI4(int int1, int int2, int int3, int int4);
void errorSI(char *string, int nr);
void error(char *string1, char *string2);

#endif
