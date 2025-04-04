// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _RUMBLE_H_
#define _RUMBLE_H_

void addRumble(int level, int duration);
void stopRumble();
void doRumbleLogic();

void delayNewRumble();
void executeWaitingRumble();

#endif
