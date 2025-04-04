// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _TIMEDTRIGGERS_H_
#define _TIMEDTRIGGERS_H_

void doTimedtriggersLogic();
void initTimedtriggers();

struct Timer *getTimedTriggers();
int getTimedTriggersSaveSize(void);
int getTimedTriggersSaveData(void *dest, int max_size);

#endif
