/* flash code for intel 32Mbit x 16bit Bottom boot flash (0x88c5)
 */
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "iceboot/flash.h"
#include "booter/epxa.h"

/** invalid argument... */
#define FEINVALID -1
#define FENOVERIFY -2

#define FVERBOSE
#undef FVERBOSE

/* block size in 16 bit words 
 */
static int blkSize(int blk) { return (blk<8) ? 4*1024 : 32*1024; }

/* convert address to chip...
 */
static int addrtochip(const void *a) {
   const short *addr = (const short *) a;
   const short *fs0 = (const short *) 0x41000000;
   const short *fe0 = (const short *) (0x41400000-2);
   const short *fs1 = (const short *) 0x41400000;
   const short *fe1 = (const short *) (0x41800000-2);

   if (addr>=fs0 && addr<=fe0)      { return 0; }
   else if (addr>=fs1 && addr<=fe1) { return 1; }
   return -1;
}

/* convert address to block location...
 */
static int addrtoblock(const void *a) {
   const short *addr = (const short *) a;
   const short *fs0 = (const short *) 0x41000000;
   const short *fs1 = (const short *) 0x41400000;
   const int chip = addrtochip(a);
   int idx, bblk;
   if (chip==0)      { idx = addr - fs0; }
   else if (chip==1) { idx = addr - fs1; }
   else { return -1; }
   bblk = idx/(32*1024);
   return (bblk==0) ? (idx/(4*1024)) : (bblk + 7);
}

/* turn chip/blk -> addr
 */
static void *blktoaddr(int chip, int blk) {
   short *fs0 = (short *) 0x41000000;
   short *fs1 = (short *) 0x41400000;
   short *addr;
   
   if (chip==0)      { addr = fs0; }
   else if (chip==1) { addr = fs1; }
   else { return NULL; }

   return addr + ( (blk<8) ? (blk*4*1024) : ( (blk-7)*32*1024) );
}

static void setNoPrefetch(int chip) {
   unsigned volatile *addr = 
      (volatile unsigned *) (0x7fffc000 + ((chip==0) ? 0xc0 : 0xc4));
   const unsigned v = *addr;

   /* disable write posting for a bit...
    */
   *(volatile unsigned *) 0x7fffc100 = 0;

   /* set the no prefetch bit... */
   *addr = v|2;

   /* FIXME: write posting and read prefetch back on... */
   /* *(volatile unsigned *) 0x7fffc100 = 3; */
}

static void clearNoPrefetch(int chip) {
   unsigned volatile *addr = 
      (volatile unsigned *) (0x7fffc000 + ((chip==0) ? 0xc0 : 0xc4));
   const unsigned v = *addr;

   /* disable write posting for a bit...
    */
   *(volatile unsigned *) 0x7fffc100 = 0;

   /* clear the no prefetch bit... */
   *addr = v&(~2);

   /* FIXME: write posting and read prefetch back on... */
   /* *(volatile unsigned *) 0x7fffc100 = 3; */
}

int flash_lock(const void *from, int bytes) {
   const void *to = (const char *)from + bytes - 2;
   const int sblk = addrtoblock(from);
   const int eblk = addrtoblock(to);
   const int chip = addrtochip(from);
   volatile short *flash;
   int i;
   
   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return FEINVALID;

   if (chip!=addrtochip(to)) return FEINVALID;

#if defined(FVERBOSE)
   printf("flash_lock: %p for %d bytes\r\n", from, bytes);
   printf("  sblk, eblk, chip: %d %d %d [%d]\r\n", sblk, eblk, chip,
	  addrtochip(to));
#endif

   flash = (volatile short *) blktoaddr(chip, 0);
   setNoPrefetch(chip);
   *flash = 0x50; /* clear status register... */
   
   for (i=sblk; i<=eblk; i++) {
      volatile short *block = blktoaddr(chip, i);
      *flash = 0x60;
      *block = 0x01;
   }

   /* now confirm lock...
    */
   *flash = 0x90;
   for (i=sblk; i<=eblk; i++) {
      volatile short *block = blktoaddr(chip, i);
      const unsigned short v = *(block+2);
      if ((v&1) != 1) {
	 break;
      }
      
   }
   
   /* back to read array mode... */
   *flash = 0xff;
   clearNoPrefetch(chip);

   /* send back confirmation...
    */
   return (i<=eblk) ? FENOVERIFY : 0;
}

int flash_unlock(const void *from, int bytes) {
   const void *to = (const char *) from + bytes - 2;
   const int sblk = addrtoblock(from);
   const int eblk = addrtoblock(to);
   const int chip = addrtochip(from);
   volatile short *flash;
   int i;
   
#if defined(FVERBOSE)
   printf("flash_unlock: %p for %d bytes\r\n", from, bytes);
   printf("  sblk, eblk, chip: %d %d %d [%d]\r\n", sblk, eblk, chip,
	  addrtochip(to));
#endif

   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return FEINVALID;

   if (chip!=addrtochip(to)) return FEINVALID;

   flash = blktoaddr(chip, 0);
   setNoPrefetch(chip);
   *flash = 0x50; /* clear status register... */
   
   for (i=sblk; i<=eblk; i++) {
      volatile short *block = blktoaddr(chip, i);
      *flash = 0x60;
      *block = 0xd0;
   }

   /* now confirm lock...
    */
   *flash = 0x90;
   for (i=sblk; i<=eblk; i++) {
      volatile short *block = blktoaddr(chip, i);
      const unsigned short v = *(block+2);
      if ( (v&1) != 0 ) break; 
   }
   
   /* back to read array mode... */
   *flash = 0xff;
   clearNoPrefetch(chip);

   /* send back confirmation...
    */
   return (i<=eblk) ? FENOVERIFY : 0;
}

