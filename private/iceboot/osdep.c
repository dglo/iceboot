#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>

#include "osdep.h"
#include "hal/DOM_MB_hal.h"
#include "iceboot/flash.h"

#define MAX_CLI_ARGS 6
#define MAX_FILENAME_LEN 32
#define FLASH_FILE_SYS_PATH "./"
#define TEMP_EXECUTION_FILENAME ".localExectuionImage.exe"

/* signature of iceboot simulation routine */
extern int iceboot(int , char *[]);

/* global storage */
char *invoking_argv[MAX_CLI_ARGS];
int invoking_argc;
char lastUsedFileName[MAX_FILENAME_LEN];
int lastUsedFileNameHash;
char flashFileSysPath[]=FLASH_FILE_SYS_PATH;

void icacheInvalidateAll(void) {
}

int fpga_config(int *addr, int nbytes) {
    return 0;
}

/* execute an image ...
 */
int executeImage(const void *addr, int nbytes) {
   /* hmmm, copy image to /tmp, exec it?
    */
   char path[128];
   int fd;
   int sts;

   if (nbytes<0) {
      printf("execute image: invalid nbytes (%d)\r\n", nbytes);
      return 1;
   }

    if(((int)addr == lastUsedFileNameHash) &&
	(nbytes == 0)) {
	strcpy(path, flashFileSysPath);
	strcat(path, lastUsedFileName);
    }
    else {
	strcpy(path, flashFileSysPath);
	strcat(path, TEMP_EXECUTION_FILENAME);

        if ((fd=open(path, O_RDWR|O_CREAT|O_TRUNC, 0600))<0) {
            return 1;
        }
   
        if (write(fd, addr, nbytes)<0) {
            return 1;
        }
        close(fd);
    } 
    /* now exec...
    */
    sts = execv(path, &invoking_argv[1]);

    /* should never return -- how do we remove exec? */
    return 0;
}

/* FIXME: before we dump, make a copy and
 * fixup all the addresses so they map to 0x41000000
 * when this is done, we can remove the address in
 * mmap() in flashdrv.c (get_flash_limits())
 */
static void dumpflash(int sig) {
   int fd;
   const char* dumpfile = getenv("FLASH_DUMP_FILE");
   if (0 == dumpfile) {
     dumpfile = "flash.dump";
   }
   fd = open(dumpfile, O_WRONLY|O_CREAT|O_TRUNC, 0644);
   if (fd>=0) {
      void *flash_start, *flash_end;
      flash_get_limits(NULL, &flash_start, &flash_end);
      write(fd, flash_start, (char *) flash_end - (char *) flash_start);
      close(fd);
   }
}

int osInit(int argc, char *argv[]) {
    int i;

    /* keep copy of argv for use with do_go */
    invoking_argc = argc;
    if(argc > MAX_CLI_ARGS) {
	invoking_argc = MAX_CLI_ARGS;
    }

    i=invoking_argc;
    for (i = 0; i < argc; i++) {
	invoking_argv[i]=(char *)malloc(strlen(argv[i]));
	strcpy(invoking_argv[i], argv[i]);
    }

    if (argc >= 3) {
  	/* insert ID and name into hal simulation for later use.
   	ID length is important, so make sure its 6 chars */

  	if(strlen(argv[1])!=12) {
    	halSetBoardID("123456123456");
  	} else {
    	    halSetBoardID(argv[1]);
  	}
  	halSetBoardName(argv[2]);
    } else {
  	/* must have id and name to run, make something up */
  	halSetBoardID("000000000000");
  	halSetBoardName("none");
    }

    /* do this for simboot IO consistency */
    setvbuf(stdout, (char *)0, _IONBF, 0);

    signal(SIGUSR1, dumpflash);

    return 0;
}


/* wait for input data approx ms milliseconds...
 */
int waitInputData(int ms) {
   struct pollfd fds[1];
   fds[0].fd = 0;
   fds[0].events = POLLIN;
   return poll(fds, 1, ms)==1;
}

void dcacheInvalidateAll(void) {
}
