/* flash code for intel 32Mbit x 16bit Bottom boot flash (0x88c5)
 */
#include <stddef.h>
#include <string.h>

#include "flash.h"

/* block size in 16 bit words 
 */
static int blkSize(int blk) { return (blk<8) ? 4 : 32; }

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

int flash_lock(const void *from, const void *to) {
   const int sblk = addrtoblock(from);
   const int eblk = addrtoblock(to);
   const int chip = addrtochip(from);
   volatile short *flash;
   int i;
   
   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return -1;

   if (chip!=addrtochip(to)) return -2;

   flash = (volatile short *) blktoaddr(chip, 0);
   *flash = 0x60;
   
   for (i=sblk; i<=eblk; i++) {
      volatile short *block = blktoaddr(chip, i);
      *block = 0x01;
   }

   /* now confirm lock...
    */
   *flash = 0x90;
   for (i=sblk; i<=eblk; i++) {
      volatile short *block = blktoaddr(chip, i);
      const unsigned short v = *(block+2);
      if ( (v&1) != 1 ) break; 
   }
   
   /* back to read array mode... */
   *flash = 0xff;

   /* send back confirmation...
    */
   return (i<=eblk) ? -3 : 0;
}

int flash_unlock(const void *from, const void *to) {
   const int sblk = addrtoblock(from);
   const int eblk = addrtoblock(to);
   const int chip = addrtochip(from);
   volatile short *flash;
   int i;
   
   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return -1;

   if (chip!=addrtochip(to)) return -2;

   flash = blktoaddr(chip, 0);
   *flash = 0x60;
   
   for (i=sblk; i<=eblk; i++) {
      volatile short *block = blktoaddr(chip, i);
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

   /* send back confirmation...
    */
   return (i<=eblk) ? -3 : 0;
}

/* write cnt words of mem to to */
int flash_write(void *to, const void *mem, int cnt) {
   short *addr = (short *) to;
   const short *ptr = (const short *) mem;
   const int sblk = addrtoblock(to);
   const int eblk = addrtoblock((const short *)to+cnt);
   const int chip = addrtochip(to);
   volatile short *flash;
   int i;
   
   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return -1;

   if (chip!=addrtochip((const short *)to + cnt)) return -2;

   flash = blktoaddr(chip, 0);
   *flash = 0x40;

   for (i=0; i<cnt; i++) {
      unsigned short v;
      addr[i] = ptr[i];
      while (1) {
	 v = *(volatile short *)(addr + i);
	 if (v&0x80) break;
      }
   }

   /* back to read array mode... 
    */
   *flash = 0xff;

   /* send back confirmation...
    */
   return (memcmp(to, mem, cnt*2)==0) ? 0 : -1;
}

int flash_erase(const void *from, const void *to) {
   const int sblk = addrtoblock(from);
   const int eblk = addrtoblock(to);
   const int chip = addrtochip(from);
   volatile short *flash;
   int i;
   
   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return -1;

   if (chip!=addrtochip(to)) return -2;

   for (i=sblk; i<=eblk; i++) {
      volatile short *block = blktoaddr(chip, i);
      unsigned short v;
      
      *block = 0x20;
      *block = 0xd0;
      
      while (1) {
	 v = *block;
	 if (v&0x80) break;
      }
   }

   /* back to read array mode... */
   *flash = 0xff;

   /* send back confirmation...
    */
   for (i=sblk; i<=eblk; i++) {
      volatile unsigned short *block = blktoaddr(chip, i);
      int j;
      const int sz = blkSize(i);
      int err = 0;

      for (j=0; j<sz && !err; j++) {
	 if (block[j]!=0xffff) err = 1;
      }
      if (err) break;
   }
   
   return (i<=eblk) ? -1 : 0;
}


