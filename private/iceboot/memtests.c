/*
 * Very simple (yet, for some reason, very effective) memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <memtest@discworld.dyndns.org>
 *
 * This file contains the functions for the actual tests, called from the
 * main routine in memtest.c.  See other comments in that file.
 *
 */
#include <stdio.h>

#include "iceboot/memtests.h"

#define RAND32 0x16383245

/* Data. */
int silent_test = 0;

/* Function definitions. */
int test_verify_success (ui32 *bp1, ui32 *bp2, ui32 count) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 i;
  
  for (i = 0; i < count; i++, p1++, p2++) {
    if (*p1 != *p2) {
      printf("FAILURE: 0x%08x != 0x%08x at offset 0x%08x.\r\n",
	       *p1, *p2, i);
      return ERROR;
    }
  }
  return OK;
}

int test_random_value(ui32 *bp1, ui32 *bp2, ui32 count) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 i;
  
  silent_test = 0;		
  for (i = 0; i < count; i++) {
    *p1++ = *p2++ = RAND32;
  }
  return test_verify_success (bp1, bp2, count);
}

int test_xor_comparison (ui32 *bp1, ui32 *bp2, ui32 count) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 i;
  ui32 q = RAND32;
  
  silent_test = 0;		
  for (i = 0; i < count; i++)
    {
      *p1++ ^= q;
      *p2++ ^= q;
    }
  return (test_verify_success (bp1, bp2, count));
}

int test_sub_comparison (ui32 *bp1, ui32 *bp2, ui32 count) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 i;
  ui32 q = RAND32;
  
  silent_test = 0;		
  for (i = 0; i < count; i++)
    {
      *p1++ -= q;
      *p2++ -= q;
    }
  return (test_verify_success (bp1, bp2, count));
}

int test_mul_comparison (ui32 *bp1, ui32 *bp2, ui32 count) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 i;
  ui32 q = RAND32;
  
  silent_test = 0;		
  for (i = 0; i < count; i++)
    {
      *p1++ *= q;
      *p2++ *= q;
    }
  return (test_verify_success (bp1, bp2, count));
}

int test_div_comparison (ui32 *bp1, ui32 *bp2, ui32 count) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 i;
  ui32 q = RAND32;
  
  silent_test = 0;		
  for (i = 0; i < count; i++) {
    if (!q)
      q++;
    *p1++ /= q;
    *p2++ /= q;
  }
  return (test_verify_success (bp1, bp2, count));
}


int test_or_comparison (ui32 *bp1, ui32 *bp2, ui32 count) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 i;
  ui32 q = RAND32;
  
  silent_test = 0;		
  for (i = 0; i < count; i++)
    {
      *p1++ |= q;
      *p2++ |= q;
    }
  return (test_verify_success (bp1, bp2, count));
}


int test_and_comparison (ui32 *bp1, ui32 *bp2, ui32 count) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 i;
  ui32 q = RAND32;
  
  silent_test = 0;		
  for (i = 0; i < count; i++)
    {
      *p1++ &= q;
      *p2++ &= q;
    }
  return (test_verify_success (bp1, bp2, count));
}


int test_seqinc_comparison (ui32 *bp1, ui32 *bp2, ui32 count) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 i;
  ui32 q = RAND32;
  
  silent_test = 0;		
  for (i = 0; i < count; i++)
    {
      *p1++ = *p2++ = (i + q);
    }
  return (test_verify_success (bp1, bp2, count));
}

int test_solidbits_comparison (ui32 *bp1, ui32 *bp2, ui32 count) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 q, i, j;
  
  silent_test = 1;
  for (j = 0; j < 64; j++)
    {
      q = (j % 2) == 0 ? 0xFFFFFFFF : 0x00000000;
      p1 = (volatile ui32 *) bp1;
      p2 = (volatile ui32 *) bp2;
      for (i = 0; i < count; i++)
	{
	  *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
	}
      
      if (test_verify_success (bp1, bp2, count) == ERROR)
	{
	  return (ERROR);
	}
    }
  return  OK;
}


int test_checkerboard_comparison (ui32 *bp1, ui32 *bp2, ui32 count) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 q, i, j;
  
  silent_test = 1;		
  for (j = 0; j < 64; j++)
    {
      q = (j % 2) == 0 ? 0x55555555 : 0xAAAAAAAA;
      p1 = (volatile ui32 *) bp1;
      p2 = (volatile ui32 *) bp2;
      for (i = 0; i < count; i++)
	{
	  *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
	}
      
      if (test_verify_success (bp1, bp2, count) == ERROR)
	{
	  return (ERROR);
	}
    }
  return (OK);
}

int test_blockseq_comparison (ui32 *bp1, ui32 *bp2, ui32 count) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 i, j;
  
  silent_test = 1;		
  for (j = 0; j < 256; j++) {
    p1 = (volatile ui32 *) bp1;
    p2 = (volatile ui32 *) bp2;
    for (i = 0; i < count; i++)
      {
	*p1++ = *p2++ = (int) (j);
      }
    
    if (test_verify_success (bp1, bp2, count) == ERROR)
      {
	return (ERROR);
      }
  }
  return (OK);
}


