// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _VBLANKCOUNT_H_
#define _VBLANKCOUNT_H_

void handleVBlankInterrupt();

void resetVBlankCount();
int getVBlankCount();

#endif
