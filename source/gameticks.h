// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _GAMETICKS_H_
#define _GAMETICKS_H_

#define GAMETICKS_PER_FRAME 560190 /* approximately, according to theory. tests on duration between 2 consecutive VBlanks confirms this: 560128, 560384 ticks */

void stopGameticks();

// returns 0 if the number of gameticks could not be measured accurately.
unsigned getGameticks();

void startGameticksPrecision(unsigned division);
void startGameticks(); // uses DIV64 precision by default, as to be able to count over 7 frames

#endif