/* write cnt words of mem to to */
int flash_write(void *to, const void *mem, int cnt) {
   volatile short *addr = (volatile short *) to;
   const short *ptr = (const short *) mem;
   const int sblk = addrtoblock(to);
   const int eblk = addrtoblock((const char *)to+cnt-2);
   const int chip = addrtochip(to);
   volatile short *flash;
   int i;
   
#if defined(FVERBOSE)
   printf("flash_write: %p for %d bytes from %p\r\n", to, cnt, mem);
   printf("  sblk, eblk, chip: %d %d %d [%d]\r\n", sblk, eblk, chip,
	  addrtochip(to));
#endif

   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return FEINVALID;

   if (chip!=addrtochip((const char *)to + cnt - 2)) return FEINVALID;

   flash = blktoaddr(chip, 0);
   setNoPrefetch(chip);
   *flash = 0x50; /* clear status register... */

   for (i=0; i<(cnt+1)/2; i++) {
      unsigned short v;
      *flash = 0x40;
      addr[i] = ptr[i];
      while (1) {
	 v = *(volatile short *)flash;
	 if (v&0x80) break;
      }
   }

   /* back to read array mode... 
    */
   clearNoPrefetch(chip);
   *flash = 0xff;

   /* send back confirmation...
    */
   return (memcmp(to, mem, cnt)==0) ? 0 : FENOVERIFY;
}

/* bytes is size to erase in bytes... 
 */
int flash_erase(const void *from, int bytes) {
   const int sblk = addrtoblock(from);
   const void *to = (const char *) from + bytes - 2;
   const int eblk = addrtoblock(to);
   const int chip = addrtochip(from);
   volatile short *flash;
   int i;

#if defined(FVERBOSE)
   printf("flash_erase: %p for %d bytes\r\n", from, bytes);
   printf("  sblk, eblk, chip: %d %d %d [%d]\r\n", sblk, eblk, chip,
	  addrtochip(to));
#endif

   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return FEINVALID;
   if (chip!=addrtochip(to)) return FEINVALID;

   flash = blktoaddr(chip, 0);
   setNoPrefetch(chip);
   *flash = 0x50; /* clear status register... */

   for (i=sblk; i<=eblk; i++) {
      volatile short *block = blktoaddr(chip, i);
      unsigned short sr;

      *block = 0x20;
      *block = 0xd0;
      
      while (1) {
	 sr = *block;
	 if (sr&0x80) break;
      }

      /* error? */
      if (sr&(1<<5)) { 
	 if (sr&(1<<3)) {
#if defined(FVERBOSE)
	    printf("flash: error: vpp range error!\r\n");
#endif
	    break;
	 }
	 else if ( (sr&(3<<4)) == (3<<4)) {
#if defined(FVERBOSE)
	    printf("flash: error: command sequence error!\r\n");
#endif
	    break;
	 }
	 else if (sr&2) {
#if defined(FVERBOSE)
	    printf("flash: error: attempt to erase locked block!\r\n");
#endif
	    break;
	 }
	 else {
#if defined(FVERBOSE)
	    printf("flash: error: unknown error %04x\r\n", sr);
#endif
	    break;
	 }
      }
   }

   /* back to read array mode... 
    */
   flash = blktoaddr(chip, 0);
   *flash = 0xff;
   clearNoPrefetch(chip);

   /* send back confirmation...
    */
   for (i=sblk; i<=eblk; i++) {
      volatile unsigned short *block = blktoaddr(chip, i);
      int j;
      const int sz = blkSize(i);
      int err = 0;

      for (j=0; j<sz && !err; j++) {
	 if (block[j]!=0xffff) {
#if defined(FVERBOSE)
	    printf("verify error, chip %d, block %d [%p], offset %d (%d)\r\n",
		   chip, i, block, j, sz);
#endif
	    err = 1;
	 }
      }
      if (err) break;
   }
   
   return (i<=eblk) ? FENOVERIFY : 0;
}

const char *flash_errmsg(int stat) {
   static char msg[80];
#if defined(FVERBOSE)
   if (stat == FEINVALID) {
      sprintf(msg, "flash: error: invalid argument\r\n");
   }
   else if (stat == FENOVERIFY) {
      sprintf(msg, "flash: error: unable to verify\r\n");
   }
   else {
      sprintf(msg, "flash: error: %d\r\n", stat);
   }
#endif   

   return msg;
}

int flash_verify_addr(const void *addr) {
   const int chip = addrtochip(addr);
   return (chip==0 || chip==1) ? 0 : FEINVALID;
}

int flash_code_overlaps(void *from, void *to) {
   return addrtochip(from)!=addrtochip(to);
}

int flash_init(void) { return 0; }
int flash_get_limits(void *zero, void **flash_start, void **flash_end) {
   if (flash_start!=NULL) *flash_start = (void *) FLASH_CTL_START_ADDR;
   if (flash_end!=NULL) *flash_end = (void *) FLASH_CTL_END_ADDR;
   return 0;
}

int flash_get_block_info(int *block_size) {
   *block_size = 64*1024;
   return 0;
}

void *flash_chip_addr(int chip) { return blktoaddr(chip, 0); }
