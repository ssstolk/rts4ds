// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "vblankcount.h"

#include "debug.h"
#include "shared.h"


#define INFINITE_LOOP_VBLANK_COUNT    (1 * 60 * SCREEN_REFRESH_RATE)


int VBlankCounter = 0;

void handleVBlankInterrupt() {
    VBlankCounter++;
    
    if (VBlankCounter >= INFINITE_LOOP_VBLANK_COUNT) {
        REG_MASTER_BRIGHT_SUB = 0;
        
        defaultExceptionHandler();
		
        // generate an 'undefined instruction' exception, which will cause defaultExceptionHandler() to output
        asm(".arm\n\t"
            "ldmfd  sp!,{r0-r3,r12,r14}");
        
        // the extra numbers shown by defaultExceptionHandler() are values on the stack
        // - the 3rd one (2nd row, 1st col) is the previous program counter.... or is it the 8th one? (4th row, 2nd col)
        // - the 4th one (2nd row, 2nd col) is the previous stack pointer
        
        // using the following line, you can find the infinite loop's whereabouts:
        // \DevkitPro\devkitARM\bin\arm-none-eabi-addr2line.exe -f -C -s -i -e rts4ds.arm9.elf <pc>
    }
}

void resetVBlankCount() {
    VBlankCounter = 0;
}

int getVBlankCount() {
    return VBlankCounter;
}
