#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>

#include "iceboot/fis.h"
#include "iceboot/flash.h"

#define smalloc(s) ((struct s *) malloc(sizeof(struct s)))

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

   printf("%-16s  %-10s  %-10s  %-s\r\n",
	  "Name","FLASH addr",
	  "Length",
	  "Data Length" );

   for (i = 0;  i < block_size/sizeof(*img);  i++, img++) {
      if (img->name[0] != (unsigned char)0xFF) {
         printf("%-16s  0x%08lX  0x%08lX  0x%08lX\r\n", 
                img->name,
                (unsigned long) img->flash_base, 
                img->size, 
                (unsigned long) img->data_length);
      }
   }
}

const struct fis_image_desc *fisLookup(const char *name) {
   int i;
   struct fis_image_desc *img;
   int block_size = fisDirSize();
   
   img = (struct fis_image_desc *)fisAddr();
   for (i = 0;  i < block_size/sizeof(*img);  i++, img++) {
      if ((img->name[0] != (unsigned char)0xFF) && 
	  (strcmp(name, img->name) == 0)) {
	 return img;
      }
   }
   return (struct fis_image_desc *)0;
}

/* update a directory entry -- current image is img, new image is val...
 */
static int updateDirectory(const struct fis_image_desc *img,
			   struct fis_image_desc *val) {
   struct fis_image_desc *dir = (struct fis_image_desc *)fisAddr();
   void *fis_addr = dir;
   const int block_size = fisDirSize();
   const int idx = img - dir;
   struct fis_image_desc *t = (struct fis_image_desc *) malloc(block_size);
   int err = 0;
   int stat;
   void *err_addr;

   if (t==NULL) {
      printf("unable to malloc block_size %d!\r\n", block_size);
      return 1;
   }

   /* copy directory block...
    */
   memcpy(t, dir, block_size);

   /* copy new value to put in there...
    */
   memcpy(t + idx, val, sizeof(struct fis_image_desc));
   
   /* Insure [quietly] that the directory is unlocked 
    * before trying to update
    */
   flash_unlock(fis_addr, block_size);

   if ((stat = flash_erase(fis_addr, block_size))) {
      printf("Error erasing at %p: %s\r\n", err_addr, flash_errmsg(stat));
      err = 1;
   }
   else {
      /* Now program it
       */
      if ((stat = flash_write(fis_addr, t, block_size))) {
	 printf("Error programming at %p: %s\r\n", 
		err_addr, flash_errmsg(stat));
	 err = 1;
      }
   }
   
   /* Insure [quietly] that the directory is locked after the update
    */
   flash_lock((void *)fis_addr, block_size);
   free(t);
   return err;
}

static int fisNumReserved(void) {
   return 2;
}


/* delete a file...
 */
