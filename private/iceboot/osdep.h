#ifndef OSDEPENDENCYHEADERS
#define OSDEPENDENCYHEADERS

/* invalidate icache...
 */
void icacheInvalidateAll(void);

/* execute binary image...
 */
int executeImage(const void *addr, int nbytes);

/* FIXME: program fpga, etc...
 */




#endif
