#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>


#include <unistd.h>

#include "iceboot/fis.h"
#include "iceboot/flash.h"

#define tmalloc(t) ((t *) malloc(sizeof(t)))
#define smalloc(s) ((struct s *) malloc(sizeof(struct s)))

static int hashFileName(const char *nm, int bound);

extern char flashFileSysPath[];

extern char lastUsedFileName[];
extern int lastUsedFileNameHash;

static struct fis_image_desc img;
static char imgName[32];

static int fisBlockSize(void) {
   int block_size;
   flash_get_block_info(&block_size);
   return block_size;
}

static int fisDirSize(void) { return fisBlockSize(); }

static struct fis_image_desc *fisAddr(void) {
   unsigned flash_end;
   int block_size = fisDirSize();

   flash_get_limits(NULL, NULL, (void **) &flash_end);
   return (struct fis_image_desc *)(flash_end - block_size);
}

void fisList(void) {
   int i;
   int block_size = fisDirSize();
   struct fis_image_desc *img = fisAddr();
   DIR *dirp;
   struct dirent *dp;
   unsigned long dummy_flash_base;


   printf("%-16s  %-10s  %-10s  %-s\r\n",
	  "Name","FLASH addr",
	  "Length",
	  "Data Length" );

    /* open the directory */
    dirp = opendir(flashFileSysPath);
    dummy_flash_base = 0;

    /* get directory contents */
    while ((dp = readdir(dirp)) != NULL) {
	if(dp->d_name[0] != '.') {
            printf("%-16s  0x%08lX  0x%08lX  0x%08lX\r\n", 
	        dp->d_name, 
                dummy_flash_base, 
                0x0, 0x0);
	    /* bump flash_base to dummy up differnet flash indicies */
	    dummy_flash_base++;
	}
    }
    closedir(dirp);
}

const struct fis_image_desc *fisLookup(const char *name) {
    FILE *fd;
    char path[128];
   
    /* find the file */
    strcpy(path,flashFileSysPath);
    strcat(path,name);

    fd = fopen(path,"r");
    if (fd == NULL) {
        return (struct fis_image_desc *)0;
    }
    fclose(fd);

    strcpy(imgName,name);
    img.flash_base = (void *)hashFileName(name,1);
    img.mem_base=img.flash_base;
    img.size=0;
    img.entry_point = 0;
    img.desc_cksum = 0;
    img.file_cksum = 0;
    
    /* store copy of file name and hash in case next command is an exec */
    strcpy(lastUsedFileName, name);
    lastUsedFileNameHash=hashFileName(name,1);
    return &img;
}

static int fisNumReserved(void) {
   return 2;
}


/* delete a file...
 */
int fisDelete(const char *name) {
    char path[128];
    FILE *fd;
      
    /* see if it is a reserved file */
    if (name[0] == '.') {
        printf("Sorry, '%s' is a reserved image and cannot be deleted\r\n",
	    name);
        return 1;
    }

    /* find the file to delete */
    strcpy(path,flashFileSysPath);
    strcat(path,name);

    fd = fopen(path,"r");
    if (fd == NULL) {
       printf("No image '%s' found\r\n", name);
       return 1;
    }
    fclose(fd);

    if(strcmp(name,"iceboot") || strcmp(name,"iceboot_config")) {
      printf("Sorry, '%s' is a reserved image and cannot be deleted...\r\n",
	     name);
      return 1;
    }

    if(remove(path) < 0) {
      printf("Error in deleting image %s\r\n", path);
      return 1;
    }

    return 0;
}

int fisUnlock(const char *name) {
   const struct fis_image_desc *img = fisLookup(name);
   void *err_addr;
   int stat;
   
   if (img==NULL) {
      printf("can't find image '%s'\r\n", name);
      return 1;
   }

   return 0;
}

int fisLock(const char *name) {
   const struct fis_image_desc *img = fisLookup(name);
   void *err_addr;
   int stat;

   if (img==NULL) {
      printf("can't find image '%s'\r\n", name);
      return 1;
   }

   return 0;
}

static int cmpents(const void *e1, const void *e2) {
   const int *i1 = (const int *) e1;
   const int *i2 = (const int *) e2;
   if (*i1<*i2) return -1;
   if (*i1>*i2) return 1;
   return 0;
}

static int hashFileName(const char *nm, int bound) {
   int h, i;
   const int n = strlen(nm);
   h = 0;
   for (i=n-1; i>=0; i--)  h = (nm[i] + h*37) % bound;
   return h;
}

static int prevEntIdx(unsigned *ents, int ndirents, int bloc) {
   /* FIXME -- this should be a binary search... */
   int i=0;
   for (i=0; i<ndirents*2; i+=2) {
      if (ents[i]>bloc) 
	 return i/2 - 1;
   }
   return ndirents - 1;
}

static int numReservedBlocks(void) { return 7; }

/* create a new file with name, using data from addr and len
 *
 * we'll use przbylsky block allocation (pba) in an attempt
 * to be compatible with the fis filesystem (no virtual mappings)
 * and still to get some wear levelling:
 *
 *   1) hash file name and start looking upwards
 *      for a block until we find one (wrap around
 *      is ok...
 *
 *   2) if we can't find space, we defragment.
 *      we pick the "emptier" half of the
 *      space and we move the "fuller" half
 *      into it, from the top down or the
 *      bottom up.  files too big to fit anywhere
 *      in the top, we just lv alone...
 */
int fisCreate(const char *name, void *addr, int len) {
   int fd;
   char path[128];

   strcpy(path, flashFileSysPath);
   strcat(path, name);
   if ((fd=open(path, O_RDWR|O_CREAT|O_TRUNC, 0600))<0) {
      return 1;
   }
   if (write(fd, addr, len) < 0) {
      return 1;
   }
   close(fd);

   return 0;
}

/* initialize fis filesystem...
 */
int fisInit(void) {
   char dataString[128];
   char path[128];
   int fd;

   /* verify...
    */
   while (1) { 
      char c;
      int nr;
      
      printf("fis init: all data on flash will be lost: are you sure [y/n]? ");
      fflush(stdout);

      nr = read(0, &c, 1);

      if (nr==1) {
	 if (toupper(c)=='Y') break;
	 else if (toupper(c)=='N') { return 1; }
	 write(1, &c, 1);
      }
   }

   /* unlock all data */
   printf("unlock... "); fflush(stdout);
   
   /* erase all data -- except for iceboot...
    */
   printf("erase... "); fflush(stdout);

   /* lock all data -- except for iceboot...
    */
   printf("lock... "); fflush(stdout);

   /* setup directory -- with iceboot...
    */
   printf("init directory... "); fflush(stdout);
   
   strcpy(path, flashFileSysPath);
   strcat(path, "iceboot");
   if ((fd=open(path, O_RDWR|O_CREAT|O_TRUNC, 0600))<0) {
      return 1;
   }
   strcpy(dataString, "iceboot flash memory init file");
   if (write(fd, dataString, strlen(dataString))<0) {
     return 1;
   }
   close(fd);

   strcpy(path, flashFileSysPath);
   strcat(path, "iceboot_config");

   if ((fd=open(path, O_RDWR|O_CREAT|O_TRUNC, 0600))<0) {
     return 1;
   }
   strcpy(dataString, "iceboot flash memory config file");
   if (write(fd, dataString, strlen(dataString))<0) {
     return 1;
   }
   close(fd);

   printf("done...\r\n");
   fflush(stdout);
   
   return 0;
}

