// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _OBJECTIVES_H_
#define _OBJECTIVES_H_

enum ObjectivesState { OBJECTIVES_INCOMPLETE, OBJECTIVES_COMPLETED, OBJECTIVES_FAILED };

enum ObjectivesState getObjectivesState();
void doObjectivesLogic();
void initObjectives();

struct EntityObjectives *getStructureObjectives();
struct EntityObjectives *getUnitObjectives();
struct ResourceObjectives *getResourceObjectives();
int getStructureObjectivesSaveSize(void);
int getUnitObjectivesSaveSize(void);
int getResourceObjectivesSaveSize(void);
int getStructureObjectivesSaveData(void *dest, int max_size);
int getUnitObjectivesSaveData(void *dest, int max_size);
int getResourceObjectivesSaveData(void *dest, int max_size);

#endif
