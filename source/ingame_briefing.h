// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _INGAME_BRIEFING_H_
#define _INGAME_BRIEFING_H_

enum IngameBriefingState { IBS_INACTIVE, IBS_QUEUED, IBS_ACTIVE };

enum IngameBriefingState getIngameBriefingState();

void doIngameBriefingLogic();
void drawIngameBriefing();

void startIngameBriefing(char *animation);
void startIngameBriefingWithDelay(char *animation, int delay); // delay measured in frames

void initIngameBriefing();

/* // cannot pause (and therefore not save) when an ingame briefing is playing
struct IngameBriefing *getIngameBriefing();
int getIngameBriefingSaveSize(void);
int getIngameBriefingSaveData(void *dest, int max_size);
*/

#endif
