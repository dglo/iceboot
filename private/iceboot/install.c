/* install a new (complete) flash image, read 
 * stdin for intel hex file format data and burn 
 * flash as you go.  before starting, erase all of
 * flash...
 */
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include "iceboot/flash.h"

/* returns nibble value or -1 if the 'c' is not
 * a nibble character...
 */
static int nibble(char c) {
   if (c>='0' && c<='9') return c - '0';
   if (c>='a' && c<='f') return c - 'a' + 10;
   if (c>='A' && c<='F') return c - 'A' + 10;
   return -1;
}

enum states {
   ST_START, ST_LEN0, ST_LEN1, /* 0, 1, 2 */
   ST_ADDR0, ST_ADDR1, ST_ADDR2, ST_ADDR3, /* 3, 4, 5, 6 */
   ST_TYPE0, ST_TYPE1, /* 7, 8 */
   ST_DATA, /* 9, 10 */
   ST_CK0, ST_CK1, /* 11, 12 */
   ST_OFFSET0, ST_OFFSET1, ST_OFFSET2, ST_OFFSET3 /* 13, 14, 15, 16, 17 */
};

int flashInstall(void) {
   void *fs, *fe;
   int nretries;
   int ck, cksum=0;
   int allDone = 0;
   int len;
   int nb = 0;
   int offset = 0;
   int ndata = 0;
   int type = 0;
   int addr = 0;
   int shift = 0;
   enum states state = ST_START;
   unsigned char data[256];
   int i;
   unsigned chip_start[2], chip_end[2];
   
   /* verify...
    */
   while (1) { 
      char c;
      int nr;
      
      printf("install: all flash data will be erased: are you sure [y/n]? ");
      fflush(stdout);

      nr = read(0, &c, 1);

      if (nr==1) {
	 printf("%c\r\n", c); fflush(stdout);
	 if (toupper(c)=='Y') break;
	 else if (toupper(c)=='N') return 1;
      }
      if (nretries==10) return 1;
   }
   
   /* erase flash...
    */
   flash_get_limits(NULL, &fs, &fe);

   chip_start[0] = (unsigned) fs;
   chip_start[1] = (unsigned) flash_chip_addr(1);
   chip_end[0] = chip_start[1]-1;
   chip_end[1] = (unsigned) fe;

   for (i=0; i<2; i++) {
      /* unlock all data -- except for iceboot (first 7 * 64K bytes)...
       */
      printf("chip %d: unlock... ", i); fflush(stdout);
      if (flash_unlock((void *)chip_start[i], chip_end[i] - chip_start[i])) {
	 printf("unable to unlock %08x -> %08x\r\n", 
		chip_start[i], chip_end[i]);
      }
      
      /* erase all data -- except for iceboot...
       */
      printf("erase... "); fflush(stdout);
      if (flash_erase((void *) chip_start[i], chip_end[i] - chip_start[i])) {
	 printf("unable to erase %08x -> %08x\r\n", 
		chip_start[i], chip_end[i]);
      }
      
      printf("\r\n");
   }

   /* program from stdin...
    *
    * use a very simple state machine to program incoming
    * data...
    */
   printf("programming... "); fflush(stdout);
   while (!allDone) {
      char line[256];
      int i;
      int nr;
      
      nr = read(0, line, sizeof(line));
      if (nr<0) {
	 fprintf(stderr, "install: read error!\r\n");
	 break;
      }
      else if (nr==0) {
	 fprintf(stderr, "install: eof!\r\n");
	 break;
      }

      for (i=0; i<nr; i++) {
	 char c = line[i];

#if 0
	 printf("c, state, nr, cksum: 0x%02x %d, %d, 0x%02x\r\n", 
		c, state, nr, cksum);
	 fflush(stdout);
#endif

	 if (state==ST_START) {
	    if (c==':') {
	       ck=0;
	       cksum=0;
	       state = ST_LEN0;
	    }
	 }
	 else if (state==ST_LEN0) {
	    int n = nibble(c);
	    if (n>=0) {
	       len = n;
	       state=ST_LEN1;
	    }
	 }
	 else if (state==ST_LEN1) {
	    int n = nibble(c);
	    if (n>=0) {
	       len<<=4;
	       len+=n;
	       cksum=len;
	       addr = 0;
	       state=ST_ADDR0;
	    }
	 }
	 else if (state==ST_ADDR0) {
	    int n = nibble(c);
	    if (n>=0) {
	       addr = n;
	       state = ST_ADDR1;
	    }
	 }
	 else if (state==ST_ADDR1) {
	    int n = nibble(c);
	    if (n>=0) {
	       addr <<= 4;
	       addr += n;

	       cksum+=addr;
	       state = ST_ADDR2;
	    }
	 }
	 else if (state==ST_ADDR2) {
	    int n = nibble(c);
	    if (n>=0) {
	       addr <<= 4;
	       addr += n;
	       state = ST_ADDR3;
	    }
	 }
	 else if (state==ST_ADDR3) {
	    int n = nibble(c);
	    if (n>=0) {
	       addr <<= 4;
	       addr += n;

	       cksum+= (addr&0xff);
	       type = 0;
	       state = ST_TYPE0;
	    }
	 }
	 else if (state==ST_TYPE0) {
	    int n = nibble(c);
	    if (n==0) {
	       state = ST_TYPE1;
	    }
	 }
	 else if (state==ST_TYPE1) {
	    int n = nibble(c);
	    if (n==0) {
	       ndata = 0;
	       nb = 0;
	       state = ST_DATA;
	    }
	    else if (n==1) {
	       /* last record! */
	       allDone = 1;
	       cksum+=1;
	       state = ST_CK0;
	    }
	    else if (n==2) {
	       offset = 0;
	       cksum += 2;
	       shift = 4;
	       state = ST_OFFSET0;
	    }
	    else if (n==4) {
	       offset = 0;
	       cksum += 4;
	       shift = 16;
	       state = ST_OFFSET0;
	    }
	 }
	 else if (state==ST_OFFSET0) {
	    int n = nibble(c);
	    if (n>=0) {
	       offset = n;
	       state = ST_OFFSET1;
	    }
	 }
	 else if (state==ST_OFFSET1) {
	    int n = nibble(c);
	    if (n>=0) {
	       offset<<=4;
	       offset += n;
	       cksum += offset;
	       state = ST_OFFSET2;
	    }
	 }
	 else if (state==ST_OFFSET2) {
	    int n = nibble(c);
	    if (n>=0) {
	       offset <<= 4;
	       offset += n;
	       state = ST_OFFSET3;
	    }
	 }
	 else if (state==ST_OFFSET3) {
	    int n = nibble(c);
	    if (n>=0) {
	       offset <<= 4;
	       offset += n;
	       ck = 0;
	       cksum += offset&0xff;
	       offset <<= shift;
	       state = ST_CK0;
	    }
	 }
	 else if (state==ST_DATA) {
	    int n = nibble(c);
	    if (n>=0) {
	       if (nb==0) {
		  data[ndata] = n;
		  nb++;
	       }
	       else {
		  data[ndata]<<=4;
		  data[ndata]+=n;
		  cksum+=data[ndata];
		  ndata++;
		  if (ndata==len) {
		     ck = 0;
		     state = ST_CK0;
		     /* now program the data... 
		      *
		      * FIXME: we are relying on the fact
		      * that we don't overlap chips here!!!
		      *
		      * probably safe as the chip boundry is
		      * a multiple of at least 16k...
		      */
		     if (flash_write(fs + offset + addr, data, ndata)) {
			printf("install: can't write flash!\r\n");
		     }
		  }
		  nb=0;
	       }
	    }
	 }
	 else if (state==ST_CK0) {
	    int n = nibble(c);
	    if (n>=0) {
	       ck = n;
	       state = ST_CK1;
	    }
	 }
	 else if (state==ST_CK1) {
	    int n = nibble(c);
	    if (n>=0) {
	       ck <<= 4;
	       ck += n;
	       /* FIXME: compare to calculated value... */

#if 0
	       printf("ck: 0x%02x, cksum: 0x%02x\r\n", 
		      ck, 0x100 - (cksum&0xff));
#endif
	       if (allDone) break;
	       else {
		  cksum = 0;
		  state = ST_START;
	       }
	    }
	 }
      }
   }

   for (i=0; i<2; i++) {
      /* unlock all data -- except for iceboot (first 7 * 64K bytes)...
       */
      printf("chip %d: lock... ", i); fflush(stdout);
      if (flash_unlock((void *)chip_start[i], chip_end[i] - chip_start[i])) {
	 printf("unable to unlock %08x -> %08x\r\n", 
		chip_start[i], chip_end[i]);
      }
      printf("\r\n");
   }

   return 0;
}



