/* simple app used to interact with the
 * xp10 devel board...
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "stripe.h"
#include "sfi.h"

void delay(int);

static void *memalloc(size_t );
static void memfree(void *);

static int blperr = 0;

static int *stack;
static int sp = 0;
static int stacklen = 128;

static void push(int v) { stack[sp] = v; sp++; }
static int  pop(void) { int ret = stack[sp-1]; sp--; return ret; }

static void validateStack(int nentries) {
  if (sp<nentries) {
    printf("!!!stack underflow!!!\r\n");
  }
}

static int numberBase = 10;
static void pNumber(int num) {
  if (numberBase==10) printf("%d", num);
  else if (numberBase==16) printf("%x", num);
  else if (numberBase==2) {
    unsigned i;
    for (i=0x80000000; i!=0; i>>=1) printf("%c", (i&num)?'1':'0');
  }
  else printf("?");
}

static const char *pStack(const char *pt) {
  int i;
  printf("<%d> ", sp);
  for (i=0; i<sp; i++) { pNumber(stack[i]); printf(" "); }
  printf("\r\n");
  return pt;
}

static const char *pTop(const char *pt) {
  if (sp>0) pNumber(stack[sp-1]);
  else printf("<>");
  printf("\r\n");
  return pt;
}

static const char *peek8(const char *pt) {
  validateStack(1);
  push( *(const unsigned char *)pop());
  return pt;
}

static const char *poke8(const char *pt) {
  unsigned char *cp;
  int value;
  validateStack(2);
  cp = (unsigned char *) pop();
  value = pop();
  *cp = (unsigned char) value;
  return pt;
}

static const char *peek32(const char *pt) {
  validateStack(1);
  push( *(const unsigned *)pop());
  return pt;
}

static const char *poke32(const char *pt) {
  unsigned *cp;
  validateStack(2);
  cp = (unsigned *) pop();
  *cp = (unsigned) pop();
  return pt;
}

/* name buckets...
 */
typedef enum BucketTypeEnum { 
   CFUNC, CFUNC1, CFUNC2, CFUNC3, CFUNC4, FUNC, CONSTANT 
} BucketType;

typedef struct BucketStruct {
  const char *nm;
  BucketType type;
  union {
    CFunc cfunc;
    CFunc1 cfunc1;
    CFunc2 cfunc2;
    CFunc3 cfunc3;
    CFunc4 cfunc4;
    const char *func;
    int constant;
  } u;
  struct BucketStruct *next; /* next in line for this hash bucket... */
} Bucket;

/* all buckets...
 */
static Bucket *buckets;
static const int nbuckets = 512;

static int hashName(const char *key, int len) {
  unsigned hash;
  for (hash=len; len>0; len--) { hash = ((hash<<5)^(hash>>27))^*key++; }
  return hash % nbuckets;
}

static Bucket *lookupRootBucket(const char *nm) {
  const int len = strlen(nm);
  int idx = hashName(nm, len);
  /*printf("rootBucket: '%s' -> %d\r\n", nm, idx);*/
  return buckets + idx;
}

static Bucket *lookupBucket(const char *nm) {
  Bucket *rbp = lookupRootBucket(nm);
  Bucket *bp;
  
  if (rbp->nm==NULL) return NULL;
  
  for (bp = rbp; bp!=NULL; bp = bp->next)
    if (strcmp(bp->nm, nm)==0) {
      /*printf("lookup: '%s'[%p]: %p\r\n", nm, bp->u.cfunc, bp);*/
      return bp;
    }

  return NULL;
}

static Bucket *allocBucketFromRoot(const char *nm, Bucket *rbp) {
  Bucket *bp;

  /*printf("allocBucketFromRoot('%s', %p)\r\n", nm, rbp);*/

  if (rbp->nm==NULL) { bp = rbp; }
  else {
    for (bp = rbp; bp!=NULL; bp = bp->next) {
      if (strcmp(bp->nm, nm)==0) break;
    }
    if (bp==NULL) {
      /* not found...
       */
      bp = (Bucket *) memalloc(sizeof(Bucket));
      bp->nm = NULL;
    }
  }

  if (bp->nm==NULL) {
    bp->nm = strdup(nm);
    bp->next = NULL;
  }
  if (bp!=rbp) {
    /* insert into bucket chain...
     */
    bp->next = rbp->next;
    rbp->next = bp;
  }
  return bp;
}

/* allocate and fill name in a bucket, return bucket...
 */
static Bucket *allocBucket(const char *nm) {
  return allocBucketFromRoot(nm, lookupRootBucket(nm));
}

