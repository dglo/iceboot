#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>

#include "iceboot/sfi.h"
#include "booter/epxa.h"
#include "iceboot/osdep.h"
#include "hal/DOM_MB_hal.h"

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

/* get m or n parameter...
 */
static int mnparm(int reg) {
   if (reg & 0x00040000) return 1;
   return (reg&0x7f)*2 + ((reg&0x00010000)?1:0);
}

static int kparm(int reg) {
   if (reg & 0x00040000) return 1;
   return (reg&0x3)*2 + ((reg&0x00010000)?1:0);
}

/* get pll1 frequency...
 */
static unsigned pllfreq(volatile int *addr) {
   const int n = mnparm(addr[0]);
   const int m = mnparm(addr[1]);
   const int k = kparm(addr[2]);
   return ((EXTERNAL_CLK * m)/n)/k;
}

static const char *prtPLL(const char *p) {
   const int plln = pop();
   volatile int *addr = NULL;
   volatile int *status = (volatile int *) (REGISTERS + 0x324);
   const char *locked;
   const char *bypassed;
   const char *changed;

   if (plln==1) {
      addr = (volatile int *) (REGISTERS + 0x300);
      locked = (*status & 1) ? "yes" : "no";
      bypassed = (*status & 0x10) ? "no" : "yes";
      changed = (*status & 0x4) ? "yes" : "no";
   }
   else if (plln==2) {
      addr = (volatile int *) (REGISTERS + 0x310);
      locked = (*status & 2) ? "yes" : "no";
      bypassed = (*status & 0x20) ? "no" : "yes";
      changed = (*status & 0x8) ? "yes" : "no";
   }
   else {
      printf("unknown plln, must be 1 or 2\r\n");
      return p;
   }

   if (addr!=NULL) {
      int freq = pllfreq(addr);
      const char *units = "Hz";
      
      if (freq%1000000 == 0) {
	 units = "MHz";
	 freq /= 1000000;
      }
      else if (freq%1000 == 0) {
	 units = "KHz";
	 freq /= 1000;
      }
      
      printf("pll %d: locked: %s, bypassed: %s, changed: %s, freq: %d%s"
	     " [%d %d %d]\r\n",
	     plln, locked, bypassed, changed, freq, units, 
	     mnparm(addr[0]), mnparm(addr[1]), kparm(addr[2]));
   }
   
   return p;
}

int osInit(int argc, char *argv[]) { 
   extern short *acqAddr;
   extern int acqLen;
   extern void addCFuncBucket(const char *nm, CFunc cf);

    addConstantBucket("REGISTERS", REGISTERS);
#if defined(CPLD_ADDR)
    addConstantBucket("CPLD", CPLD_ADDR);
#endif
    addCFuncBucket("prtPLL", prtPLL);

    /* setup acq buffers... */
    acqAddr = (short *)(MEMORY_SIZE/2);
    acqLen = MEMORY_SIZE/2;

    return 0; 
}

/* re-program fpga...
 *
 * returns: 0 ok, non-zero error...
 */
