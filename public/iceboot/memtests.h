#ifndef MEMTESTS_HEADER
#define MEMTESTS_HEADER

/*
 * Very simple (yet, for some reason, very effective) memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <memtest@discworld.dyndns.org>
 *
 * This file contains the functions for the actual tests, called from the
 * main routine in memtest.c.  See other comments in that file.
 *
 */
#define OK 0
#define ERROR 1

typedef unsigned int ui32;

/* Data. */
extern int silent_test;

/* Function definitions. */
int test_verify_success (ui32 *bp1, ui32 *bp2, ui32 count);
int test_random_value(ui32 *bp1, ui32 *bp2, ui32 count);
int test_xor_comparison (ui32 *bp1, ui32 *bp2, ui32 count);
int test_sub_comparison (ui32 *bp1, ui32 *bp2, ui32 count);
int test_mul_comparison (ui32 *bp1, ui32 *bp2, ui32 count);
int test_div_comparison (ui32 *bp1, ui32 *bp2, ui32 count);
int test_or_comparison (ui32 *bp1, ui32 *bp2, ui32 count);
int test_and_comparison (ui32 *bp1, ui32 *bp2, ui32 count);
int test_seqinc_comparison (ui32 *bp1, ui32 *bp2, ui32 count);
int test_solidbits_comparison (ui32 *bp1, ui32 *bp2, ui32 count);
int test_checkerboard_comparison (ui32 *bp1, ui32 *bp2, ui32 count);
int test_blockseq_comparison (ui32 *bp1, ui32 *bp2, ui32 count);
int test_walkbits_comparison (ui32 *bp1, ui32 *bp2, ui32 count, int m);
int test_bitspread_comparison (ui32 *bp1, ui32 *bp2, ui32 count);
int test_bitflip_comparison (ui32 *bp1, ui32 *bp2, ui32 count);
int test_stuck_address (ui32 *bp1, ui32 *unused, ui32 count);
int test_thorsten0f(ui32 *bp, ui32 *unused, ui32 count);
int test_thorsten16(ui32 *bp, ui32 *unused, ui32 count);

#endif