void addCFuncBucket(const char *nm, CFunc cf) {
  Bucket *bucket = allocBucket(nm);
  bucket->type = CFUNC;
  bucket->u.cfunc = cf;
}

void addCFunc1Bucket(const char *nm, CFunc1 cf) {
  Bucket *bucket = allocBucket(nm);
  bucket->type = CFUNC1;
  bucket->u.cfunc1 = cf;
}

void addCFunc2Bucket(const char *nm, CFunc2 cf) {
  Bucket *bucket = allocBucket(nm);
  bucket->type = CFUNC2;
  bucket->u.cfunc2 = cf;
}

void addCFunc3Bucket(const char *nm, CFunc3 cf) {
  Bucket *bucket = allocBucket(nm);
  bucket->type = CFUNC3;
  bucket->u.cfunc3 = cf;
}

void addCFunc4Bucket(const char *nm, CFunc4 cf) {
  Bucket *bucket = allocBucket(nm);
  bucket->type = CFUNC4;
  bucket->u.cfunc4 = cf;
}

static void addFuncBucket(const char *nm, const char *f) {
  Bucket *bucket = allocBucket(nm);
  bucket->type = FUNC;
  bucket->u.func = strdup(f);
}

void addConstantBucket(const char *nm, int constant) {
  Bucket *bucket = allocBucket(nm);
  bucket->type = CONSTANT;
  bucket->u.constant = constant;
}

/* colon word, next word we save, then we copy the rest...
 */
static const char *colon(const char *pr) {
  const char *func;
  char word[128];
  int len = 0;

  /* skip whitespace for name...
   */
  while (isspace(*pr)) pr++;

  /* get next word...
   */
  while (*pr && !isspace(*pr)) { word[len] = *pr; pr++; len++; }
  word[len] = 0;

  /* now find ';' or end of line...
   */
  for (func = pr; *pr!=';' && *pr; pr++) ;
  if (*pr) {
    *(char *)pr = 0;
    pr++;
  }
  addFuncBucket(word, func);
  return pr;
}
static const char *constant(const char *pr) {
  char word[128];
  int len = 0;
  const int val = pop();

  /* skip whitespace for name...
   */
  while (isspace(*pr)) pr++;

  /* get next word...
   */
  while (*pr && !isspace(*pr)) { word[len] = *pr; pr++; len++; }
  word[len] = 0;

  addConstantBucket(word, val);
  return pr;
}

/* iffunc, find else and endif words, strip out rest 
 * and exec them...
 */
static const char *iffunc(const char *pr) {
#if 0
  const char *func;
  char word[128];
  int len = 0;
  const int val = pop();
  const char *search[] = { "else", "endif" };
  
  while (1) {
    /* skip whitespace for...
     */
    while (isspace(*pr)) pr++;
    
    /* get next word...
     */
    while (*pr && !isspace(*pr)) { word[len] = *pr; pr++; len++; }
    word[len] = 0;
    
    /* now find ';' or end of line...
     */
    for (func = pr; *pr!=';' && *pr; pr++) ;
    if (*pr) {
      *(char *)pr = 0;
      pr++;
    }
    addFuncBucket(word, func);
  }
#endif
  return pr;
}


/* returns: 0 not a number, 1 number num <- number
 */
static int parseNumber(const char *wd, int *num) {
  int isNum = 0;
  char digits[] = "0123456789ABCDEF";
  int ret = 0;
  int base = numberBase;

  if (*wd=='$') {
    base = 16;
    wd++;
  }
  else if (*wd=='&') {
    base = 10;
    wd++;
  }
  else if (*wd=='%') {
    base = 2;
    wd++;
  }
  
  while (*wd) {
    int v = toupper(*wd);
    int i;

    for (i=0; i<base; i++) {
      if (v==digits[i]) {
	ret *= base;
	ret += i; 
	isNum = 1;
	break;
      }
    }

    /* couldn't find digit!
     */
    if (i==base) return 0;

    wd++;
  }
  if (num!=NULL && isNum) {
    *num = ret;
  }
  return isNum;
}

/* call back when we get a new line...
 *
 * return 0 ok, non-zero error...
 */
