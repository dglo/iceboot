#include <string.h>

#include "osdep.h"

void icacheInvalidateAll(void) {
    /* this macro can discard dirty cache lines (N/A for ICache) */
    asm volatile (
        "mov    r1,#0;"
        "mcr    p15,0,r1,c7,c5,0;"  /* flush ICache */
        "mcr    p15,0,r1,c8,c5,0;"  /* flush ITLB only */
        "nop;" /* next few instructions may be via cache    */
        "nop;"
        "nop;"
        "nop;"
        "nop;"
        "nop;"
        :
        :
        : "r1" /* Clobber list */
        );
}

/* execute an image ...
 */
int executeImage(const void *addr, int nbytes) {
   void (*f)(void) = ( void (*)(void) ) 0x00400000;
   
   /* copy code to 0x00400000... */
   memcpy((void *) 0x00400000, addr, nbytes);

   /* disable interrupts... */
   asm volatile("msr cpsr_c, #0xd3");

   /* flush icache (just in case)... */
   icacheInvalidateAll();
   
   /* jump... */
   f();

   /* never returns... */
   return 0;
}