int fpga_config(int *p, int nbytes) {
  int *cclk = (int *) (REGISTERS + 0x144);
  int *ccntl = (int *) (REGISTERS + 0x140);
  int *idcode = (int *) (REGISTERS + 0x8);
  int *cdata = (int *) (REGISTERS + 0x148);
  int i;
  const int rbtreq = halIsFPGALoaded() && hal_FPGA_is_comm_avail();
  unsigned interrupts = 0;
  
  /* request reboot from DOR */
  if (rbtreq) hal_FPGA_request_reboot();

  /* check magic number...
   */
  if (p[0] != 0x00494253) {
    printf("invalid magic number...\n");
    return 4;
  }

  /* check buffer length */
  if ((nbytes%4) != 0 || nbytes<p[2]+p[3]) {
    printf("invalid buffer length\n");
    return 1;
  }

  /* check file length */
  if ((p[3]%4)!=0 || (p[2]%4)!=0) {
    printf("invalid file length/offset\n");
    return 1;
  }

  /* wait for reboot granted from DOR */
  if (rbtreq) {
     while (!hal_FPGA_is_reboot_granted()) ;
  }

  /* setup clock */
  *cclk = 4;

  /* check lock bit of ccntl */
  if ( (*(volatile int *)ccntl)&1 ) {
    int *cul = (int *) (REGISTERS + 0x14c);
    *(volatile int *) cul = 0x554e4c4b;
    
    if ((*(volatile int *)ccntl)&1 ) {
      printf("can't unlock config_control!\n");
      return 1;
    }
  }

  /* check idcode */
  if (*idcode!=p[1]) {
    printf("idcodes do not match\n");
    return 2;
  }

  /* save interrupts... */
  interrupts = *(volatile unsigned *) (REGISTERS + 0xc00);

  /* no interrupt sources during reload... */
  *(volatile unsigned *) (REGISTERS + 0xc04) = 0;


  /* set CO */
  *(volatile int *) ccntl = 2;

  /* write data... */
  for (i=(p[2]/4); i<(p[2]+p[3])/4; i++) {
    /* write next word */
    *(volatile int *) cdata = p[i];

    /* wait for busy bit... */
    while ( (*(volatile int *)ccntl)&4) {
      /* check for an error...
       */
      if ( (*(volatile int *) ccntl)&0x10) {
	printf("config programming error!\n");

        /* restore interrupts... */
        *(volatile unsigned *) (REGISTERS + 0xc04) = interrupts;
	return 10;
      }
    }
  }

  /* wait for CO bit to clear...
   */
  while ((*(volatile int *) ccntl)&0x2) {
    if ( (*(volatile int *) ccntl)&0x10) {
      /* restore interrupts... */
      *(volatile unsigned *) (REGISTERS + 0xc04) = interrupts;
      printf("config programming error (waiting for CO)!\n");
      return 10;
    }
  }

  /* restore interrupts... */
  *(volatile unsigned *) (REGISTERS + 0xc04) = interrupts;

#if 0
  /* load thresholds for comm -- these are defaults except sdelay
   * and rdelay...
   */
  if (hal_FPGA_query_type()==DOM_HAL_FPGA_TYPE_STF_COM) {
     *(unsigned *) 0x90081080 = (970<<16)|960; /* clev */
     *(unsigned *) 0x90081084 = (1<<24)|(1<<16)|(3<<8)|64;
  }
#endif

  /* load domid here so that "hardware" domid
   * works... sample domid: 710200073c7214d0
   *
   * FIXME: test fpga registers are used directly
   * here.  i'm not sure how else to do this, exporting
   * an fpga hal routine doesn't seem very nice, maybe
   * we should just export the fpga config routine from
   * the hal -- however, we don't want anyone else except
   * iceboot reloading the fpga probably...
   */
  {  
     unsigned long long domid = halGetBoardIDRaw();
     unsigned base = 
        (hal_FPGA_query_type()==DOM_HAL_FPGA_TYPE_DOMAPP) ?
        0x90000530 : 0x90081058;
     
     /* low 32 bits...
      */
     *(volatile unsigned *)(base) = domid&0xffffffff;

     /* high 16 bits + ready bit...
      */
     *(volatile unsigned *)(base+4) = (domid>>32) | 0x10000;
  }

  return 0;
}

/* re-program flasher board cpld...
 *
 * returns: 0 ok, non-zero error...
 */
int fb_cpld_config(int *p, int nbytes) {

    int err;

    /* Check data pointer */
    if (p == NULL) {
        printf("Error: file pointer argument is null!\n");
        return 1;
    }

    /* Check xsvf file sanity */
    /* Should start with XREPEAT, XENDDIR */
    if (p[0] != 0x00130007) {
        printf("Error: file doesn't appear to be a valid XSVF file.\n");
        return 1;
    }

    /* Check to make sure flasherboard is powered */
    if (!hal_FB_isEnabled()) {
        printf("Please power the flasherboard with enableFB first!\r\n");
        return 1;
    }
    
    /* Enable JTAG in PLD and FPGA */
    hal_FPGA_TEST_FB_JTAG_enable();
    halEnableFlasherJTAG();
    
    /* Execute xsvf player */
    err = hal_FB_xsvfExecute(p, nbytes);
    
    if (err == 0)
        printf("XSVF executed successfully!\n");
    else               
        printf("XSVF execution failed with error code %d.\n", err);
    
    /* Disable JTAG in PLD and FPGA  */
    hal_FPGA_TEST_FB_JTAG_disable();
    halDisableFlasherJTAG();
    
    return err;
}

/* wait input data
 */
int waitInputData(int ms) {
   int j;
   
   for (j=0; j<ms*10; j++) {
      halUSleep(100);
      if (halIsInputData()) break;
   }
   
   return halIsInputData();
}

void dcacheInvalidateAll(void) {
   unsigned _tmp1, _tmp2;
   asm volatile (
		 "mov    %0, #0;"
		 "1: "
		 "mov    %1, #0;"
		 "2: "
		 "orr    r0,%0,%1;"
		 "mcr    p15,0,r0,c7,c14,2;"  /* clean index in DCache */
		 "add    %1,%1,%2;"
		 "cmp    %1,%3;"
		 "bne    2b;"
		 "add    %0,%0,#0x04000000;"  /* get to next index */
		 "cmp    %0,#0;"
		 "bne    1b;"
		 "mcr    p15,0,r0,c7,c10,4;" /* drain the write buffer */
		 : "=r" (_tmp1), "=r" (_tmp2)
		 : "I" (0x20),
		 "I" (0x80)
		 : "r0" /* Clobber list */
		 );
   
   asm volatile ("mov    r1,#0;"
		 "mcr    p15,0,r1,c7,c6,0;" /* flush d-cache */
		 "mcr    p15,0,r1,c8,c6,0;" /* flush DTLB only */
		 :
		 :
		 : "r1" /* clobber list */);
}

void osDumpFlash(void) {}