static int newLine(const char *line) {
  /* new line has come in...
   */
  while (*line) {
    char word[128];
    int wi = 0;
    int num;

    /* skip ws
     */
    while (isspace(*line)) line++;
    
    /* word -- it is either a constant or func or cfunc
     */
    while (*line && !isspace(*line)) {
      if (wi<sizeof(word)-1) {
	word[wi] = *line;
	wi++;
      }
      line++;
    }
    word[wi]=0;

    /* no word?
     */
    if (wi==0) continue;

    if (strcmp(line, "quit")==0) return 1;

    if (parseNumber(word, &num)) {
      push(num);
    }
    else {
      Bucket *bp = lookupBucket(word);
      if (bp==NULL) {
	printf("unknown word '%s'\r\n", word);
      }
      else {
	if (bp->type==CFUNC) line = bp->u.cfunc(line);
	else if (bp->type>=CFUNC1 && bp->type<=CFUNC4) {
	  int v[4], i;
	  const int nitems = bp->type - CFUNC1 + 1;
	  validateStack(nitems);
	  for (i=0; i<nitems; i++) v[i] = pop();
	  if (bp->type == CFUNC1) push(bp->u.cfunc1(v[0]));
	  else if (bp->type == CFUNC2) push(bp->u.cfunc2(v[1], v[0]));
	  else if (bp->type == CFUNC3) push(bp->u.cfunc3(v[2], v[1], v[0]));
	  else if (bp->type == CFUNC4) push(bp->u.cfunc4(v[3], v[2], v[1], v[0]));
	}
	else if (bp->type==FUNC) newLine(bp->u.func);
	else if (bp->type==CONSTANT) push(bp->u.constant);
      }
    }
  }
  return 0;
}

static int plus(int v1, int v2) { return v1+v2; }
static int minus(int v1, int v2) { return v1 - v2; }
static int times(int v1, int v2) { return v1*v2; }
static int divide(int v1, int v2) { return v1/v2; }
static int greater(int v1, int v2) { return v1>v2 ? -1 : 0; }
static int less(int v1, int v2) { return v1<v2 ? -1 : 0; }
static int equals(int v1, int v2) { return v1==v2 ? -1 : 0; }
static int swap(int v1, int v2) { push(v2); return v1; }
static int or(int v1, int v2) { return v1|v2; }
static int and(int v1, int v2) { return v1&v2; }
static int xor(int v1, int v2) { return v1^v2; }
static int shr(int v1, int v2) { return ((unsigned) v1) >> v2; }
static int shl(int v1, int v2) { return v1 << v2; }

/* print out list of words on word stack
 *
 * assume 80 columns output format...
 */
static const char *words(const char *p) {
  int i;

  for (i=0; i<nbuckets; i++) {
    if (buckets[i].nm!=NULL) {
      Bucket *bp;
      for (bp = buckets + i; bp!=NULL; bp=bp->next) {
	printf("[%p:%p] ", bp, bp->next); 
	if (bp==bp->next) {
	  printf("\r\n");
	  return p;
	}
	printf("%s", bp->nm);
	if (bp->type==CONSTANT) printf(" [%d -- %x]", 
				       bp->u.constant, bp->u.constant);
	else if (bp->type==FUNC) printf(" '%s'", bp->u.func);
	else if (bp->type==CFUNC) printf(" [%p]", bp->u.cfunc);
	printf("\r\n");
      }
    }
  }
  return p;
}

static const char *comment(const char *p) {
  while (*p) p++;
  return p;
}

static const char *drop(const char *p) {
  pop();
  return p;
}

/* crctab calculated by Mark G. Mendel, Network Systems Corporation */
static unsigned short crctab[256] = {
    0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
    0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
    0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
    0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
    0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
    0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
    0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
    0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
    0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
    0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
    0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
    0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
    0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
    0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
    0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
    0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
    0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
    0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
    0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
    0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
    0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
    0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
    0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
    0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
    0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
    0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
    0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
    0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
    0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
    0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
    0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
    0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
};

/*
 * updcrc macro derived from article Copyright (C) 1986 Stephen Satchell. 
 *  NOTE: First srgument must be in range 0 to 255.
 *        Second argument is referenced twice.
 * 
 * Programmers may incorporate any or all code into their programs, 
 * giving proper credit within the source. Publication of the 
 * source routines is permitted so long as proper credit is given 
 * to Stephen Satchell, Satchell Evaluations and Chuck Forsberg, 
 * Omen Technology.
 */
#define updcrc(cp, crc) ( crctab[((crc >> 8) & 255)] ^ (crc << 8) ^ cp)

