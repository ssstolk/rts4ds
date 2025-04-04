#include <nds/asminc.h>

	.text
    .arm
	.balign 4

@---------------------------------------------------------------------------------
BEGIN_ASM_FUNC memset32
@---------------------------------------------------------------------------------
@ void memset32(u32 *dest, u32 word, u32 size); 
@   r0 = dest 
@   r1 = word to fill 
@   r2 = number of BYTES to write 
@   all parameters MUST be word aligned, and size must be a multiple of 4
@   this is DWedit code taken from http://forum.gbadev.org/viewtopic.php?p=168702#168702
@---------------------------------------------------------------------------------

   @pre-subtract, jump ahead if not enough remaining 
   subs r2,r2,#32 
   bmi 2f
   stmfd sp!,{r3-r7,lr} 
   mov r3,r1 
   mov r4,r1 
   mov r5,r1 
   mov r6,r1 
   mov r7,r1 
   mov r12,r1 
   mov lr,r1 
1: 
   stmia r0!,{r1,r3-r7,r12,lr} @32 bytes 
   subs r2,r2,#32 
   bmi 3f 
   stmia r0!,{r1,r3-r7,r12,lr} 
   subs r2,r2,#32 
   bpl 1b 
3: 
   ldmfd sp!,{r3-r7,lr} 
2: 
   adds r2,r2,#32 
   bxle lr 
1:    
   str r1,[r0],#4 
   subs r2,r2,#4 
   bxle lr 
   str r1,[r0],#4 
   subs r2,r2,#4 
   bgt 1b 
   bx lr