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

/* is there input on stdin?
 */
int isInputData(void);

int osInit(int argc, char *argv[]);

int fpga_config(int *addr, int nbytes);


#endif
