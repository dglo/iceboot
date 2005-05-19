/* simulated flash driver...
 */
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <sys/mman.h>
#include <unistd.h>

#include "flash.h"
#include "booter/epxa.h"

/** invalid argument... */
#define FEINVALID -1
#define FENOVERIFY -2

#define FVERBOSE
#undef FVERBOSE

/* block size in bytes
 */
static int blkSize(void) { 
   static int pgsz = -1;
   if (pgsz==-1) pgsz = getpagesize();
   return pgsz;
}

static char *flash_start;
static char *flash_end;

static int addrtochip(const void *a) {
   const char *addr = (const char *) a;
   if (addr>=flash_start && addr<=flash_end-1) return 0;
   return -1;
}

/* convert address to block location...
 */
static int addrtoblock(const void *a) {
   const char *addr = (const char *) a;
   const char *fs0 = (const char *) flash_start;
   int idx;
   const int chip = addrtochip(a);
   if (chip==0) idx = addr - fs0;
   else return -1;
   return idx/blkSize();
}

/* lock against writes...
 */
int flash_lock(const void *from, int bytes) {
   const void *to = (const char *)from + bytes - 2;
   const int sblk = addrtoblock(from);
   const int eblk = addrtoblock(to);
   const int chip = addrtochip(from);
   char *addr;
   
   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return FEINVALID;

   if (chip!=addrtochip(to)) return FEINVALID;

#if defined(FVERBOSE)
   printf("flash_lock: %p for %d bytes\r\n", from, bytes);
   printf("  sblk, eblk, chip: %d %d %d [%d]\r\n", sblk, eblk, chip,
	  addrtochip(to));
#endif
   addr = flash_start + sblk*blkSize();
   if (mprotect(addr, (eblk-sblk+1)*blkSize(), PROT_READ)<0) {
      perror("flash_lock");
      return 1;
   }
   return 0;
}

/* lock against writes...
 */
int flash_unlock(const void *from, int bytes) {
   const void *to = (const char *)from + bytes - 2;
   const int sblk = addrtoblock(from);
   const int eblk = addrtoblock(to);
   const int chip = addrtochip(from);
   char *addr;
   
   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return FEINVALID;

   if (chip!=addrtochip(to)) return FEINVALID;

#if defined(FVERBOSE)
   printf("flash_unlock: %p for %d bytes\r\n", from, bytes);
   printf("  sblk, eblk, chip: %d %d %d [%d]\r\n", sblk, eblk, chip,
	  addrtochip(to));
#endif
   addr = flash_start + sblk*blkSize();
   if (mprotect(addr, (eblk-sblk+1)*blkSize(), PROT_READ|PROT_WRITE)<0) {
      perror("flash_unlock");
      return 1;
   }
   return 0;
}

/* write cnt words of mem to to */
int flash_write(void *to, const void *mem, int cnt) {
   if (to<(void *)flash_start || to>=(void *)flash_end) {
      printf("invalid flash address %p [%p -> %p]\r\n", 
	     to, flash_start, flash_end);
      return 1;
   }
   memcpy(to, mem, cnt);
   return 0;
}

/* bytes is size to erase in bytes... 
 */
int flash_erase(const void *from, int bytes) {
   const void *to = (const char *) from + bytes - 1;
   const int sblk = addrtoblock(from);
   const int eblk = addrtoblock(to);
   const int chip = addrtochip(to);
   void *addr;

#if defined(FVERBOSE)
   printf("flash_erase: %p for %d bytes\r\n", from, bytes);
   printf("  sblk, eblk, chip: %d %d %d [%d]\r\n", sblk, eblk, chip,
	  addrtochip(to));
#endif

   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return FEINVALID;
   if (chip!=addrtochip(to)) return FEINVALID;

   addr = flash_start + blkSize()*sblk;
   memset(addr, 0xff, blkSize()*(eblk-sblk+1));
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

int flash_code_overlaps(void *from, void *to) {
   /* ?!?!?? */
   return 0;
}

int flash_init(void) { return 0; }
int flash_get_limits(void *zero, void **f_start, void **f_end) {
   static int isInit = 0;
   
   if (!isInit) {
      /* FIXME: check for saved file...
       */
      void *p = mmap(NULL, 8*1024*1024, PROT_READ|PROT_WRITE, 
		     MAP_SHARED|MAP_ANON, -1, 0);
      memset(p, 0xff, 8*1024*1024);
      flash_start = p;
      flash_end = p + 8*1024*1024;
      mprotect(p, 8*1024*1024, PROT_READ);
      isInit = 1;
   }
   
   if (f_start!=NULL) *f_start = flash_start;
   if (f_end!=NULL) *f_end = flash_end;
   return 0;
}

int flash_get_block_info(int *block_size) {
   *block_size = blkSize();
   return 0;
}
