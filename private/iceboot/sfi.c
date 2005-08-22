/**
 * \mainpage Iceboot
 * \section introduction Introduction
 *
 * Iceboot implements
 * a small subset of the standard forth environment, just
 * enough to get the devices on the board working.
 *
 * The up-arrow button will go back one command and
 * the down-arrow button will go up one command.  The
 * right and left arrow keys can be used to edit the
 * line.
 *
 * \section commands Commands
 *   \arg \b . (period) -- print top of stack
 *   \arg \b .s -- print entire stack contents
 *   \arg \e val \e addr \b c! -- set byte at address \e addr to \e val
 *   \arg \e val \e addr \b w! -- set 16 bit word at address \e addr to \e val
 *   \arg \e val \e addr \b ! -- set 32 bit word at address \e addr to \e val
 *   \arg \e addr \b @ -- read the 32 bit word at address \e addr
 *   \arg \e addr \b c@ -- read the byte at address \e addr
 *   \arg \e addr \b w@ -- read the 16 bit word at address \e addr
 *   \arg \b : -- start a function definition
 *   \arg \e cond \b if \e code1 \e [ \b else \e code2] \b endif -- 
 * if \e cond is true execute \e code1 otherwise execute \e code2.
 *   \arg \e val \b constant \e name -- set value of \e name to \e val
 *   \arg \b \ -- the rest of the line is a comment
 *   \arg \b drop -- drop the top value of the stack
 *   \arg \b dup -- duplicate the top value of the stack
 *   \arg \b ymodem1k -- push address and size of a buffer retrieved from
 * the ymodem protocol
 *   \arg \e nbytes \b allocate -- allocate nbytes on the heap
 *   \arg \e addr \b free -- free addr as allocated 
 *   \arg \e from \e to \e count \b icopy -- copy count words from \e 
 * from to \e to
 *   \arg \e end \e start \b ?DO \e code \b LOOP -- 
 *  execute code from \e start (inclusive) to \e end (exclusive), 
 *  the variable \e i contains the current iteration.
 *   \arg \e channel \e value \b writeDAC -- write \e val to dac at \e channel
 *   \arg \e us \b usleep -- sleep \e us microseconds
 *   \arg \e addr \e count \b od -- hex dump of \e count 
 * words at address \e addr
 *   \arg \b reboot -- reboot the machine
 *   \arg \e channel \b analogMuxInput -- set analog mux input to \e channel
 *   \arg \b readTemp -- read the temperature, value returned is coded
 *   \arg \e code \b prtTemp -- print coded temperature
 *   \arg \e channel \b readADC -- read the ADC counts from \e channel
 *   \arg \e val \b not -- return bitwise not of \e val
 *   \arg \e val1 \e val2 \b and -- return bitwise and of \e val1 and \e val2
 *   \arg \e val1 \e val2 \b or -- return bitwise or of \e val1 and \e val2
 *   \arg \e val1 \e val2 \b xor -- return bitwise xor of \e val1 and \e val2
 *   \arg \e val \e shift \b lshift -- return \e val shifted left \e shift
 *   \arg \e val \e shift \b rshift -- return \e val shifted right \e shift
 *   \arg \e val1 \e val2 \b + -- return sum of \e val1 and \e val2
 *   \arg \e val1 \e val2 \b - -- return difference of \e val1 and \e val2
 *   \arg \e val1 \e val2 \b * -- return multiplication of \e val1 and \e val2
 *   \arg \e val1 \e val2 \b / -- return division or of \e val1 and \e val2
 *   \arg \e val1 \e val2 \b > -- return greater than of \e val1 and \e val2
 *   \arg \e val1 \e val2 \b < -- return less than or of \e val1 and \e val2
 *   \arg \e val1 \e val2 \b = -- return equals of \e val1 and \e val2
 *   \arg \b swap -- swap top two stack values
 *   \arg \e addr \e count \b fpga -- program fpga from \e count bytes at
 * address \e addr
 *   \arg \e addr \e count \b interpret -- interpret \e count bytes of
 * forth code at address \e addr
 *   \arg \b REGISTERS -- base address of excalibur registers
 *   \arg \b CPLD  -- base address of cpld registers
 *   \arg \b true -- value of true
 *   \arg \b false -- value of false
 *   \arg \b base -- address of number base
 *   \arg \b rawCurrents -- address of flag to display raw or corrected
 * currents
 *   \arg \b disableCurrents -- address of flag to disable 
 * display of board currents
 *   \arg \b s" \e string \b" -- push addr and count of \e string
 *  chars on the stack
 *   \arg \e addr \e count \b type -- print contents of string at
 *  address \e addr with \e count bytes
 *   \arg \e addr \e count \b swicmd -- execute swi (software interrupt)
 *   command given by string at address \e addr with \e count bytes.
 *   \b prtRawCurrents -- print raw currents to screen.
 *   \b prtCookedCurrents -- print efficiency corrected currents to screen.
 *   \arg \e plln \b prtPLL -- print pll info for \e plln (1 or 2).
 *
 * \section notes Notes
 *   requires vt100 terminal set to 115200,N,8,1 hardware flow control...
 *
 * $Revision: 1.126 $
 * $Author: arthur $
 * $Date: 2005-08-22 14:52:07 $
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <setjmp.h>

#include <unistd.h>

#include "zlib.h"
#include "sfi.h"
#include "iceboot/memtests.h"
#include "hal/DOM_MB_hal.h"
#include "iceboot/flash.h"
#include "iceboot/fis.h"
#include "osdep.h"
#include "md5.h"
#include "versions.h"

/* location and length of the buffer
 * used for acquisition...
 *
 * these are initialized in osInit in osdep.c...
 */
short *acqAddr = NULL;
int acqLen = 0;

void delay(int);

static const char *udelay(const char *p);
static const char *writeDAC(const char *p);
static void *memalloc(size_t );
static void initMemTests(void);
static int newLine(const char *line);
static void setErr(const char *msg);

static int blperr = 0;

static int *stack = NULL;
static int sp = 0;
static int stacklen = 1024;

typedef enum {
   EXC_STACK_UNDERFLOW = 1,
   EXC_STACK_OVERFLOW,
   EXC_LINE_TOO_LONG,
   EXC_UNKNOWN_WORD,
} ExceptionCodes;

enum { ACQ_MODE_CPU = 1, 
       ACQ_MODE_DISC = 2, 
       ACQ_MODE_PP = 3,
       ACQ_MODE_LED = 4};

static jmp_buf jenv;

void push(int v) { 
   if (sp>=stacklen) {
      longjmp(jenv, EXC_STACK_OVERFLOW);
   }
   
   stack[sp] = v; 
   sp++; 
}


int  pop(void) {
   int ret;

   if (sp<1) {
      /* whoops! */
      longjmp(jenv, EXC_STACK_UNDERFLOW);
   }
   
   ret = stack[sp-1]; 
   sp--; 
   return ret; 
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
  push( *(const unsigned char *)pop());
  return pt;
}

static const char *poke8(const char *pt) {
  unsigned char *cp;
  int value;
  cp = (unsigned char *) pop();
  value = pop();
  *cp = (unsigned char) value;
  return pt;
}

static const char *peek16(const char *pt) {
  push( *(const unsigned short *)pop());
  return pt;
}

static const char *poke16(const char *pt) {
  unsigned short *cp;
  int value;
  cp = (unsigned short *) pop();
  value = pop();
  *cp = (unsigned short) value;
  return pt;
}

static const char *peek32(const char *pt) {
  push( *(const unsigned *)pop());
  return pt;
}

static const char *poke32(const char *pt) {
  unsigned *cp;
  cp = (unsigned *) pop();
  *cp = (unsigned) pop();
  return pt;
}

/* name buckets...
 */
typedef enum BucketTypeEnum { 
   CFUNC, CFUNC0, CFUNC1, CFUNC2, CFUNC3, CFUNC4, FUNC, CONSTANT 
} BucketType;