int test_walkbits_comparison (ui32 *bp1, ui32 *bp2, ui32 count, int m) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 i, j;
  
  silent_test = 1;		
  for (j = 0; j < 64; j++)
    {
      p1 = (volatile ui32 *) bp1;
      p2 = (volatile ui32 *) bp2;
      for (i = 0; i < count; i++)
	{
	  if (j < 32)	/* Walk it up. */
	    {
	      *p1++ = *p2++ = (m == 0) ? 0x00000001 << j :
		0xFFFFFFFF ^ (0x00000001 << j);
	    }
	  else		/* Walk it back down. */
	    {
	      *p1++ = *p2++ = (m == 0)
		? 0x00000001 << (64 - j - 1)
		: 0xFFFFFFFF ^ (0x00000001 << (64 - j - 1));
	    }				
	}
      
      if (test_verify_success (bp1, bp2, count) == ERROR)
	{
	  return (ERROR);
	}
    }
  return (OK);
}


int test_bitspread_comparison (ui32 *bp1, ui32 *bp2, ui32 count) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 i, j;
  
  silent_test = 1;		
  for (j = 0; j < 64; j++)
    {
      p1 = (volatile ui32 *) bp1;
      p2 = (volatile ui32 *) bp2;
      for (i = 0; i < count; i++)
	{
	  if (j < 32)	/* Walk it up. */
	    {
	      *p1++ = *p2++ = (i % 2 == 0)
		? (0x00000001 << j) | (0x00000001 << (j + 2))
		: 0xFFFFFFFF ^ ((0x00000001 << j)
				| (0x00000001 << (j + 2)));
	    }
	  else		/* Walk it back down. */
	    {
	      *p1++ = *p2++ = (i % 2 == 0)
		? (0x00000001 << (63 - j)) | (0x00000001 << (65 - j))
		: 0xFFFFFFFF ^ (0x00000001 << (63 - j) 
				| (0x00000001 << (65 - j)));
	    }				
	}
      
      if (test_verify_success (bp1, bp2, count) == ERROR)
	{
	  return (ERROR);
	}
    }
  return (OK);
}

int test_bitflip_comparison (ui32 *bp1, ui32 *bp2, ui32 count) {
  volatile ui32 *p1 = (volatile ui32 *) bp1;
  volatile ui32 *p2 = (volatile ui32 *) bp2;
  ui32 i, j, k;
  ui32 q;
  
  silent_test = 1;		
  for (k = 0; k < 32; k++)
    {
      q = 0x00000001 << k;
      
      for (j = 0; j < 8; j++)
	{
	  q = ~q;
	  p1 = (volatile ui32 *) bp1;
	  p2 = (volatile ui32 *) bp2;
	  for (i = 0; i < count; i++) {
	      *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
	  }
	  
	  if (test_verify_success (bp1, bp2, count) == ERROR) {
	    return (ERROR);
	  }
	}
    }
  return (OK);
}

int test_stuck_address (ui32 *bp1, ui32 *unused, ui32 count) {
  volatile ui32 *p1;
  /* second argument is not used; just gives it a compatible signature. */
  ui32 i, j;
  int err = OK;

  for (j = 0; j < 16; j++)
    {
      p1 = (volatile ui32 *) bp1;
      for (i = 0; i < count; i++, p1++)
	{
	  *p1 = ((j + i) % 2) == 0 ? (ui32) p1 : ~((ui32) p1);
	}

      p1 = (volatile ui32 *) bp1;
      for (i = 0; i < count; i++, p1++)
	{
	  if (*p1 != (((j + i) % 2) == 0 ? (ui32) p1 : ~((ui32) p1)))
	    {
	      printf("FAILURE: offset: "
		     " %d (rep %d). [%08x should be: %08x]\r\n", i, j,
		     *p1,
		     (((j + i) % 2) == 0 ? (ui32) p1 : ~((ui32) p1)));
	      err = ERROR;
	    }
	}
    }
  return err;
}

int test_thorsten0f(ui32 *bp, ui32 *unused, ui32 count) {
   ui32 i;
   ui32 *save = bp;
   int err = 0;
   
   for (i=0; i<count; i++, bp++) {
      const unsigned addr = (unsigned) bp;
      *bp = (addr>>5)&1 ? 0xffffffff : 0;
   }
   
   bp = save;
   for (i=0; i<count; i++, bp++) {
      const unsigned addr = (unsigned) bp;
      const ui32 ev = (addr>>5)&1 ? 0xffffffff : 0;
      if ( *bp != ev) {
	 printf("FAILURE: offset %d: expected: %08x, got: %08x\r\n",
		i, ev, *bp);
	 err = ERROR;
      }
   }

   return err;
}

int test_thorsten16(ui32 *bp, ui32 *unused, ui32 count) {
   ui32 i;
   ui32 *save = bp;
   int err = 0;
   
   for (i=0; i<count; i++, bp++) {
      const unsigned addr = (unsigned) bp;
      *bp = ((addr>>5)&1 ? 0xffffffff : 0)&0xffff;
   }
   
   bp = save;
   for (i=0; i<count; i++, bp++) {
      const unsigned addr = (unsigned) bp;
      const ui32 ev = ((addr>>5)&1 ? 0xffffffff : 0)&0xffff;
      if ( *bp != ev) {
	 printf("FAILURE: offset %d: expected: %08x, got: %08x\r\n",
		i, ev, *bp);
	 err = ERROR;
      }
   }

   return err;
}

