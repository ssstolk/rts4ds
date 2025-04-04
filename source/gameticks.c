// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "gameticks.h"

#include <nds/timers.h>
#include <nds/interrupts.h>

unsigned gameticksDivision;


inline void stopGameticks() {
    TIMER2_CR = 0;
}

inline unsigned getGameticks() {
    unsigned ticks = TIMER2_DATA;
    ticks = (gameticksDivision == TIMER_DIV_1) ?
             (ticks) :
             (ticks << (4 + (gameticksDivision * 2)));
    
//    gameticksAvg = (gameticksAvg * gameticksAvgWeight + ticks) / ++gameticksAvgWeight;
    
    return (TIMER2_CR & TIMER_ENABLE) ? ticks : 0;
}

void handleGameticksTimerInterrupt() {
    // NOTE: Could maintain an overflow counter here to be capable of a higher range than 29 frames.
    //       Currently, however, we'll just stop the gameticks timer, returning value 0 in subsequent getGameticks calls.
    
    stopGameticks();
}

inline void startGameticksPrecision(unsigned division) {
    gameticksDivision = division;
    
    irqSet(IRQ_TIMER2, handleGameticksTimerInterrupt);
    
    TIMER2_DATA = 0;
    TIMER2_CR = TIMER_ENABLE | TIMER_IRQ_REQ | division;
}

inline void startGameticks() {
    stopGameticks();
    startGameticksPrecision(TIMER_DIV_256);
    // using TIMER_DIV_64, we can time up to (65536*64)-1 ticks or 4194303 ticks, which is over 7 frames 
    // using TIMER_DIV_256, we can time up to (65536*256)-1 ticks or 16777215 ticks, which is over 29 frames 
}