static const char *ymodem1k(const char *p) {
  const int bllen = 1024+2;
  unsigned char *block = (unsigned char *) memalloc(bllen);
  int blp = 0;
  extern volatile int tx_head,tx_tail,rx_head,rx_tail;
  int i, cnt, firstblock = 1, blocknum, ndatabytes = 0;
  unsigned char ack = 0x06;
  int crc = 1;
  char *addr = NULL;
  unsigned char nak = 'C'; /* 0x15;*/
  int firstEOT = 1;
  int filelen;

  /* continuously write 'C' until we get a packet...
   */
  for (i=0; i<20; i++) {
    int count = 10000000;

    write(1, &nak, 1);
    while (rx_head==rx_tail && count>=0 ) count--;
    if (rx_head!=rx_tail) break;
  }

  /* read packet loop...
   */
  while (1) {
    int i, nr, ret, pktsz = 0;
    unsigned char pkt, blknum[2];

    if ((ret = read(0, &pkt, 1))!=1) {
      blperr = -1;
      push(0); push(0);
      return p;
    }

    if (pkt==0x01) {
      /* soh...
       */
      pktsz = 128;
    }
    else if (pkt==0x02) {
      /* stx...
       */
      pktsz = 1024;
    }
    else if (pkt==0x04) {
      unsigned char rnak = 0x15;
      
      /* eot...
       */
      blperr = ndatabytes;
      write(1, &rnak, 1);
      read(0, &rnak, 1);
      write(1, &ack, 1);
      write(1, &nak, 1);
      firstblock = 1;
      continue;
    }
    else {
      blperr = pkt&0xff;
      push(0); push(0);
      return p;
    }

    /* read block number...
     */
    blp = 0;
    while (blp<2) {
      int nr = read(0, blknum+blp, 2 - blp);
      if (nr<=0) {
	blperr = -3;
	push(0); push(0);
	return p;
      }
      blp+=nr;
    }

    if ( blknum[0] !=  ((~blknum[1])&0xff) ) {
      blperr = -4;
      push(0); push(0);
      return p;
    }

    if (firstblock) {
      firstblock = 0;
      
      if (blknum[0] == 0) {
	/* filename block
	 */
	blocknum = 0;
      }
      else if (blknum[0] == 1) {
	/* data block...
	 */
	blocknum = 1;
      }
      else {
	blperr = -6;
	push(0);
	push(0);
	return p;
      }
    }
    else {
      if (blknum[0] != (blocknum & 0xff)) {
	blperr = -5;
	push(0);
	push(0);
	return p;
      }
    }

    nr = pktsz + crc + 1;
    blp = 0;
    while (blp<nr) {
      int ret;
      if ((ret=read(0, block + blp, nr-blp))<=0) {
	blperr = -7;
	push(0);
	push(0);
	return p;
      }
      blp+=ret;
    }

    /* check crc...
     */
    if (crc) {
      unsigned short v = 0;
      for (i=0; i<nr; i++) v = updcrc(block[i], v);

      if (v!=0) {
	blperr = -8;
	push(0);
	push(0);
	return p;
      }
    }

    /* send ack...
     */
    write(1, &ack, 1);

    if (blocknum==0) {
      int i;

      /* parse name...
       */
      for (i=0; i<pktsz; i++) if (block[i]==0) break;
      
      if (i==0) {
	write(1, &nak, 1);
	break;
      }

      /* parse length...
       */
      filelen = atoi(block + i + 1);

      if (filelen>0) {
	addr = (char *) memalloc(filelen);
      }

      write(1, &nak, 1);
    }
    else {
      int tocp = (ndatabytes+pktsz > filelen) ? (filelen - ndatabytes) : pktsz;
      memcpy(addr + ndatabytes, block, tocp);
      ndatabytes+=tocp;
    }

    blocknum++;
  }

  push(ndatabytes);
  push((int) addr);
  return p;
}

/* re-program fpga...
 *
 * returns: 0 ok, non-zero error...
 */
static int fpga_config(int nbytes, int addr) {
  int *p =  (int *) addr;
  int *cclk = (int *) (EXC_REGISTERS_BASE + 0x144);
  int *ccntl = (int *) (EXC_REGISTERS_BASE + 0x140);
  int *idcode = (int *) (EXC_REGISTERS_BASE + 0x8);
  int *cdata = (int *) (EXC_REGISTERS_BASE + 0x148);
  int i;

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

  /* setup clock */
  *cclk = 4;

  /* check lock bit of ccntl */
  if ( (*(volatile int *)ccntl)&1 ) {
    int *cul = (int *) (EXC_REGISTERS_BASE + 0x14c);
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
	return 10;
      }
    }
  }

  /* wait for CO bit to clear...
   */
  while ((*(volatile int *) ccntl)&0x2) {
    if ( (*(volatile int *) ccntl)&0x10) {
      printf("config programming error (waiting for CO)!\n");
      return 10;
    }
  }
  
  return 0;
}

