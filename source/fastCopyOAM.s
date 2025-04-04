#include <nds/asminc.h>

	.text
	.arm
	.balign 4

@---------------------------------------------------------------------------------
BEGIN_ASM_FUNC fastCopyOAM
@---------------------------------------------------------------------------------

mov   r1,  #0x07000000 @ Location of OAM (main)
stmfd sp!, {r2-r9}     @ 8 word copy

@ begin rollout

ldmia r0!, {r2-r9} @ this block of 2 is repeated now to take place 32 times
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #2
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #3
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #4
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #5
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #6
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #7
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #8
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #9
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #10
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #11
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #12
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #13
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #14
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #15
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #16
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #17
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #18
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #19
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #20
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #21
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #22
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #23
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #24
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #25
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #26
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #27
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #28
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #29
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #30
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #31
stmia r1!, {r2-r9}
ldmia r0!, {r2-r9} @ block #32
stmia r1!, {r2-r9}

@ end rollout

ldmfd sp!, {r2-r9}
bx    lr