int fisDelete(const char *name) {
   int stat;
   const struct fis_image_desc *img = fisLookup(name);
   struct fis_image_desc *dir = (struct fis_image_desc *)fisAddr();
   const int num_reserved = fisNumReserved();
   int err = 0;
      
   if (img==NULL) {
      printf("No image '%s' found\r\n", name);
      return 1;
   }
   if (img-dir<num_reserved) {
      printf("Sorry, '%s' is a reserved image and cannot be deleted...\r\n",
	     name);
      return 1;
   }

   /* Erase Data blocks (free space)
    */
   if ((stat = flash_erase((void *)img->flash_base, img->size))) {
      return 1;
   }
   else {
      struct fis_image_desc *t = (struct fis_image_desc *)
	 malloc(sizeof(struct fis_image_desc));

      if (t==NULL) {
	 printf("fis delete: unable to malloc %lu bytes\r\n", 
		(long)sizeof(struct fis_image_desc));
	 return 1;
      }
      
      memset(t, 0xff, sizeof(struct fis_image_desc));

      err = updateDirectory(img, t);
      
      free(t);
      
      return err;
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

   if ((stat=flash_unlock((void *)img->flash_base, img->size))) {
      printf("Error unlocking at %p: %s\r\n", err_addr, flash_errmsg(stat));
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

   if ((stat=flash_lock((void *)img->flash_base, img->size))) {
      printf("Error locking at %p: %s\r\n", err_addr, flash_errmsg(stat));
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

/* combine adjacent entries in flash maps...
 */
static int combents(unsigned *ents, int nents) {
   int i;
   
   for (i=0; i<nents*2-2; i+=2) {
      if (ents[i+2]==ents[i] + ents[i+1]) {
	 int j;
	 
	 ents[i+1] += ents[i+3];
	 for (j=i+2; j<nents*2; j+=2) {
	    ents[j] = ents[j+2];
	    ents[j+1] = ents[j+3];
	 }
	 nents--;
      }
   }

   return nents;
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
 *
 *   3) files may _not_ span more than one chip.
 */
int fisCreate(const char *name, void *addr, int len) {
   const struct fis_image_desc *img = fisLookup(name);
   int i;
   int old_size = 0;
   void *old_base;
   const int block_size = fisDirSize();
   const int nblocks = (len + block_size - 1) / block_size;
   const int ndirs = block_size/sizeof(*img);
   int bloc = -1;
   struct fis_image_desc *nimg;

   /* can't be bigger than 4MB (one chip)...
    */
   if (len<=0 || len>=4*1024*1024 - block_size) {
      printf("fis create: invalid file size (%d), must be >0 and <4M\r\n",
	     len);
      return 1;
   }

   if (img==NULL) {
      /* find an empty slot...
       */
      img = fisAddr();
      for (i=0;  i<ndirs;  i++, img++) if (img->name[0] == 0xff) break;

      if (i==ndirs) {
	 printf("fis create: no empty slots!\r\n");
	 return 1;
      }
   }
   else {
      /* img found...
       */
      const int bIdx = img - fisAddr();

      if (bIdx<fisNumReserved()) {
	 /* up to numReserved files get burnt in place...
	  */
	 if (flash_unlock(img->flash_base, len)) {
	    printf("fis create: can't unlock\r\n");
	    return 1;
	 }

	 if (flash_erase(img->flash_base, len)) {
	    printf("fis create: can't erase!\r\n");
	    return 1;
	 }

	 if (flash_write(img->flash_base, addr, len)) {
	    printf("fis create: can't write %p <- %p [%d]\r\n", 
		   img->flash_base, addr, len);
	    return 1;
	 }
	 
	 if (flash_lock(img->flash_base, len)) {
	    printf("fis create: can't lock\r\n");
	    return 1;
	 }

	 return 0;
      }
      else {
	 old_base = img->flash_base;
	 old_size = img->size;
      }
   }
   
   if (nblocks>0) {
      const struct fis_image_desc *dirs = fisAddr();
      unsigned flash_start, flash_end;
      unsigned first_block, last_block;
      int block_size = fisDirSize();
      int ndirents = 0;
      unsigned *ents = (unsigned *) calloc(ndirs*2, sizeof(unsigned));
      int tblocks;
      int idx;
      
      if (ents==NULL) {
	 printf("fis create: unable to malloc %lu bytes!\r\n", 
		(long)ndirs*2*sizeof(unsigned));
	 return 1;
      }
      
      for (i=0; i<ndirs; i++) {
	 if (dirs[i].name[0]!=0xff && dirs[i].size>0) {
	    ents[ndirents*2] = (unsigned) dirs[i].flash_base;
	    ents[ndirents*2+1] = dirs[i].size;
	    ndirents++;
	 }
      }

      qsort(ents, ndirents, 2*sizeof(unsigned), cmpents);
      ndirents = combents(ents, ndirents);

      flash_get_limits(NULL, (void **)&flash_start, (void **)&flash_end);
      first_block = flash_start + numReservedBlocks()*block_size;
      last_block = flash_end - block_size;
      tblocks = (last_block - first_block + block_size - 1)/block_size;

      bloc = (unsigned) (first_block + block_size*hashFileName(name, tblocks));

      if ((idx = prevEntIdx(ents, ndirents, bloc))<0) {
	 printf("corrupt flash filesystem, try init...\r\n");
	 free(ents);
	 return 1;
      }

      /* skip this one...
       */
      if (bloc<ents[idx*2]+ents[idx*2+1]) {
	 bloc = ents[idx*2]+ents[idx*2+1];
      }

      /* bloc is now pointing to free space, see if we can
       * fit it in...
       */
      for (i=0; i<ndirents; i++) {
	 if (idx==ndirents-1) {
	    /* last dirent -- check for free space...
	     */
	    if (last_block - bloc > len) {
	       /* found!!!
		*/
	       break;
	    }
	 }
	 else {
	    if ( (bloc + len < ents[idx*2+2]) && 
		!flash_code_overlaps((void *) bloc, (void *)(bloc+len))) {
	       /* found!!!
		*/
	       break;
	    }
	 }
	 
	 idx = (idx + 1)%ndirents;
	 bloc = ents[idx*2] + ents[idx*2+1];
	 if (bloc>=last_block) bloc = first_block;
      }
      free(ents);
      
      if (i==ndirents) {
	 /* FIXME: if allocation fails here, defragment!!!
	  */
	 printf("fis create: can't find space for '%s'...\r\n", name);
	 return 1;
      }

      /* unlock, burn and lock the data...
       */
      if (flash_unlock((void *)bloc, len)) {
	 printf("fis create: can't unlock\r\n");
	 return 1;
      }

      if (flash_write((void *)bloc, addr, len)) {
	 printf("fis create: can't write: 0x%08x (%d) -> 0x%08x\r\n", 
		(int)addr, len, bloc);
	 return 1;
      }
      
      if (flash_lock((void *)bloc, len)) {
	 printf("fis create: can't lock\r\n");
	 return 1;
      }
   }

   /* update the directory...
    */
   nimg = (struct fis_image_desc *) malloc(sizeof(struct fis_image_desc));

   if (nimg==NULL) {
      printf("fis create: can't allocate %lu bytes\r\n",
	     (long)sizeof(struct fis_image_desc));
      if (bloc!=0) {
	 flash_unlock((void *)bloc, len);
	 flash_erase((void *)bloc, len);
	 flash_lock((void *)bloc, len);
      }
      return 1;
   }
   memset(nimg, 0xff, sizeof(struct fis_image_desc));

   strncpy(nimg->name, name, 16);
   nimg->name[15] = 0;
   nimg->flash_base = (void *)bloc;
   nimg->mem_base = 0;
   nimg->size = ((len + block_size - 1)/block_size)*block_size;
   nimg->entry_point = 0;
   nimg->data_length = len;

   if (updateDirectory(img, nimg)) {
      printf("fis create: can't update directory...\r\n");
      return 1;
   }
      
   /* erase old data...
    */
   if (old_size>0) {
      flash_unlock(old_base, old_size);
      flash_erase(old_base, old_size);
      flash_lock(old_base, old_size);
   }

   return 0;
}

/* initialize fis filesystem...
 */
int fisInit(void) {
   unsigned flash_start, flash_end;
   const int block_size = fisBlockSize();
   struct fis_image_desc *nimg;
   const struct fis_image_desc *dir;
   int stat, err = 0;
   unsigned chip_start[2], chip_end[2];
   int i;

   /* verify...
    */
   while (1) { 
      char c;
      int nr;
      
      printf("fis init: all data on flash will be lost: are you sure [y/n]? ");
      fflush(stdout);

      nr = read(0, &c, 1);

      if (nr==1) {
	 printf("%c\r\n", c); fflush(stdout);
	 if (toupper(c)=='Y') break;
	 else if (toupper(c)=='N') { return 1; }
      }
   }

   flash_get_limits(NULL, (void **)&flash_start, (void **)&flash_end);
   flash_start += numReservedBlocks() * fisBlockSize();

   chip_start[0] = flash_start;
   chip_start[1] = (unsigned) flash_chip_addr(1);
   chip_end[0] = chip_start[1]-1;
   chip_end[1] = flash_end;

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
      
      /* lock all data -- except for iceboot...
       */
      printf("lock... "); fflush(stdout);
      if (flash_lock((void *) chip_start[i], chip_end[i] - chip_start[i])) {
	 printf("unable to lock %08x -> %08x\r\n", 
		chip_start[i], chip_end[i]);
      }

      printf("\r\n");
   }
   
   /* setup directory -- with iceboot...
    */
   printf("setup directory... "); fflush(stdout);
   
   if ((nimg=(struct fis_image_desc *)malloc(block_size))==NULL) {
      printf("\r\nfis init: unable to malloc image descriptor!\r\n");
      return 1;
   }
   memset(nimg, 0xff, block_size);

   dir = fisAddr();
   
   flash_get_limits(NULL, (void **)&flash_start, (void **)&flash_end);

   strncpy(nimg[0].name, "iceboot", 16);
   nimg[0].name[15] = 0;
   nimg[0].flash_base = (void *)flash_start;
   nimg[0].mem_base = 0;
   nimg[0].size = numReservedBlocks()*block_size;
   nimg[0].entry_point = 0;
   nimg[0].data_length = numReservedBlocks()*block_size;
   
   strncpy(nimg[1].name, "iceboot config", 16);
   nimg[1].name[15] = 0;
   nimg[1].flash_base = 0;
   nimg[1].mem_base = 0;
   nimg[1].size = 0;
   nimg[1].entry_point = 0;
   nimg[1].data_length = 0;

   /* Insure [quietly] that the directory is unlocked 
    * before trying to update
    */
   flash_unlock(dir, block_size);

   if ((stat = flash_erase(dir, block_size))) {
      printf("Error erasing: %s\r\n", flash_errmsg(stat));
      err = 1;
   }
   else {
      /* Now program it
       */
      if ((stat = flash_write((void *)dir, nimg, block_size))) {
	 printf("Error programming: %s\r\n", flash_errmsg(stat));
	 err = 1;
      }
   }
   
   /* Insure [quietly] that the directory is locked after the update
    */
   flash_lock(dir, block_size);

   free(nimg);

   printf("done...\r\n");
   fflush(stdout);
   
   return err;
}