typedef struct BucketStruct {
  const char *nm;
  BucketType type;
  union {
    CFunc cfunc;
    CFunc0 cfunc0;
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
  return buckets + idx;
}

static Bucket *lookupBucket(const char *nm) {
  Bucket *rbp = lookupRootBucket(nm);
  Bucket *bp;
  
  if (rbp->nm==NULL) return NULL;
  
  for (bp = rbp; bp!=NULL; bp = bp->next)
    if (strcmp(bp->nm, nm)==0) {
      return bp;
    }

  return NULL;
}

static Bucket *allocBucketFromRoot(const char *nm, Bucket *rbp) {
  Bucket *bp;

  if (rbp->nm==NULL) { bp = rbp; }
  else {
     /* root bucket is not empty, we have to
      * search to see if the bucket already 
      * exists...
      */
     for (bp = rbp; bp!=NULL; bp = bp->next) {
	if (strcmp(bp->nm, nm)==0) {
	   /* found one!  clear it and return it...
	    */
	   if (bp->type==FUNC && bp->u.func!=NULL) {
	      free((char *)bp->u.func);
	      bp->u.func = NULL;
	   }
	   return bp;
	}
     }
     if (bp==NULL) {
	/* not found -- allocate memory...
	 */
	bp = (Bucket *) memalloc(sizeof(Bucket));
	bp->nm = NULL;
     }
  }

  if (bp->nm==NULL) {
     /* the bucket is newly allocated, fill it out...
      */
    bp->nm = strdup(nm);
    
    if (bp!=rbp) {
       bp->next = rbp->next;
       rbp->next = bp;
    }
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

void addCFunc0Bucket(const char *nm, CFunc0 cf) {
  Bucket *bucket = allocBucket(nm);
  bucket->type = CFUNC0;
  bucket->u.cfunc0 = cf;
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

/* put a string on stack (addr len)...
 */
static const char *squote(const char *pr) {
   char word[128];
   int len = 0;
   
   while (isspace(*pr)) pr++;
   
   while ( *pr && *pr!='\"') {
      word[len] = *pr;
      pr++;
      len++;
   }
   if (*pr=='\"') pr++;

   word[len] = 0;
   if (len!=0) {
      char *addr = strdup(word);
      push((int)addr);
   }
   else push(0);
   push(len);

   return pr;
}

static int loopCountValue = 0;
static int loopCount(void) { return loopCountValue; }

/* limit start ?DO code LOOP ...
 */
static const char *doloop(const char *pr) {
  char word[128];
  char line[256];

  line[0] = 0;
  while (1) {
     int len = 0;

     /* skip whitespace...
      */
     while (isspace(*pr)) pr++;

     /* get next word...
      */
     while (*pr && !isspace(*pr)) { word[len] = *pr; pr++; len++; }
     word[len] = 0;

     if (len==0 || strcmp(word, "LOOP")==0) {
	const int start = pop();
	const int stop = pop();
	
	for (loopCountValue=start; loopCountValue<stop; loopCountValue++) 
	   newLine(line);

	break;
     }
     else {
	if (line[0]!=0) strcat(line, " ");
	strcat(line, word);
     }
  }

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
 * and exec them -- ifs can not be nested...
 *
 * cond if code [else more_code] endif
 */
static const char *iffunc(const char *pr) {
  char ifline[128], elseline[128];
  const int val = pop();
  int iselse = 0;
  
  ifline[0] = 0;
  elseline[0] = 0;

  while (1) {
     char word[128];
     int wlen = 0;
     
     /* skip whitespace
      */
     while (isspace(*pr)) pr++;
     
     /* get next word...
      */
     while (*pr && !isspace(*pr)) { word[wlen] = *pr; pr++; wlen++; }
     word[wlen] = 0;
     
     if (strcmp(word, "else")==0) {
	iselse = 1;
     }
     else if (strcmp(word, "endif")==0 || strcmp(word, ";")==0 || *pr==0) {
	/* now we can execute proper value...
	 */
	break;
     }
     else {
	strcat( (iselse) ? elseline : ifline, word);
	strcat( (iselse) ? elseline : ifline, " ");
     }
  }

  if (newLine((val) ? ifline : elseline)) {
     printf("can't parse line: '%s'\r\n", 
	     (val) ? ifline : elseline);
  }
  
  return pr;
}

/* do begin until loop...
 */
static const char *begin(const char *p) {
   char line[128];

   /* initialize line */
   strcpy(line, "");
   
   /* keep getting words until we get a ';' or UNTIL...
    */
   while (1) {
      char word[128];
      int wlen = 0;

      /* skip whitespace...
       */
      while (isspace(*p)) p++;
  
      /* get next word...
       */
      while (*p && !isspace(*p)) { word[wlen] = *p; p++; wlen++; }
      word[wlen] = 0;

      if (strcmp(word, "until")==0) {
	 break;
      }
      else if (strcmp(word, ";")==0 || *p==0) {
	 printf("begin: end of line with no until!\r\n");
	 return p;
      }
      else {
	 strcat(line, word);
	 strcat(line, " ");
      }
   }
   
   /* execute line until condition is true...
    */
   while (1) {
      newLine(line);
      if (pop()!=0) break;
   }
   
   return p;
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
	 setErr(word);
	 longjmp(jenv, EXC_UNKNOWN_WORD);
      }
      else {
	if (bp->type==CFUNC) line = bp->u.cfunc(line);
	else if (bp->type>=CFUNC0 && bp->type<=CFUNC4) {
	  int v[4], i;
	  const int nitems = bp->type - CFUNC0;
	  for (i=0; i<nitems; i++) v[i] = pop();
	  if (bp->type == CFUNC0)      push(bp->u.cfunc0());
	  else if (bp->type == CFUNC1) push(bp->u.cfunc1(v[0]));
	  else if (bp->type == CFUNC2) push(bp->u.cfunc2(v[1], v[0]));
	  else if (bp->type == CFUNC3) push(bp->u.cfunc3(v[2], v[1], v[0]));
	  else if (bp->type == CFUNC4) 
	     push(bp->u.cfunc4(v[3], v[2], v[1], v[0]));
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
static int not(int v) { return ~v; }

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

static const char *memallocate(const char *p) {
   void *ptr = malloc(pop());
   push((int)ptr);
   push((ptr==NULL) ? 1 : 0);
   return p;
}

static const char *memfree(const char *p) {
   free((void *)pop());
   push(0);
   return p;
}

static const char *type(const char *p) {
   const int len = pop();
   const char *addr = (const char *) pop();
   write(1, addr, len);
   return p;
}

static const char *rcvMsg(const char *p) {
   char *msg = (char *) malloc(4096);
   char *tmp = NULL;
   int type, len;
   int ret = hal_FPGA_receive(&type, &len, msg);

   if (ret) {
      printf("rcv: unable to receive msg!\r\n");
      return p;
   }
   
   if (len>0) {
      tmp = (char *) malloc(len);
      memcpy(tmp, msg, len);
   }

   push(type);
   push((int)tmp);
   push(len);

   free(msg);
   return p;
}

static const char *sendMsg(const char *p) {
   const int len = pop();
   const char *addr = (const char *) pop();
   const int type = pop();
   
   if (hal_FPGA_send(type, len, addr)) {
      printf("send: unable to send msg!\r\n");
   }

   return p;
}

/* simulate "real" acquisition to sdram...
 * 
 * fill upper 16M with data...
 */
static void acq(int mask, int ntrig, int mode) {
   int halTrig = 0;
   short *addr = acqAddr;
   short *ch0 = NULL, *ch1 = NULL, *ch2 = NULL, *ch3 = NULL, 
      *ch4 = NULL, *ch5 = NULL, *ch6 = NULL, *ch7 = NULL, *fadc = NULL;
   short *clkbuf;
   int i, incr = 8;
   int maxacq;
   long long clk0, clk1;

   if (mask & 0x0f) {
     halTrig |= HAL_FPGA_TEST_TRIGGER_ATWD0;
     if (mask & 1) { ch0 = addr; addr += 128; incr += 128; }
     if (mask & 2) { ch1 = addr; addr += 128; incr += 128; }
     if (mask & 4) { ch2 = addr; addr += 128; incr += 128; }
     if (mask & 8) { ch3 = addr; addr += 128; incr += 128; }
   }
   if ((mask & 0xf0) && (mode != ACQ_MODE_PP)) {
     halTrig |= HAL_FPGA_TEST_TRIGGER_ATWD1;
     if (mask & 16) { ch4 = addr; addr += 128; incr += 128; }
     if (mask & 32) { ch5 = addr; addr += 128; incr += 128; }
     if (mask & 64) { ch6 = addr; addr += 128; incr += 128; }
     if (mask & 128) { ch7 = addr; addr += 128; incr += 128; }
   }
   if ((mask & 0x100) && (mode != ACQ_MODE_PP)) { 
     halTrig |= HAL_FPGA_TEST_TRIGGER_FADC;
     fadc = addr;
     addr += 256;
     incr += 256;
   }
   clkbuf = addr;

   maxacq = ( acqLen/sizeof(short)) / incr;
   if (ntrig > maxacq || ntrig == 0) ntrig = maxacq;
   
   /* enable ping-pong mode if selected */
   if (mode == ACQ_MODE_PP)
       hal_FPGA_TEST_enable_ping_pong();

   for (i=0; i < ntrig; i++) {

       if (mode != ACQ_MODE_PP) {
           /* start trigger */
           if (mode == ACQ_MODE_DISC) {
               hal_FPGA_TEST_trigger_disc(halTrig);
           }
           else if (mode == ACQ_MODE_CPU) {
               hal_FPGA_TEST_trigger_forced(halTrig);
           }
           else if (mode == ACQ_MODE_LED) {
               hal_FPGA_TEST_trigger_LED(halTrig);
           }
           
           /* wait for done...
            */
           while (!hal_FPGA_TEST_readout_done(halTrig)) ;

           /*
            * readout the ATWD timestamp clocks
            */
           clk0 = hal_FPGA_TEST_get_atwd0_clock();
           clk1 = hal_FPGA_TEST_get_atwd1_clock();
           memcpy(clkbuf, &clk0, 8);
           memcpy(clkbuf+4, &clk1, 8);
           
           /* now readout...
            */
           hal_FPGA_TEST_readout(ch0, ch1, ch2, ch3, 
                                 ch4, ch5, ch6, ch7, 128, 
                                 fadc, 256, halTrig);
       }
       else {         
           /* Wait until ping-pong trigger and then readout */
           hal_FPGA_TEST_readout_ping_pong(ch0, ch1, ch2, ch3, 128, mask); 

           /* Get the ATWD timestamp */
           clk0 = hal_FPGA_TEST_get_ping_pong_clock();
           memcpy(clkbuf, &clk0, 8);
           
           /* Indicate that read is done for that ATWD */
           hal_FPGA_TEST_readout_ping_pong_done();
       }

       if (ch0) ch0 += incr; 
       if (ch1) ch1 += incr; 
       if (ch2) ch2 += incr; 
       if (ch3) ch3 += incr;
       if (ch4) ch4 += incr; 
       if (ch5) ch5 += incr; 
       if (ch6) ch6 += incr; 
       if (ch7) ch7 += incr;
       if (fadc) fadc += incr; 
       clkbuf += incr;
   }

   /* disable ping-pong mode if selected */
   if (mode == ACQ_MODE_PP)
       hal_FPGA_TEST_disable_ping_pong();
}

static const char *acq_cpu(const char *p) {
   const int mask = pop();
   const int ntrig = pop();
   acq(mask, ntrig, ACQ_MODE_CPU);
   return p;
}

static const char *acq_disc(const char *p) {
   const int mask = pop();
   const int ntrig = pop();
   acq(mask, ntrig, ACQ_MODE_DISC);
   return p;
}

static const char *acq_pp(const char *p) {
   const int mask = pop();
   const int ntrig = pop();
   acq(mask, ntrig, ACQ_MODE_PP);
   return p;
}

static const char *acq_led(const char *p) {
   const int mask = pop();
   const int ntrig = pop();
   acq(mask, ntrig, ACQ_MODE_LED);
   return p;
}

static int readDAC(int channel) { return halReadDAC(channel); }
static int readADC(int adc) { return halReadADC(adc); }
static int readBaseADC(void) { return halReadBaseADC(); }

static const char *reqBoot(const char *p) {
  /* request reboot from DOR */
  if (halIsFPGALoaded()) {
     hal_FPGA_request_reboot();
     while (!hal_FPGA_is_reboot_granted()) ;
  }
  
  return p;
}

static const char *md5sum(const char *p) {
   int len = pop();
   void *addr = (void *) pop();
   unsigned char bin_buffer[128/8];
   char *msg = (unsigned char *) calloc(128/4, 1);
   int i;

   memset(bin_buffer, 0, sizeof(bin_buffer));
   md5_buffer(addr, len, bin_buffer);
   for (i=0; i<128/8; i++) {
      const char digit[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8',
                             '9', 'a', 'b', 'c', 'd', 'e', 'f' };
      msg[i*2] = digit[(bin_buffer[i]>>4)&0xf];
      msg[i*2+1] = digit[bin_buffer[i]&0xf];
   }
   push((int) msg);
   push(128/4);
   
   return p;
}

static const char *iset(const char *p) {
   const int n = pop();
   int *to = (int *) pop();
   const int value = pop();
   int i;
   
   for (i=0; i<n; i++) to[i] = value;
   return p;
}

/* copy integers from one location to another...
 * 
 * from to n
 */
static const char *icopy(const char *p) {
   const int n = pop();
   int *to = (int *)pop();
   const int *from = (int *) pop();
   int i;
   
   for (i=0; i<n; i++) to[i] = from[i];
   return p;
}

static const char *enableHV(const char *p) {
   halPowerUpBase();
   halEnableBaseHV();
   return p;
}

static const char *disableHV(const char *p) {
   halPowerDownBase();
   return p;
}

static const char *enableLED(const char *p) {
    hal_FPGA_TEST_enable_LED();
    return p;
}

static const char *disableLED(const char *p) {
    hal_FPGA_TEST_disable_LED();
    return p;
}

static const char *setLEDdelay(const char *p) {
   int delay = pop();
   hal_FPGA_TEST_set_atwd_LED_delay(delay);
   return p;
}

static const char *enableFB(const char *p) {
    int err, config_t, valid_t, reset_t;    
    err = hal_FB_enable(&config_t, &valid_t, &reset_t, DOM_FPGA_TEST);
    if (err != 0) {
        switch(err) {
        case FB_HAL_ERR_CONFIG_TIME:
            printf("Error: flasherboard configuration time too long\r\n");
            break;
        case FB_HAL_ERR_VALID_TIME:
            printf("Error: flasherboard clock validation time too long\r\n");
            break;
        case FB_HAL_ERR_RESET_TIME:
            printf("Error: flasherboard power-on reset time too long\r\n");
            break;
        default:
            printf("Error: unknown flasherboard enable failure\r\n");
            break;
        }
    }
    push(reset_t);
    push(valid_t);
    push(config_t);
    push(err);
    return p;
}

static const char *enableFBmin(const char *p) {
    
    /* Warn users of risks... */
    while (1) { 
        char c;
        int nr;
        
        printf("enableFBmin: LEDs may come on (steady-state) on older flasherboards.\r\n");
        printf("Use enableFB unless CPLD state prevents it!  Continue [y/n]? ");
        fflush(stdout);
        
        nr = read(0, &c, 1);
        
        if (nr==1) {
            printf("%c\r\n", c); fflush(stdout);
            if (toupper(c)=='Y') break;
            else if (toupper(c)=='N') { return p; }
        }
    }
    
    hal_FB_enable_min();
    return p;
}

static const char *disableFB(const char *p) {
   hal_FB_disable();
   return p;
}

static const char *setFBbrightness(const char *p) {
    if (hal_FB_isEnabled()) {
        int value = pop();
        hal_FB_set_brightness(value);
    }
    else
        printf("Please power the flasherboard with enableFB first!\r\n");

    return p;
}

static const char *setFBwidth(const char *p) {
    if (hal_FB_isEnabled()) {
        int value = pop();
        hal_FB_set_pulse_width(value);
    }
    else
        printf("Please power the flasherboard with enableFB first!\r\n");
    return p;
}

static const char *setFBenables(const char *p) {
    if (hal_FB_isEnabled()){ 
        int value = pop();
        hal_FB_enable_LEDs(value);
    }
    else
        printf("Please power the flasherboard with enableFB first!\r\n");
    return p;
}

static const char *setFBmux(const char *p) {
    if (hal_FB_isEnabled()) {
        int value = pop();
        hal_FB_select_mux_input(value);
    }
    else
        printf("Please power the flasherboard with enableFB first!\r\n");
    return p;
}

static const char *startFBflashing(const char *p) {
    if (hal_FB_isEnabled())
        hal_FPGA_TEST_start_FB_flashing();
    else
        printf("Please power the flasherboard with enableFB first!\r\n");
    return p;
}

static const char *stopFBflashing(const char *p) {
    if (hal_FB_isEnabled())
        hal_FPGA_TEST_stop_FB_flashing();
    else
        printf("Please power the flasherboard with enableFB first!\r\n");    
    return p;
}

static int getFBfw(void) {
    int version = 0;
    if (hal_FB_isEnabled())
        version = hal_FB_get_fw_version();
    else
        printf("Please power the flasherboard with enableFB first!\r\n");
    return version;

}

static int getFBhw(void) {
    int version = 0;
    if (hal_FB_isEnabled())
        version = hal_FB_get_hw_version();
    else
        printf("Please power the flasherboard with enableFB first!\r\n");    
    return version;
}

static const char *setFBdcdc(const char *p) {
    if (hal_FB_isEnabled()) {
        int value = pop();
        hal_FB_set_DCDCen(value);
    }
    else
        printf("Please power the flasherboard with enableFB first!\r\n");    
    return p;
}

static const char *setFBrate(const char *p) {
    if (hal_FB_isEnabled()) {
        int value = pop();
        hal_FPGA_TEST_FB_set_rate(value);
    }
    else
        printf("Please power the flasherboard with enableFB first!\r\n");    
    return p;
}

static int getFBdcdc(void) {
    int val = -1;
    if (hal_FB_isEnabled())
        val = hal_FB_get_DCDCen();
    else
        printf("Please power the flasherboard with enableFB first!\r\n");    
    return val;
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

/* check for serial data...
 */
static const char *ymodem1k(const char *p) {
  const int bllen = 1024+2;
  unsigned char *block = (unsigned char *) memalloc(bllen);
  int blp = 0;
  int i, blocknum=0, ndatabytes = 0;
  unsigned char ack = 0x06;
  int crc = 1;
  char *addr = NULL;
  unsigned char nak = 'C'; /* 0x43;*/
  int filelen = 0;
  unsigned char CAN[2] = { 0x18, 0x18 };
  int transferring = 0;

  /* continuously write 'C' until we get a packet...
   */
  for (i=0; i<400; i++) {
     write(1, &nak, 1);
     if (waitInputData(200)) break;
  }

  /* read packet loop...
   */
  while (1) {
    int i, nr, ret, pktsz = 0;
    unsigned char pkt, blknum[2];

    if ((ret = read(0, &pkt, 1))!=1) {
      blperr = -1;
      push(0); push(0);
      write(1, CAN, 2);
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
      transferring = 0;
      continue;
    }
    else {
       printf("ymodem1k: unexpected start character: 0x%02x [%c]\r\n", 
	      pkt, pkt);

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
        write(1, CAN, 2);
	push(0); push(0);
	return p;
      }
      blp+=nr;
    }

    if ( blknum[0] !=  ((~blknum[1])&0xff) ) {
      blperr = -4;
      write(1, CAN, 2);
      push(0); push(0);
      return p;
    }

    if (pkt==0x01 && blknum[0]==0 && !transferring) {
       /* start of frame -- header block... */
       blocknum=0;
       transferring=0;
    }
    else {
       /* blocknum must match!!! */
       if (blknum[0] != (blocknum & 0xff)) {
          /* fixme, we should just start over here...
           */
          blperr = -5;
          write(1, CAN, 2);
          push(0);
          push(0);
          return p;
       }

       transferring = 1;
    }
    
    /* read the data portion of the packet... */
    nr = pktsz + crc + 1;
    blp = 0;
    while (blp<nr) {
       int ret;
       if ((ret=read(0, block + blp, nr-blp))<=0) {
          blperr = -7;
          write(1, CAN, 2);
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
          write(1, CAN, 2);
          push(0);
          push(0);
          return p;
       }
    }
    
    /* send ack...
     */
    write(1, &ack, 1);
    
    /* first packet has the name... */
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
       int tocp = 
          (ndatabytes+pktsz > filelen) ? (filelen - ndatabytes) : pktsz;
       memcpy(addr + ndatabytes, block, tocp);
       ndatabytes+=tocp;
    }
    
    blocknum++;
  }

  push((int) addr);
  push(ndatabytes);
  return p;
}

static const char *invalidateICache(const char *p) {
   icacheInvalidateAll();
   return p;
}

/* exec a binary image.  the image is to be loaded at
 * 0x00400000 (4M) and executed from there...
 */
static const char *execBin(const char *p) {
   int nbytes = pop();
   const void *addr = (void *)pop();
   executeImage(addr, nbytes);
   return p;
}

/* use buffer as interpreted input...
 *
 * ignore '\r', new lines every '\n'...
 */
static int interpret(int addr, int nbytes) {
   const char *buf = (const char *) addr;
   char line[256];
   int i = 0;
   int lineno = 1;
   int len = 0;
      
   while (i<nbytes) {
      if (buf[i]=='\n') {
	 int status;
	 
	 line[len] = 0;
	 if ((status=newLine(line))) {
	    printf("parse error, line: %d\r\n", lineno);
	    return status;
	 }
	 lineno++;
	 len = 0;
      }
      else if (buf[i]!='\r') {
	 line[len] = buf[i];
	 len++;
      }
      i++;
   }
   
   if (len>0) {
      int status;
      
      line[len] = 0;
      if ((status=newLine(line))) {
	 printf("parse error, (final) line: %d\r\n", lineno);
	 return status;
      }
   }
   
   return 0;
}

/* re-program fpga...
 *
 * returns: 0 ok, non-zero error...
 */
static int do_fpga_config(int addr, int nbytes) {
   return fpga_config((int *) addr, nbytes);
}

/* re-program flasher board cpld...
 *
 * returns: 0 ok, non-zero error...
 */
static int do_fb_cpld_config(int addr, int nbytes) {
   return fb_cpld_config((int *) addr, nbytes);
}

static const char *sdup(const char *p) {
  int v = pop();
  push(v); push(v);
  return p;
}

/*
 * do a fast, compressed binary dump like od.
 * note the non-standard convention of
 * dumping words not bytes is nevertheless
 * slavishly followed.
 */
#define ZDUMP_IOBUFSIZ 400
static const char *zdump(const char *p) {
  const int cnt = pop();
  unsigned int *addr = (unsigned int*) pop();
  char iobuf[ZDUMP_IOBUFSIZ];
  int err, flush;
  z_stream zs;
  /* Give downstream decoder a heads-up on original text size */
  write(1, &cnt, 4);
  zs.zalloc = Z_NULL; zs.zfree = Z_NULL; zs.opaque = Z_NULL;
  err = deflateInit(&zs, 5);
  zs.next_in = (char*) addr;
  zs.avail_in = 4*cnt;
  flush = Z_NO_FLUSH;
  while (err != Z_STREAM_END) {
    zs.next_out = iobuf;
    zs.avail_out = sizeof(iobuf);
    err = deflate(&zs, flush);
    if (zs.avail_out > 0) flush = Z_FINISH;
    write(1, iobuf, sizeof(iobuf)-zs.avail_out);
  }
  err = deflateEnd(&zs);
  return p;
}   

/* dump memory at address count
 */
static const char *odump(const char *p) {
   const int cnt = pop();
   unsigned *addr = (unsigned *) pop();
   int i;
   
   for (i=0; i<cnt; i+=4, addr+=4) {
      printf("%08x %08x ", (unsigned) addr, addr[0]);
      if (i<cnt-1) printf("%08x ", addr[1]);
      if (i<cnt-2) printf("%08x ", addr[2]);
      if (i<cnt-3) printf("%08x ", addr[3]);
      printf("\r\n");
   }
   return p;
}

/* dump memory at address count -- test version...
 *
 * unless you've a strong stomach, avert your
 * eyes from this code, the intention is to keep
 * the whole routine in icache...
 */
static void pch(char ch) {
   unsigned uch = ch;
   /* wait for fifo to clear */
   while (((*(volatile unsigned *)0x7fffc28c) & 0x1f)>=15) ;
   *(volatile unsigned *)0x7fffc290 = uch;
}

static void phex(unsigned v) {
   char digits[] = "0123456789abcdef";
   int shift = 28;
   while (shift>=0) {
      pch( digits[(v>>shift)&0xf] );
      shift-=4;
   }
   pch(' ');
}

static const char *odumpt(const char *p) {
   const int cnt = pop();
   unsigned *addr = (unsigned *) pop();
   int i;

   for (i=0; i<cnt; i+=4, addr+=4) {
      phex((unsigned) addr);
      phex(addr[0]);
      if (i<cnt-1) phex(addr[1]);
      if (i<cnt-2) phex(addr[2]);
      if (i<cnt-3) phex(addr[3]);
      pch('\r'); pch('\n');
   }
   return p;
}

static const char *flashReboot(const char *p) {
   halSetFlashBoot();
   return p;
}

static const char *serialReboot(const char *p) {
   halClrFlashBoot();
   return p;
}

static const char *rebootDOM(const char *p) {
   halBoardReboot();
   return p;
}

static const char *writePassiveBaseDAC(const char *p) {
   int value = pop();
   halWritePassiveBaseDAC(value);
   return p;
}

static const char *writeActiveBaseDAC(const char *p) {
   int value = pop();
   halWriteActiveBaseDAC(value);
   return p;
}

static int readBaseDAC(void) { return halReadBaseDAC(); }

static const char *analogMuxInput(const char *p) {
   const int chan = pop();
   halSelectAnalogMuxInput(chan);
   return p;
}

static int readTemp(void) { return halReadTemp(); }

static float formatTemp(int temp) {
   signed char t = (temp>>8);
   float v = t;
   int i;
   int mask = 0x80;
   for (i=0; i<4; i++, mask>>=1) {
      const float frac = 1.0 / (1<<(i+1));
      if (temp&mask) v+=frac;
   }
   return v;
}

static const char *prtTemp(const char *p) {
   const int temp = pop();
   printf("temperature: %.2f degrees C\r\n", formatTemp(temp));
   return p;
}

static int readPressure(void) {  

    unsigned int loop_cnt = 50;
    unsigned long pressure_sum = 0;
    unsigned long voltage_sum = 0;
    float mean_pressure, mean_voltage;
    int pressure, i;

    halEnableBarometer();
    halUSleep(1000);

    /* Average the barometer reading and the 5V supply reading */
    for(i = 0; i < loop_cnt; i++) {
        pressure_sum += halReadADC(DOM_HAL_ADC_PRESSURE);
        voltage_sum  += halReadADC(DOM_HAL_ADC_5V_POWER_SUPPLY);                        
    }
    
    halDisableBarometer();
    
    mean_pressure = ((float) pressure_sum) / loop_cnt;
    mean_voltage  = ((float) voltage_sum)  / loop_cnt;

    /* This converts the pressure to KPa */
    pressure = (int) ((mean_pressure/mean_voltage + 0.095)/0.009);
    
    return pressure;
}

static const char *memcp(const char *p) {
   const int len = pop();
   const void *mem = (const void *) pop();
   void *ptr = malloc(len);
   memcpy(ptr, mem, len);
   push((int)ptr);
   push(len);
   return p;
}

static const char *crlf(const char *p) { 
   static const char *crlf = "\r\n";
   push((int) crlf);
   push(2);
   return p;
}

static const char *swicmd(const char *p) {
   const int len = pop();
   const char *addr = (const char *)pop();
   write(0x1000, addr, len);
   return p;
}

/* pop string off stack, make it into a C string...
 */
static const char *mkString(void) {
   const int len = pop();
   const char *addr = (const char *)pop();
   char *ret = malloc(len+1);
   memcpy(ret, addr, len);
   ret[len] = 0;
   return ret;
}

static const char *boardID(const char *p) {
   const char *id = halGetBoardID();
   push((int) id);
   push(strlen(id));
   return p;
}

static const char *domID(const char *p) {
   const char *id = halGetBoardID();
   push((int) id);
   push(strlen(id));
   return p;
}

static const char *hvid(const char *p) {
   const char *id = halHVSerial();
   push((int) id);
   push(strlen(id));
   return p;
}

static const char *fbid(const char *p) {
    char **id = 0;
    int err;
    if (hal_FB_isEnabled()) {        
        err = hal_FB_get_serial(id);
        if (err == 0) {
            push((int)(*id));
            push(strlen(*id));
        }
        else {
            switch(err) {
            case FB_HAL_ERR_ID_NOT_PRESENT:
                printf("Error: flasherboard ID chip not detected\r\n");
                break;
            case FB_HAL_ERR_ID_BAD_CRC:
                printf("Error: flasherboard ID CRC failure\r\n");
                break;
            default:
                printf("Error: unknown flasherboard ID failure\r\n");
                break;
            }
        }
    }
    else
        printf("Please power the flasherboard with enableFB first!\r\n");
   return p;
}

static void prtCurrents(int rawCurrents) {
   float currents[5];
   /* volts per count */
   const float vpc = 2.048/1023.0;
   /* voltages */
   const float volts[] = { 5, 3.3, 2.5, 1.8, -5 };
   /*                     5V  3.3V 2.5V 1.8V -5V */
   const float gain[] = { 100, 10, 10, 100, 100 };
   const float reff[] = { 1,   0.98, 0.80, 0.96, 1 };
   const float ueff[] = { 1,     1,    1, 1,   1 };
   const float *eff = (rawCurrents) ? ueff : reff;

   /*                     5V  3.3V 2.5V 1.8V -5V */
   /* 1.8V and 2.5V have been swapped back
    * 
    * FIXME: this order is no longer needed, should
    * we remove it?
    */
   const int   order[] = { 0, 1,   2,   3,    4 };
   int ii;
   
   /* format the currents... */
   for (ii=0; ii<5; ii++) {
      currents[order[ii]] = 
	 eff[ii]*
	 (5/volts[order[ii]])*((readADC(3+ii) * vpc)/gain[ii])*10;
   }

   /* write the string... */
   printf("  5V %.3fA, 3.3V %.3fA, "
	  "2.5V %.3fA, 1.8V %.3fA, -5V %.3fA, %.1f (C)",
	  currents[0], currents[1],
	  currents[2], currents[3], currents[4],
	  formatTemp(readTemp()));
}

static const char *prtRawCurrents(const char *p) {
   prtCurrents(1);
   printf("\r\n");
   return p;
}

static const char *prtCookedCurrents(const char *p) {
   prtCurrents(0);
   printf("\r\n");
   return p;
}

static const char *doFisList(const char *p) {
   fisList();
   return p;
}

static const char *doFisCreate(const char *p) {
   char *s = (char *) mkString();
   int len = pop();
   void *addr = (void *) pop();
   fisCreate(s, addr, len);
   return p;
}

static const char *doFisInit(const char *p) {
   fisInit();
   return p;
}


static const char *doFisRm(const char *p) {
   char *s = (char *) mkString();
   push(fisDelete(s)==0 ? 1 : 0);
   free(s);
   return p;
}

static const char *doFisUnlock(const char *p) {
   char *s = (char *) mkString();
   push(fisUnlock(s)==0 ? 1 : 0);
   free(s);
   return p;
}


static const char *doFisLock(const char *p) {
   char *s = (char *) mkString();
   push(fisLock(s)==0 ? 1 : 0);
   free(s);
   return p;
}

/* find string given...
 *
 * if file exists push(length), push(addr)
 * push existence:
 *   true file exists
 *   false file doesn't exist
 */
static const char *find(const char *p) {
   char *s = (char *) mkString();
   const struct fis_image_desc *img = fisLookup(s);

   if (img==NULL) {
      push(0);
   }
   else {
      push((int) img->flash_base);
      push(img->data_length);
      push(1);
   }
   free(s);
   return p;
}

/* search for and source startup.fs if it exists...
 */
static void sourceStartup(void) {
   const struct fis_image_desc *img = fisLookup("startup.fs");
   if (img==NULL) return;
   interpret((int) img->flash_base, img->data_length);
}

/* length address on stack (dropped)
 * length address pushed on stack...
 */
static const char *zcompress(const char *p) {
   const Bytef *src = (const Byte *) pop();
   uLong srcLen = pop();
   Bytef *dest = (Bytef *)malloc(12 + (int) (1.011*srcLen));
   uLongf destLen;

   compress(dest, &destLen, src, srcLen);

   if (destLen>0) {
      Bytef *ret = (Bytef *) malloc(destLen);
      /* FIXME: prepend header..
       */
      memcpy(ret, dest, destLen);

      /* FIXME: append crc32, isize...
       */
      push(destLen);
      push((int) ret);
   }
   else {
      push(0);
      push(0);
   }
   free(dest);
   return p;
}

static int uncomp(Bytef *dest, uLongf *destLen, 
		  const Bytef *source, uLong sourceLen) {
    z_stream stream;
    int err;

    stream.next_in = (Bytef*)source;
    stream.avail_in = (uInt)sourceLen;
    /* Check for source > 64K on 16-bit machine: */
    if ((uLong)stream.avail_in != sourceLen) return Z_BUF_ERROR;

    stream.next_out = dest;
    stream.avail_out = (uInt)*destLen;
    if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;

    /* send -MAX_WBITS to indicate no zlib header...
     */
    err = inflateInit2(&stream, -MAX_WBITS);
    if (err != Z_OK) return err;
    
    err = inflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        inflateEnd(&stream);

	/* hmmm, sometimes we get an error, even though
	 * the data are ok...
	 *
	 * FIXME: track this down...
	 */
	if (stream.total_out==*destLen) return Z_OK;

        return err == Z_OK ? Z_BUF_ERROR : err;
    }
    *destLen = stream.total_out;


    err = inflateEnd(&stream);
    return err;
}


/* length address on stack (dropped)...
 * length address status pushed on stack...
 */
static const char *gunzip(const char *p) {
   uLong srcLen = pop();
   const Bytef *src = (const Byte *) pop();

   /* uncompress a gzip file...
    */
   if (src[0]==0x1f && src[1]==0x8b && src[2]==8) {
      Bytef *dest;
      uLongf destLen;
      int idx = 10, dl;
      int ret;
      
      if (src[3]&0x04) {
	 int xlen = src[10] + (((int)src[11])<<8);
	 idx += xlen + 2;
      }
      
      if (src[3]&0x08) {
	 while (src[idx]!=0) idx++;
	 idx++;
      }
      
      if (src[3]&0x10) {
	 while (src[idx]!=0) idx++;
	 idx++;
      }

      if (src[3]&0x02) {
	 idx+=2;
      }

      /* header is now stripped...
       */
      dl = 
	 (int) src[srcLen-4] +
	 ((int) src[srcLen-3]<<8) + 
	 ((int) src[srcLen-2]<<16) +
	 ((int) src[srcLen-1]<<24);
      
      dest = (Bytef *) malloc(dl);
      destLen = dl;
      ret = uncomp(dest, &destLen, src + idx, srcLen - idx - 8);

      if (ret==Z_OK && destLen==dl) {
	 push((int)dest);
	 push(dl);
      }
      else {
	 printf("gunzip: error uncompressing: %d %lu %d\r\n",
		ret, destLen, dl);
	 free(dest);
	 push(0);
	 push(0);
      }
   }
   else {
      printf("gunzip: invalid file header...\r\n");
      push(0);
      push(0);
   }

   return p;
}

static const char *doInstall(const char *p) {
   extern int flashInstall(void);
   flashInstall();
   return p;
}

static const char *pldVersions(const char *p) {
   /* print out the versioning info...
    */
   printf("version      %d [%d]\r\n", halGetHWVersion(), halGetVersion());
   printf("build number %d [%d]\r\n", halGetHWBuild(), halGetBuild());
   printf("matches?     %s\r\n", 
	  halGetVersion()==halGetHWVersion() ? "yes" : "no");
   return p;
}

static const char *fpgaType(DOM_HAL_FPGA_TYPES type) {
   if (type==DOM_HAL_FPGA_TYPE_STF_COM)         { return "stf"; }
   else if (type==DOM_HAL_FPGA_TYPE_DOMAPP)     { return "domapp"; }
   else if (type==DOM_HAL_FPGA_TYPE_CONFIGBOOT) { return "configboot"; }
   else if (type==DOM_HAL_FPGA_TYPE_ICEBOOT)    { return "iceboot"; }
   else if (type==DOM_HAL_FPGA_TYPE_STF_NOCOM)  { return "stf-no-comm"; }
   return "unknown";
}

static const char *fpgaVersions(const char *p) {
   const int type = hal_FPGA_query_type();
#define FPGA_QUERY(a) \
  hal_FPGA_query_component_version(DOM_HAL_FPGA_COMP_##a), \
  hal_FPGA_query_component_expected(DOM_HAL_FPGA_TYPE_STF_COM, \
    DOM_HAL_FPGA_COMP_##a)

   /* print out the versioning info...
    */
   printf("fpga type    %s\r\n", fpgaType(type));
   printf("build number %d\r\n", hal_FPGA_query_build());
   printf("matches?     %s\r\n", 
	  hal_FPGA_query_versions(DOM_HAL_FPGA_TYPE_STF_COM,
				  DOM_HAL_FPGA_COMP_ALL)?"no":"yes");
   printf("versions:\r\n");
   printf("  com_fifo           %d [%d]\r\n", FPGA_QUERY(COM_FIFO));
   printf("  com_dp             %d [%d]\r\n", FPGA_QUERY(COM_DP));
   printf("  daq                %d [%d]\r\n", FPGA_QUERY(DAQ));
   printf("  pulsers            %d [%d]\r\n", FPGA_QUERY(PULSERS));
   printf("  discriminator_rate %d [%d]\r\n", FPGA_QUERY(DISCRIMINATOR_RATE));
   printf("  local_coinc        %d [%d]\r\n", FPGA_QUERY(LOCAL_COINC));
   printf("  flasher_board      %d [%d]\r\n", FPGA_QUERY(FLASHER_BOARD));
   printf("  trigger            %d [%d]\r\n", FPGA_QUERY(TRIGGER));
   printf("  local_clock        %d [%d]\r\n", FPGA_QUERY(LOCAL_CLOCK));
   printf("  supernova          %d [%d]\r\n", FPGA_QUERY(SUPERNOVA));

#undef FPGA_QUERY

   return p;
}

/* we do our own buffering for stdout...
 */
static int  soBufLen = 0;
static char soBuf[256];

static void soPutm(const char *s, const int len) {
   if (soBufLen+len>=sizeof(soBuf)-1) {
      soBufLen=0;
      longjmp(jenv, EXC_LINE_TOO_LONG);
   }
   memcpy(soBuf + soBufLen, s, len);
   soBufLen += len;
}
static void soPuts(const char *s) { soPutm(s, strlen(s)); }
static void soPutc(char c) { soPutm(&c, 1); }
static void soFlush(void) {
   if (soBufLen>0) write(1, soBuf, soBufLen);
   soBufLen = 0;
}

static const char *errmsg = NULL;

static void setErr(const char *msg) {
   if (errmsg!=NULL) {
      free((char *)errmsg);
      errmsg=NULL;
   }
   errmsg = strdup(msg);
}

static void prtErr(int err) {
   if (err==EXC_STACK_UNDERFLOW) {
      printf("Error: stack underflow");
   }
   else if (err==EXC_STACK_OVERFLOW) {
      printf("Error: stack overflow");
   }
   else if (err==EXC_LINE_TOO_LONG) {
      printf("Error: line too long");
   }
   else if (err==EXC_UNKNOWN_WORD) {
      printf("Error: unknown word");
   }
   else {
      printf("Error: unknown");
   }

   /* print extra info... */
   if (errmsg!=NULL) {
      printf(": '%s'", errmsg);
      free((char *)errmsg);
      errmsg=NULL;
   }
   printf("\r\n");
   fflush(stdout);
}

static int isInputData(void) { return halIsInputData(); }

#define PSKHACK
#undef PSKHACK

#if defined(PSKHACK)

/* HACK!!! -- don't forget to take these lines out!!! */
#define CORDICBITS 15
#include "../../crdc.c"

/* collect a bunch of angles...
 */
static const char *scanAngles(const char *p) {
   const int nAngles = pop();
   short *mem = (short *) 0x01000000;
   int i;
   #define PSKSTAT ( * (unsigned volatile *) 0x9008100c )
   #define PSKSIN  ( * (unsigned volatile *) 0x90081010 )
   #define PSKCOS  ( * (unsigned volatile *) 0x90081014 )

   /* HACK: make sure table is computed... */
   (void) getPhase(0, 0);
   
   for (i=0; i<nAngles; i++, mem++) {
      /* wait for new phase... */
      while ( ((PSKSTAT ^ PSKSTAT)&1) == 0) ;
      *mem = getPhase(PSKSIN, PSKCOS);
   }

   #undef PSKSTAT
   #undef PSKSIN
   #undef PSKCOS
   return p;
}

static const char *dumpCordic(const char *p) {
   int i;
   short *mem = (short *) 0x01000000;
   const int pattern = pop();
   const int ntries = pop();
   unsigned lastStat;

   /* step size is in 2*pi*maxint ... */
   #define PSKSTAT   ( * (unsigned volatile *) 0x9008100c )
   #define PSKCORDIC ( * (unsigned volatile *) 0x90081020 )

   /* HACK: make sure table is computed... */
   (void) getPhase(0, 0);

   lastStat = PSKSTAT;
   for (i=0; i<ntries; i++, mem++) {

      if (i==4) (* (unsigned volatile *) 0x90081038) = pattern;
      
      /* wait for new phase... */
      while ( ((lastStat ^ PSKSTAT)&1) == 0) ;

      /* mark our new phase... */
      lastStat = PSKSTAT;

      /* wait 1us for cordic to be ready... */
      halUSleep(1);

      /* get the cordic value */
      *mem = PSKCORDIC;
   }
   #undef PSKSTAT
   #undef PSKCORDIC

   return p;
}

static const char *dumpIQ(const char *p) {
   int i;
   unsigned *mem = (unsigned *) 0x01000000;
   const int ntries = pop();
   unsigned lastStat;

   /* step size is in 2*pi*maxint ... */
   #define PSKSTAT   ( * (unsigned volatile *) 0x9008100c )
   #define PSKI      ( * (unsigned volatile *) 0x90081010 )
   #define PSKQ      ( * (unsigned volatile *) 0x90081014 )

   /* HACK: make sure table is computed... */
   (void) getPhase(0, 0);

   lastStat = PSKSTAT;
   for (i=0; i<ntries; i++, mem++) {
      /* wait for new phase... */
      while ( ((lastStat ^ PSKSTAT)&1) == 0) ;

      /* mark our new phase... */
      lastStat = PSKSTAT;

      /* wait 1us for cordic to be ready... */
      halUSleep(1);

      /* get the cordic value */
      *mem = PSKI; mem++;
      *mem = PSKQ;
   }
   #undef PSKSTAT
   #undef PSKI
   #undef PSKQ

   return p;
}

static inline void setRxOsc(unsigned char phase, unsigned short rate) {
   *(unsigned volatile *)0x90081008 = 1|(phase<<8)|(rate<<16);
}

static inline void setTxOsc(unsigned short gain, unsigned short rate) {
   *(unsigned volatile *)0x90081000 = 1|(gain<<8)|(rate<<16);
}

static void pskLock(int channel) {
   unsigned lastStat;
   int nok = 0;
   short phase = 0;
   short minorAdjust = 0;
   unsigned short rate = 1<<(channel+4);
   static int offset = 0; /* HACK: cycle offsets */
   short offsetMask = (3>>(2-channel));
   
   /* step size is in 2*pi*maxint ... */
   #define PSKSTAT   ( * (unsigned volatile *) 0x9008100c )
   #define PSKCORDIC ( * (unsigned volatile *) 0x90081020 )

   /* HACK: cycle offset gets updated... */
   offset++; offset|=offsetMask;

   /* setup the oscillator */
   setRxOsc( (offset<<8)|phase, rate);

   /* HACK: make sure table is computed... */
   (void) getPhase(0, 0);

   lastStat = PSKSTAT;
   while (nok<8) {
      /* wait for new cycle... */
      while ( ((lastStat ^ PSKSTAT)&1) == 0) ;

      /* mark our new phase... */
      lastStat = PSKSTAT;

      /* wait 1us for cordic to be ready... */
      halUSleep(1);

      /* is it ok? */
      {  const short angle = PSKCORDIC;
         const int angleThreshold = 4; /* 1 part in 64 (too loose?) */
         const int correction = angle>>8;
         const int ok = abs(correction) < angleThreshold;

#if 0
         printf("angle=%hd correction=%hd minor=%hd phase=%hd offset=%hd\r\n", 
                angle, correction, minorAdjust, phase, offset);
#endif

         if (ok) {
            nok++; 
            minorAdjust += correction;
         }
         else {
            nok=0;
            minorAdjust = 0;
            
            /* adjust angle -- use only top 8 bits of angle... */
            phase += correction;
            setRxOsc((offset<<8)|phase, rate);
               
            /* wait for another cycle */
            while ( ((lastStat ^ PSKSTAT)&1) == 0) ;

            /* mark our new phase... */
            lastStat = PSKSTAT;
         }
      }
   }

   /* make minor adjustment... */
   phase += minorAdjust>>3;
   setRxOsc((offset<<8)|phase, rate);

#if 0
   printf("final phase: %hd\r\n", phase);
#endif

   #undef PSKSTAT
   #undef PSKCORDIC
}

static const char *doPskLock(const char *p) {
   pskLock(pop());
   return p;
}

#if 0
/* sync to channel ch -- assume a 10101010 pattern
 * already established on ... */
static void pskSync(int channel) {
   unsigned lastStat;
   int nok = 0;
   short phase = 0;
   short minorAdjust = 0;
   unsigned short rate = 1<<(channel+4);

   /* step size is in 2*pi*maxint ... */
   #define PSKSTAT   ( * (unsigned volatile *) 0x9008100c )
   #define PSKCORDIC ( * (unsigned volatile *) 0x90081020 )

   /* setup the oscillator */
   setRxOsc(phase, rate);

   /* HACK: make sure table is computed... */
   (void) getPhase(0, 0);

   lastStat = PSKSTAT;
   while (nok<8) {
      /* wait for new cycle... */
      while ( ((lastStat ^ PSKSTAT)&1) == 0) ;

      /* mark our new phase... */
      lastStat = PSKSTAT;

      /* wait 1us for cordic to be ready... */
      halUSleep(1);

      /* is it ok? */
      {  const short angle = PSKCORDIC;
         const int angleThreshold = 8; /* 1 part in 32 (too loose) */
         const int correction = angle>>8;
         const int ok = abs(correction) < angleThreshold;

         printf("angle=%hd correction=%hd minor=%hd phase=%hd\r\n", 
                angle, correction, minorAdjust, phase);

         if (ok) {
            nok++; 
            minorAdjust += correction;
         }
         else {
            nok=0;
            minorAdjust = 0;
   
            /* adjust angle -- use only top 8 bits of angle... */
            phase += correction;
            setRxOsc(phase, rate);
               
            /* wait for another cycle */
            while ( ((lastStat ^ PSKSTAT)&1) == 0) ;

            /* mark our new phase... */
            lastStat = PSKSTAT;
         }
      }
   }

   /* make minor adjustment... */
   phase += minorAdjust>>3;
   setRxOsc(phase, rate);
   printf("final phase: %hd\r\n", phase);

   #undef PSKSTAT
   #undef PSKCORDIC
}
#endif

/* go into psk echo mode -- never comes out... */
static int pskEchoMode(int ich, int och, int npackets) {
   #define PSKSTAT   ( * (unsigned volatile *) 0x9008100c )
   #define PSKRX     ( * (unsigned volatile *) 0x90081024 )
   #define PSKTX     ( * (unsigned volatile *) 0x90081038 )

   int lastSend = 0;

   /* phase lock */
   printf("echo mode phase lock...\r\n");
   pskLock(0);

   printf("emit carrier...\r\n");

#if 0   
   /* wait for sync bits msg */

   
   {  unsigned lastStat = PSKSTAT & 2;
      int i;
   
      for (i=0; i<npackets; i++) {
         /* wait for something in rx register... */
         printf("wait tx...\r\n");
         while ( ((lastStat ^ PSKSTAT)&2) == 0) ;

         /* mark the new status */
         lastStat = PSKSTAT;

         /* wait for next time slot... */
         while (lastSend<20*40) {
            halUSleep(1);
            lastSend++;
         }
         
         /* copy word... */
         PSKTX = PSKRX;
         lastSend = 0;
         
         printf("lock...\r\n");
         pskLock(ich);

         halUSleep(1);
         lastSend++;
      }
   }
#endif

   return npackets;

   #undef PSKSTAT
   #undef PSKRX
   #undef PSKTX
}

/* start an echo test... */
static int pskEchoTest(int ich, int channel, int npackets) {
   #define PSKSTAT   ( * (unsigned volatile *) 0x9008100c )
   #define PSKRX     ( * (unsigned volatile *) 0x90081024 )
   #define PSKTX     ( * (unsigned volatile *) 0x90081038 )
   #define PSKTXSTAT ( * (unsigned volatile *) 0x90081004 )

   unsigned queue[8];
   const unsigned nq = sizeof(queue)/sizeof(queue[0]);
   unsigned qh=0, qt=0;
   int nerrs = 0;
   int i;
   unsigned rn = 0;
   int us = 0;
   unsigned short rate = 1<<(channel+4);

   /* setup tx osc... */
   setTxOsc(4, rate);

   /* lock to input... */
   pskLock(ich);
   
   {  unsigned lastStat = PSKSTAT & 2;
      int nr = 0, nw = 0;
      unsigned lastBusy = PSKTXSTAT;
      
      while (nr<npackets) {

         if (us>1000*3) {
            printf("psk-echo-test: timeout!\r\n");
            printf(" nr=%d nw=%d nerrs=%d\r\n", nr, nw, nerrs);
            printf(" qh=%u qt=%u nq=%u\r\n", qh, qt, nq);
            printf(" us=%u\r\n", us);
            printf(" PSKSTAT=%08x PSKTXSTAT=%08x\r\n", PSKSTAT, PSKTXSTAT);
            return nerrs;
         }
         
         if (lastBusy ^ PSKTXSTAT) {
            unsigned newBusy = PSKTXSTAT;
            printf("tx busy change: %d -> %d\n", lastBusy, newBusy);
            lastBusy = newBusy;
         }

         /* anything read? ... */
         if ( ((lastStat ^ PSKSTAT)&2) == 0) {
            /* no, stuff another packet out if there is room... */
            if (qh-qt<nq && nw<npackets && us>1000) {
               queue[qh%nq] = rn = rn*69069+1;
               qh++;
               PSKTX = rn;
               lastBusy = PSKTXSTAT;
               us = 0;
               nw++;
               halUSleep(1000);
#if 0
            }
         }
         else {
#endif
            /* mark the new status */
            lastStat = PSKSTAT;

            /* mark packet read... */
            nr++;

            /* check the value returned... */
            if (qh!=qt) {
               unsigned v = queue[qt%nq];
               const unsigned rx = PSKRX;
               
               qt++;
               if (v!=rx) {
                  printf("error: expected=%08x read=%08x [%08x]\r\n",
                         v, rx, v^rx);
                  nerrs++;
               }
            }
            else {
               printf("error: received a packet, yet no packet was sent!\r\n");
               nerrs++;
            }
         }
#if 1 
      }
#endif

         halUSleep(1);
         us++;
      }
   }
   
   return nerrs;

   #undef PSKSTAT
   #undef PSKRX
   #undef PSKTX
   #undef PSKTXSTAT
}

#endif

static const char *doDumpFlash(const char *p) {
   osDumpFlash();
   return p;
}

int main(int argc, char *argv[]) {
  char *line;
  const int linesz = 256;
  static int rawCurrents = 1;
  static int disableCurrents = 1;

  static struct {
    const char *nm;
    CFunc cf;
  } initCFuncs[] = {
     /* don't forget to document these commands
      * at the top of this file!
      */
     { ".", pTop },
     { ".s", pStack },
     { "c!", poke8 },
     { "w!", poke16 },
     { "!", poke32 },
     { "c@", peek8 },
     { "w@", peek16 },
     { "@", peek32 },
     { ":", colon },
     { "if", iffunc },
     { "words", words },
     { "constant", constant },
     { "\\", comment },
     { "drop", drop },
     { "dup", sdup },
     { "ymodem1k", ymodem1k },
     { "allocate", memallocate },
     { "free", memfree },
     { "icopy", icopy },
     { "iset", iset },
     { "?DO", doloop },
     { "writeDAC", writeDAC },
     { "usleep", udelay },
     { "od", odump },
     { "odt", odumpt },
     { "reboot", rebootDOM },
     { "md5sum", md5sum },
     { "boot-serial", serialReboot },
     { "boot-flash", flashReboot },
     { "analogMuxInput", analogMuxInput },
     { "prtTemp", prtTemp },
     { "swicmd", swicmd },
     { "s\"", squote },
     { "type", type },
     { "crlf", crlf },
     { "boardid", boardID },
     { "domid", domID },
     { "hvid", hvid },
     { "prtRawCurrents", prtRawCurrents },
     { "prtCookedCurrents", prtCookedCurrents },
     { "invalidateICache", invalidateICache },
     { "exec", execBin },
     { "find", find },
     { "compress", zcompress },
     { "gunzip", gunzip },
     { "ls",  doFisList },
     { "rm", doFisRm },
     { "lock", doFisLock },
     { "unlock", doFisUnlock },
     { "init", doFisInit },
     { "create", doFisCreate },
     { "writeActiveBaseDAC", writeActiveBaseDAC },
     { "writePassiveBaseDAC", writePassiveBaseDAC },
     { "rcv", rcvMsg },
     { "send", sendMsg },
     { "enableHV", enableHV },
     { "disableHV", disableHV },
     { "cp", memcp },
     { "install", doInstall },
     { "fpga-versions", fpgaVersions },
     { "pld-versions", pldVersions },
     { "boot-req", reqBoot },
     { "begin", begin },
     { "acq-forced" , acq_cpu },
     { "acq-disc" , acq_disc },
     { "acq-pp" , acq_pp },
     { "acq-led", acq_led },
     { "zd"	, zdump },
     { "enableLED", enableLED },
     { "disableLED", disableLED },
     { "setLEDdelay", setLEDdelay },
     { "enableFB", enableFB},
     { "enableFBmin", enableFBmin},
     { "disableFB", disableFB },
     { "fbid", fbid },
     { "setFBbrightness", setFBbrightness },
     { "setFBwidth", setFBwidth },
     { "setFBenables", setFBenables },
     { "setFBmux", setFBmux },
     { "startFBflashing", startFBflashing },
     { "stopFBflashing", stopFBflashing },
     { "setFBdcdc", setFBdcdc },
     { "setFBrate", setFBrate },
#if defined(PSKHACK)
     { "scan-angles", scanAngles },
     { "dump-cordic", dumpCordic },
     { "dump-iq", dumpIQ },
     { "psk-lock", doPskLock },
#endif
     { "dump-flash", doDumpFlash },
  };
  const int nInitCFuncs = sizeof(initCFuncs)/sizeof(initCFuncs[0]);

  static struct {
     const char *nm;
     CFunc0 cf;
  } initCFuncs0[] = {
     /* don't forget to document these commands
      * at the top of this file!
      */
     { "readTemp", readTemp },
     { "i", loopCount },
     { "readBaseDAC", readBaseDAC },
     { "readBaseADC", readBaseADC },         
     { "readPressure", readPressure },
     { "getFBfw", getFBfw },
     { "getFBhw", getFBhw },
     { "getFBdcdc", getFBdcdc },
  };
  const int nInitCFuncs0 = sizeof(initCFuncs0)/sizeof(initCFuncs0[0]);

  static struct {
     const char *nm;
     CFunc1 cf;
  } initCFuncs1[] = {
     /* don't forget to document these commands
      * at the top of this file!
      */
     { "readADC", readADC },
     { "readDAC", readDAC }, 
     { "not", not },
 };
  const int nInitCFuncs1 = sizeof(initCFuncs1)/sizeof(initCFuncs1[0]);

  static struct {
    const char *nm;
    CFunc2 cf;
  } initCFuncs2[] = {
     /* don't forget to document these commands
      * at the top of this file!
      */
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
    { "fpga", do_fpga_config },
    { "interpret", interpret },
    { "fb-cpld", do_fb_cpld_config },
  };
  const int nInitCFuncs2 = sizeof(initCFuncs2)/sizeof(initCFuncs2[0]);

#if defined(PSKHACK)
  static struct {
    const char *nm;
    CFunc3 cf;
  } initCFuncs3[] = {
     /* don't forget to document these commands
      * at the top of this file!
      */
#if defined(PSKHACK)
    { "psk-echo-mode", pskEchoMode },
    { "psk-echo-test", pskEchoTest },
#endif
  };
  const int nInitCFuncs3 = sizeof(initCFuncs3)/sizeof(initCFuncs3[0]);
#endif

  static struct {
    const char *nm;
    int constant;
  } initConstants[] = {
     /* don't forget to document these commands
      * at the top of this file!
      */
     { "true", -1 },
     { "false", 0 },
     { "base", (int) &numberBase },
     { "blperr", (int) &blperr },
     { "rawCurrents", (int) &rawCurrents },
     { "disableCurrents", (int) &disableCurrents },
  };
  const int nInitConstants = sizeof(initConstants)/sizeof(initConstants[0]);

  int li = 0, ei = 0, i;
  extern void initMemTests(void);
  char *lines[32];
  int hl = 0, bl = -1;
  const int nl = sizeof(lines)/sizeof(lines[0]);
  int escaped = 0;
  int err;

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

  for (i=0; i<nInitCFuncs0; i++)
     addCFunc0Bucket(initCFuncs0[i].nm, initCFuncs0[i].cf);
  
  for (i=0; i<nInitCFuncs1; i++)
     addCFunc1Bucket(initCFuncs1[i].nm, initCFuncs1[i].cf);
  
  for (i=0; i<nInitCFuncs2; i++) 
    addCFunc2Bucket(initCFuncs2[i].nm, initCFuncs2[i].cf);

#if defined(PSKHACK)
  for (i=0; i<nInitCFuncs3; i++) 
    addCFunc3Bucket(initCFuncs3[i].nm, initCFuncs3[i].cf);
#endif

  for (i=0; i<nInitConstants; i++) 
    addConstantBucket(initConstants[i].nm, initConstants[i].constant);

  /* os specific initializations... */
  osInit(argc, argv);

  initMemTests();

  if ((err=setjmp(jenv))!=0) {
     printf("In startup.fs: ");
     prtErr(err);
  }
  else sourceStartup();

  printf(" Iceboot (%s) build %s.....\r\n",  versions.tag, versions.build);

  line = (char *) memalloc(linesz);

  if ((err=setjmp(jenv))!=0) {
     prtErr(err);
     /* reset parsed line... */
     escaped = 0;
     ei = 0;
     li = 0;
  }

  soPuts("\r\n> ");
  while (1) {
    /* check for input, if there is none some
     * amount of time (500ms?) then update the currents
     * on the top bar...
     */
    if (!disableCurrents) {
       while (1) {
	  int ii;
	  
	  for (ii=0; ii<500; ii++) {
	     halUSleep(1000);
	     if (isInputData()) break;
	  }
	  
	  /* jump out if there is data... */
	  if (isInputData()) break;
	  
	  /* save the current cursor location, set reverse video, cursor
	   * to upper-left... */
	  printf("%c7%c[7m%c[H", 27, 27, 27);
	  
	  /* write the string... */
	  if (!isInputData()) prtCurrents(rawCurrents);
	  
	  /* set normal video, reset the current cursor location */
	  printf("%c[m%c8", 27, 27); 
	  fflush(stdout);
       }
    }

    /* always flush before reading... */
    soFlush();
    {  const int buflen = 4096;
       char buf[buflen];
       int nr = read(0, buf, sizeof(buf));
       int idx;

       if (nr<=0) {
	  printf("!!!! can't read from stdin!!!!\r\n");
	  continue;
       }

       for (idx=0; idx<nr; idx++) {
	  char c = buf[idx];
	  
#if 0
	  printf("\r\n[%d] 0x%02x\r\n", escaped, c);
#endif
	  
	  /* FIXME: why do we get a '0' on epxa4??? */
	  if (c == 0) continue;
    
	  if (escaped) {
	     char mv[8];
	     int lineflip = 0;
	     
	     if (c=='A') {
		/* up arrow... */
		if (bl<hl-1 && bl<nl) {
		   bl++;
		   lineflip = 1;
		}
	     }
	     else if (c=='B') {
		/* down... */
		if (bl>0) {
		   bl--;
		   lineflip = 1;
		}
	     }
	     else if (c=='C') {
		/* right... */
		if (li<ei) {
		   sprintf(mv, "%c[C", 27);
		   soPuts(mv);
		   li++;
		}
	     }
	     else if (c=='D') {
		/* left... */
		if (li>0) {
		   sprintf(mv, "%c[D", 27);
		   soPuts(mv);
		   li--;
		}
	     }
	     else if (c=='O') {
		escaped = 1;
		continue;
	     }
	     else if (c=='[') {
		escaped = 1;
		continue;
	     }
	     
	     if (lineflip) {
		const int idx = (hl - bl - 1)%nl;
		char cmd[8];
		
		/* move to beginning of line...
		 */
		sprintf(cmd, "%c[D", 27);
		while (li>0) {
		   soPuts(cmd);
		   li--;
		}
		
		/* clear to end of line
		 */
		sprintf(cmd, "%c[K", 27);
		soPuts(cmd);
		
		/* add the line...
		 */
		strcpy(line, lines[idx]);
		li = strlen(line);
		ei = strlen(line);
		
		/* write the line...
		 */
		soPuts(line);
	     }
	     
	     escaped = 0;
	     continue;
	  }
	  
	  if (c==27) { 
	     /* esc character...
	      */
	     escaped = 1;
	  }
	  else if (c==0x06) {
	     int snw = 0;
	     char mv[16];
	     
	     /* ^F forward word: skip whitespace until non-whitespace skip
	      * non whitespace until space or end of line
	      */
	     while (1) {
		if (li<ei) {
		   if (!snw) snw = !isspace(line[li]);
		   sprintf(mv, "%c[C", 27);
		   soPuts(mv);
		   li++;
		   
		   if (snw && li<ei && isspace(line[li])) break;
		}
		else break;
	     }
	  }
	  else if (c==0x02) {
	     int snw = 0;
	     char mv[16];
	     
	     /* ^B back word: skip whitespace backwards, 
	      * skip non-whitespace backwards until whitespace 
	      * is previous
	      */
	     while (1) {
		if (li>0) {
		   sprintf(mv, "%c[D", 27);
		   soPuts(mv);
		   li--;
		   
		   if (!snw) snw = !isspace(line[li]);
		   
		   if (snw && li>0 && isspace(line[li-1])) break;
		}
		else break;
	     }
	  }
	  else if (c=='\b' || c==0x7f) {
	     if (li>0) {
		char cmd[8];

		sprintf(cmd, "%c%c[K", '\b', 27);
		soPuts(cmd);
		
		if (li<ei) {
		   /* we need to write the rest...
		    */
		   sprintf(cmd, "%c7", 27);
		   soPuts(cmd);
		   soPutm(line+li, (ei-li));
		   sprintf(cmd, "%c8", 27);
		   soPuts(cmd);
		   memmove(line+li-1, line+li, ei-li);
		}
		li--;
		ei--;
	     }
	  }
	  else if (c=='\r') {
	     /* we need to flush on '\r' so things
	      * come out in the right order...
	      */
	     soPuts("\r\n");
	     soFlush();

	     line[ei] = 0;

	     /* before we parse the line, we save it in a ring buffer...
	      */
	     if (ei>0) {
		if (hl>=nl) free(lines[hl%nl]);
		
		lines[hl%nl] = strdup(line);
		hl++;
		
		if (newLine(line)) {
		   printf("!!! error new line\r\n");
		}
	     }
	     
	     bl = -1;
	     li = ei = 0;
	     soPuts("> ");
	  }
	  else {
	     /* make space for character...
	      */
	     if (li<ei) memmove(line+li+1, line+li, ei-li);
	     
	     /* insert character...
	      */
	     line[li] = c;
	     
	     soPutc(c);
	     
	     ei++;
	     li++;
	     
	     if (li<ei) {
		char ec[4];
		
		/* we need to write the rest...
		 */
		sprintf(ec, "%c7", 27);
		soPuts(ec);
		soPutm(line+li, (ei-li));
		sprintf(ec, "%c8", 27);
		soPuts(ec);
	     }
	  }
       }
       soFlush();
    }
  }
  return 0;
}
  
static const char *udelay(const char *p) {
   halUSleep(pop());
   return p;
}

void *memalloc(size_t sz) { return malloc(sz); }

static void initMemTests(void) {
  addConstantBucket("memtest-silent", (int) &silent_test);
  addCFunc3Bucket("memtest-random-value", (CFunc3) test_random_value);
  addCFunc3Bucket("memtest-xor-comparison", (CFunc3) test_xor_comparison);
  addCFunc3Bucket("memtest-sub-comparison", (CFunc3) test_sub_comparison);
  addCFunc3Bucket("memtest-mul-comparison", (CFunc3) test_mul_comparison);
  addCFunc3Bucket("memtest-div-comparison", (CFunc3) test_div_comparison);
  addCFunc3Bucket("memtest-or-comparison", (CFunc3) test_or_comparison);
  addCFunc3Bucket("memtest-and-comparison", (CFunc3) test_and_comparison);
  addCFunc3Bucket("memtest-seqinc-comparison", 
		  (CFunc3) test_seqinc_comparison);
  addCFunc3Bucket("memtest-solidbits-comparison", 
		  (CFunc3) test_solidbits_comparison);
  addCFunc3Bucket("memtest-checkerboard-comparison", 
		  (CFunc3) test_checkerboard_comparison);
  addCFunc3Bucket("memtest-blockseq-comparison", 
		  (CFunc3) test_blockseq_comparison);
  addCFunc3Bucket("memtest-walkbits-comparison", 
		  (CFunc3) test_walkbits_comparison);
  addCFunc3Bucket("memtest-bitspread-comparison", 
		  (CFunc3) test_bitspread_comparison);
  addCFunc3Bucket("memtest-bitflip-comparison", 
		  (CFunc3) test_bitflip_comparison);
  addCFunc3Bucket("memtest-stuck-address", (CFunc3) test_stuck_address);
  addCFunc3Bucket("memtest-thorsten0f", (CFunc3) test_thorsten0f);
  addCFunc3Bucket("memtest-thorsten16", (CFunc3) test_thorsten16);
}

static const char *writeDAC(const char *p) {
   int value = pop();
   int channel = pop();
   halWriteDAC(channel, value);
   return p;
}