static const char *dup(const char *p) {
  int v = pop();
  push(v); push(v);
  return p;
}

#if 0
static const char *dosetupSDRAM(const char *p) {
  extern void setupSDRAM(void);
  setupSDRAM();
  return p;
}
#endif

int main(int argc, char *argv[]) {
  char *line;
  const int linesz = 256;
  static struct {
    const char *nm;
    CFunc cf;
  } initCFuncs[] = {
    { ".", pTop },
    { ".s", pStack },
    { "c!", poke8 },
    { "!", poke32 },
    { "@", peek32 },
    { "c@", peek8 },
    { ":", colon },
    { "if", iffunc },
    { "words", words },
    { "constant", constant },
    { "\\", comment },
    { "drop", drop },
    { "dup", dup },
    { "ymodem1k", ymodem1k },
    /*    { "setupSDRAM", dosetupSDRAM },*/
  };
  const int nInitCFuncs = sizeof(initCFuncs)/sizeof(initCFuncs[0]);
  static struct {
    const char *nm;
    CFunc2 cf;
  } initCFuncs2[] = {
    { "and", and },
    { "or", or },
    { "xor", xor },
    { "lshift", shl },
    { "rshift", shr },
    { "+", plus },
    { "-", minus },
    { "*", times },
    { "/", divide },
    { ">", greater },
    { "<", less },
    { "=", equals },
    { "swap", swap },
    { "fpga", fpga_config },
  };
  const int nInitCFuncs2 = sizeof(initCFuncs2)/sizeof(initCFuncs2[0]);

  static struct {
    const char *nm;
    int constant;
  } initConstants[] = {
    { "LEDS", EXC_PLD_BLOCK0_BASE },
    { "BASE", EXC_REGISTERS_BASE },
    { "SDRAM", 0x10000000 },
    { "true", -1 },
    { "false", 0 },
    { "base", (int) &numberBase },
    { "blperr", (int) &blperr },
  };
  const int nInitConstants = sizeof(initConstants)/sizeof(initConstants[0]);
  int li = 0, done = 0, i;
  extern void initMemTests(void);
  extern int end;

  if ((stack = (int *) memalloc(sizeof(int) * stacklen))==NULL) {
    printf("can't allocate stack...\n");
    return 1;
  }

  if ((buckets = (Bucket *) memalloc(sizeof(Bucket) * nbuckets))==NULL) {
    printf("can't allocate buckets...\n");
    return 1;
  }

  memset(buckets, 0, nbuckets*sizeof(Bucket));

  for (i=0; i<nInitCFuncs; i++) 
    addCFuncBucket(initCFuncs[i].nm, initCFuncs[i].cf);

  for (i=0; i<nInitCFuncs2; i++) 
    addCFunc2Bucket(initCFuncs2[i].nm, initCFuncs2[i].cf);

  for (i=0; i<nInitConstants; i++) 
    addConstantBucket(initConstants[i].nm, initConstants[i].constant);

  initMemTests();

  printf("--------\r\n");
  printf("Ready...\r\n");

  line = (char *) memalloc(linesz);

  write(1, "\r\n> ", 4);
  while (!done) {
    int nr, i;
    
    if (li==linesz) {
      printf("!!!! line too long!!!\r\n");
      break;
    }

    nr = read(0, line + li, linesz - li);

    if (nr<=0) {
      printf("!!!! can't read from stdin!!!!\r\n");
      break;
    }

    for (i=li; i<li+nr; i++) {
      /* echo the character back...
       */
      if (line[i]=='\r') continue;

      /* FIXME: deal with backspace here...
       */
      write(1, line+i, 1);
    }

    /* check for new line and call callback if found...
     */
    li += nr;
    for (i=0; i<li; i++) {
      /* look for '\r'
       */
      if (line[i]=='\r') {
	int j, k;

	write(1, "\r\n", 2);
	line[i] = 0;
	if (newLine(line)) {
	  printf("!!! error new line\r\n");
	  done = 1;
	  break;
	}
	for (k=0, j=i+1; j<li; j++, k++) line[k] = line[j];
	li = k;

	/* echo new line...
	 */
	write(1, "> ", 2);
      }
    }
  }

  return 0;
}

void delay(int time)
{
  int i;
  for(i = 0; i < time; i++);
}

void *memalloc(size_t sz) { return malloc(sz); }
void memfree(void *ptr) { free(ptr); }
