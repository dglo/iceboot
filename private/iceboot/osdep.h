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

/* wait for input data, but only for 100-200ms...
 *
 * return non-zero if data are avail...
 */
int waitInputData(int ms);


int osInit(int argc, char *argv[]);

int fpga_config(int *addr, int nbytes);

/* invalidate the dcache... */
void dcacheInvalidateAll(void);



#endif
