// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "rumble.h"

#include <nds.h>
#include <nds/arm9/rumble.h>
#include <stdlib.h>

#include "shared.h"


// rumble uses TIMER0 and TIMER1
// wifi uses TIMER3


#define MAX_FREQUENCY_RANDOMIZATION      10
#define MAX_SWITCHDELAY_RANDOMIZATION  2000


enum RumbleType {
   RT_OFFICIAL,
   RT_WARIOWARE
};

enum RumbleType rumbleType;


static int rumbleLevel;
static int rumbleDuration;
static int rumbleTimer;

static int rumbleDelay;

static int rumbleDetected = -1;


//                                level:     0,     1,     2,     3,     4,     5,     6,     7,     8,     9,    10
// frequencies
int rumbleFrequencyStart[]           = {   40,    39,    38,    37,    36,    35,    34,    33,    32,    31,    30 };
int rumbleFrequencyEnd[]             = {   45,    44,    43,    42,    41,    40,    39,    38,    37,    36,    35 };
// switch delays
int rumblePositionSwitchDelayStart[] = {  4000,  5600,  7200,  8800, 10400, 12000, 13600, 15200, 16800, 18400, 20000 };
int rumblePositionSwitchDelayEnd[]   = {  4000,  4000,  4000,  4000,  4000,  4000,  4000,  4000,  4000,  8000, 10000 };

static volatile int rumbleTick = 0;


bool _rumbleInserted(void) {
    if (isDSiMode())
        return false;
    sysSetCartOwner(BUS_OWNER_ARM9);
    
    vu16 id;
    
    // First, check for 0x96 to see if it's a GBA game
	if (GBA_HEADER.is96h == 0x96) {
		//if it is a game, we check the game code
		//to see if it is warioware twisted
		if (	(GBA_HEADER.gamecode[0] == 'R') &&
				(GBA_HEADER.gamecode[1] == 'Z') &&
				(GBA_HEADER.gamecode[2] == 'W') &&
				(GBA_HEADER.gamecode[3] == 'E')
			)
		{
			rumbleType = WARIOWARE;
			WARIOWARE_ENABLE = 8;
			return true;
		}

		// Otherwise, we have a different game and thus no rumble
		return false;
	}

	// The Rumble Pak signifies itself this way (D1 pulled low)
	rumbleType = RUMBLE;
	id = *(vu16*)0x08000000;
    return (id == 0xFFFD || id == 0x7E00);
}

bool rumbleInserted() {
    // only checking once; the assumption being that the RumblePak 
    // is present from the very start of the game and not taken out
    if (rumbleDetected == -1)
        rumbleDetected = (int)_rumbleInserted();
    return (bool)rumbleDetected;
}

void setRumblePosition(bool position) {
    if (rumbleType == RT_WARIOWARE)
        WARIOWARE_PAK = (position ? 8 : 0);
    else
        RUMBLE_PAK = (position ? 2 : 0);
}


void stopRumble() {
    TIMER0_CR = 0;
    TIMER1_CR = 0;
    rumbleDuration = 0;
    rumbleTick = 0;
}


void adjustRumble() {
    int i;
    volatile int frequency   = (( (( rumbleTimer                   << 16) / rumbleDuration) * rumbleFrequencyStart[rumbleLevel] ) +
                                ( (((rumbleDuration - rumbleTimer) << 16) / rumbleDuration) * rumbleFrequencyEnd[rumbleLevel] )) >> 16;
    volatile int switchDelay = (( (( rumbleTimer                   << 16) / rumbleDuration) * rumblePositionSwitchDelayStart[rumbleLevel] ) +
                                ( (((rumbleDuration - rumbleTimer) << 16) / rumbleDuration) * rumblePositionSwitchDelayEnd[rumbleLevel] )) >> 16;
    
    TIMER0_CR = 0;
    TIMER1_CR = 0;
    
    frequency += ((rand() % (2*MAX_FREQUENCY_RANDOMIZATION + 1)) - MAX_FREQUENCY_RANDOMIZATION);
    switchDelay += ((rand() % (2*MAX_SWITCHDELAY_RANDOMIZATION + 1)) - MAX_SWITCHDELAY_RANDOMIZATION);
    
    TIMER0_DATA = TIMER_FREQ_64(frequency);
    TIMER1_DATA = TIMER_FREQ_64(frequency);
    
    TIMER0_CR = TIMER_ENABLE | TIMER_IRQ_REQ | TIMER_DIV_64;
    for(i=0; i<switchDelay; i++) {}
    if (i != switchDelay) return; // making sure the loop doesn't get optimized away
    TIMER1_CR = TIMER_ENABLE | TIMER_IRQ_REQ | TIMER_DIV_64;
}


void rumbleInterrupt() {
    static bool rumblePosition = false;
    static int sema = 0;
    if (sema) return;
    sema = 1;
    
    if (!rumbleInserted()) {
        stopRumble();
        return;
    }
    
    rumblePosition = !rumblePosition;
    setRumblePosition(rumblePosition);
    
    if (rumbleTick) { // second tick. value of 0 signifies first tick.
        if (rumbleTimer-- <= 0)
            stopRumble();
        else
            adjustRumble();
    }
    
    rumbleTick = !rumbleTick;
    sema = 0;
}


void startRumble(int level, int duration) {
    int i;
    volatile int switchDelay = rumblePositionSwitchDelayStart[level];
    
    stopRumble();
    
    rumbleLevel    = level;
    rumbleDuration = duration;
    rumbleTimer    = duration;
    
    if (rumbleDelay)
        return;
    
    irqSet(IRQ_TIMER0, rumbleInterrupt);
    irqSet(IRQ_TIMER1, rumbleInterrupt);
    irqEnable(IRQ_TIMER0);
    irqEnable(IRQ_TIMER1);
    
    rumbleInterrupt(); // first position
    for(i=0; i<switchDelay; i++) {}
    if (i != switchDelay) return; // making sure the loop doesn't get optimized away
    rumbleInterrupt(); // second position
}


void addRumble(int level, int duration) {
    if (!rumbleInserted()) {
        stopRumble();
        return;
    }
    
    if (level == 0) // though I did implement level 0 rumble, I assume that it is usually used to signify -no- rumble at all
        return;
    
    if (level > 10)
        level = 10;
    
    if (rumbleTimer > 0 && level < rumbleLevel)
        return;
    
    if (duration > 1)
        // duration is specified in game-frames here, needs to be converted to amount of rumbles
        duration = (((duration << 16) / FPS) *                                       // amount of seconds... shifted for accuracy
                    ((rumbleFrequencyStart[level] + rumbleFrequencyEnd[level]) / 2))  // amount of rumbles per second
                   >> 16; // shift it back
    
    if (rumbleTimer > 0 && level == rumbleLevel && duration < rumbleTimer)
        return;
    
    startRumble(level, duration);
}

void delayNewRumble() {
    stopRumble();
    rumbleLevel = 0;
    rumbleDelay = 1;
}

void executeWaitingRumble() {
    rumbleDelay = 0;
    addRumble(rumbleLevel, rumbleDuration);
}
