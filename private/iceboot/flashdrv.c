/* simulated flash driver...
 *
 * we try to emulate the intel flash chips
 * with an anonymous memory block...
 */
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <sys/mman.h>
#include <unistd.h>

#include "iceboot/flash.h"

/** invalid argument... */
#define FEINVALID -1
#define FENOVERIFY -2

#define FVERBOSE
#undef FVERBOSE

/* block size in 16 bit kwords 
 */
static int blkSize(int blk) { return (blk<8) ? 4*2*1024 : 32*2*1024; }

static char *flash_start;
static char *flash1_start;
static char *flash_end;

/* convert address to chip...
 */
static int addrtochip(const void *a) {
   const short *addr = (const short *) a;
   const short *fs0 = (const short *) flash_start;
   const short *fe0 = (const short *) (flash1_start-2);
   const short *fs1 = (const short *) flash1_start;
   const short *fe1 = (const short *) flash_end;

   if (addr>=fs0 && addr<=fe0)      { return 0; }
   else if (addr>=fs1 && addr<=fe1) { return 1; }
   return -1;
}

/* convert address to block location...
 */
static int addrtoblock(const void *a) {
   const short *addr = (const short *) a;
   const short *fs0 = (const short *) flash_start;
   const short *fs1 = (const short *) flash1_start;
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
   short *fs0 = (short *) flash_start;
   short *fs1 = (short *) flash1_start;
   short *addr;
   
   if (chip==0)      { addr = fs0; }
   else if (chip==1) { addr = fs1; }
   else { return NULL; }

   return addr + ( (blk<8) ? (blk*4*1024) : ( (blk-7)*32*1024) );
}

/* lock against writes...
 */
int flash_lock(const void *from, int bytes) {
   const void *to = (const char *)from + bytes - 2;
   int sblk = addrtoblock(from);
   const int eblk = addrtoblock(to);
   const int chip = addrtochip(from);

#if defined(FVERBOSE)
   printf("flash_lock: %p for %d bytes\r\n", from, bytes);
   printf("  sblk, eblk, chip: %d %d %d [%d]\r\n", sblk, eblk, chip,
	  addrtochip(to));
#endif
   
   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return FEINVALID;

   if (chip!=addrtochip(to)) return FEINVALID;

   while (sblk<=eblk) {
      void *addr = blktoaddr(chip, sblk);
      if (mprotect(addr, blkSize(sblk), PROT_READ)<0) {
	 perror("flash_lock");
	 return 1;
      }
      sblk++;
   }
   return 0;
}

/* lock against writes...
 */
int flash_unlock(const void *from, int bytes) {
   const void *to = (const char *)from + bytes - 2;
   int sblk = addrtoblock(from);
   const int eblk = addrtoblock(to);
   const int chip = addrtochip(from);
   
#if defined(FVERBOSE)
   printf("flash_unlock: %p for %d bytes\r\n", from, bytes);
   printf("  sblk, eblk, chip: %d %d %d [%d]\r\n", sblk, eblk, chip,
	  addrtochip(to));
#endif
   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return FEINVALID;

   if (chip!=addrtochip(to)) return FEINVALID;

   while (sblk<=eblk) {
      void *addr = blktoaddr(chip, sblk);
      if (mprotect(addr, blkSize(sblk), PROT_READ|PROT_WRITE)<0) {
	 perror("flash_unlock");
	 return 1;
      }
      sblk++;
   }
   return 0;
}

/* write cnt bytes of mem to to */
int flash_write(void *to, const void *mem, int cnt) {
   const int sblk = addrtoblock(to);
   const int eblk = addrtoblock((const char *)to+cnt-2);
   const int chip = addrtochip(to);
   
#if defined(FVERBOSE)
   printf("flash_write: %p for %d bytes from %p\r\n", to, cnt, mem);
   printf("  sblk, eblk, chip: %d %d %d [%d]\r\n", sblk, eblk, chip,
	  addrtochip(to));
#endif

   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return FEINVALID;

   if (chip!=addrtochip((const char *)to + cnt - 2)) return FEINVALID;

   memcpy(to, mem, cnt);

   return 0;
}

/* bytes is size to erase in bytes... 
 */
int flash_erase(const void *from, int bytes) {
   const void *to = (const char *) from + bytes - 1;
   int sblk = addrtoblock(from);
   const int eblk = addrtoblock(to);
   const int chip = addrtochip(to);

#if defined(FVERBOSE)
   printf("flash_erase: %p for %d bytes\r\n", from, bytes);
   printf("  sblk, eblk, chip: %d %d %d [%d]\r\n", sblk, eblk, chip,
	  addrtochip(to));
#endif

   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return FEINVALID;
   if (chip!=addrtochip(to)) return FEINVALID;

   while (sblk<=eblk) {
      void *addr = blktoaddr(chip, sblk);
      memset(addr, 0xff, blkSize(sblk));
      sblk++;
   }

   return 0;
}

const char *flash_errmsg(int stat) {
   static char msg[80];
   if (stat == FEINVALID) {
      sprintf(msg, "flash: error: invalid argument\r\n");
   }
   else if (stat == FENOVERIFY) {
      sprintf(msg, "flash: error: unable to verify\r\n");
   }
   else {
      sprintf(msg, "flash: error: %d\r\n", stat);
   }
   
   return msg;
}

int flash_verify_addr(const void *addr) {
   if (addr<(void *)flash_start || addr>=(void *)flash_end) return FEINVALID;
   return 0;
}

/* do the addresses overlap more than one chip?
 */
int flash_code_overlaps(void *from, void *to) {
   return addrtochip(from)!=addrtochip(to);
}

int flash_init(void) { return 0; }
int flash_get_limits(void *zero, void **f_start, void **f_end) {
   static int isInit = 0;
   
   if (!isInit) {
      /* FIXME: check for saved file...
       */
      void *p = mmap((void *) 0x41000000, 8*1024*1024, PROT_READ|PROT_WRITE,
		     MAP_SHARED|MAP_ANON, -1, 0);
      memset(p, 0xff, 8*1024*1024);
      flash_start = p;
      flash_end = p + 8*1024*1024;
      flash1_start = flash_start + 8*1024*1024/2;
      mprotect(p, 8*1024*1024, PROT_READ);
      isInit = 1;
   }
   
   if (f_start!=NULL) *f_start = flash_start;
   if (f_end!=NULL) *f_end = flash_end;
   return 0;
}

int flash_get_block_info(int *block_size) {
   *block_size = 64*1024;
   return 0;
}

void *flash_chip_addr(int chip) { return blktoaddr(chip, 0); }




