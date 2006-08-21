/* lfis.c, implement fis using a log structured filesystem,
 * based loosely on jffs -- shooting for 603 lines...
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iceboot/fis.h"
#include "iceboot/flash.h"
#include "iceboot/fis.h"

#define smalloc(s) ((struct s *) malloc(sizeof(struct s)))
#define lengthof(a) ( sizeof(a)/sizeof(a[0]) )

static void *blockAddress(void *flash, unsigned block) {
   return (unsigned char *) flash + (block << LFIS_LOG2_BLOCK_SIZE);
}

/* directory structure is mirrored here... */
struct dirent_list {
   struct lfis_dirent *dirent;  /* pointer to flash */
   struct lfis_extent *ext;     /* pointer to flash */
   void *flash;
   struct dirent_list *next;    /* pointer to memory */
};

struct lfis_extent_info {
   void               *flash;  /* flash memory address... */
   struct lfis_extent *ext;
};

struct lfis_inmem {
   unsigned seqn;
   struct lfis_extent_info exts[2];
};

void direntListFree(struct dirent_list *dl) {
   while (dl!=NULL) {
      struct dirent_list *t = dl;
      dl = dl->next;
      free(t);
   }
}

/* FIXME: export crc32 from hal? */
static unsigned crc32Once(unsigned crcval, unsigned char cval) {
   int i;
   const unsigned poly32 = 0x04C11DB7;
   crcval ^= cval << 24;
   for (i = 8; i--; )
      crcval = crcval & 0x80000000 ? (crcval << 1) ^ poly32 : crcval << 1;
   return crcval;
}

static unsigned crc32(const void *p, int n) {
   const unsigned char *b = (const unsigned char *) p;
   unsigned crc = 0;
   int i;
   for (i=0; i<n; i++) crc = crc32Once(crc, b[i]);
   return crc;
}

/* insert nnode just after node... */
static struct dirent_list *direntListInsert(struct dirent_list *node,
                                            struct lfis_dirent *dirent,
                                            struct lfis_extent *extent,
                                            void *flash) {
   struct dirent_list *ret = smalloc(dirent_list);

   if (ret==NULL) return NULL;

   ret->dirent = dirent;
   ret->ext    = extent;
   ret->flash  = flash;

   if (node!=NULL) {
      ret->next = node->next;
      node->next = ret;
   }
   else {
      ret->next = NULL;
   }

   return ret;
}

static unsigned direntListSeqn(struct dirent_list *de) {
   return de->ext->inodes[de->dirent->ino].seqn;
}

static struct dirent_list *direntListFilterDeletes(struct dirent_list *top) {
   struct dirent_list *dl = top, *prev = NULL;
   
   while (dl!=NULL) {
      if (dl->dirent->action == LFIS_ACTION_RM) {
         struct dirent_list *dlt = dl;

         if (prev==NULL) top = dl->next;
         else            prev->next = dl->next;

         dl = dl->next;
         dlt->next = NULL;
         direntListFree(dlt);
      }
      else if (dl->dirent->action == LFIS_ACTION_ADD) {
         prev = dl;
         dl = dl->next;
      }
      else {
         printf("filter deletes: yikes! action=%d\r\n", dl->dirent->action);
         prev = dl;
         dl = dl->next;
      }
   }
   return top;
}

static struct dirent_list *direntListRemove(struct dirent_list *top,
                                            struct lfis_dirent *de,
                                            struct lfis_extent *ext,
                                            void *flash) {
   if (de->ino!=0xffffffff &&
       crc32(de, sizeof(struct lfis_dirent)-4)==de->crc32) {
      struct dirent_list *t, *prev;
      const unsigned seqn = ext->inodes[de->ino].seqn;
      
      for (t=top, prev=NULL; t!=NULL; prev=t, t=t->next) {
         if (strcmp(t->dirent->name, de->name)==0) {
            const unsigned oseqn = t->ext->inodes[t->dirent->ino].seqn;
            
            /* found a match -- check seqn... */
            if (seqn >= oseqn) {
               struct dirent_list *around = t->next;
               
               while (around!=NULL && direntListSeqn(around) == seqn) {
                  struct dirent_list *dlt = around;
                  around = around->next;
                  dlt->next = NULL;
                  direntListFree(dlt);
               }
               
               t->dirent = de;
               t->ext = ext;
               t->flash = flash;
            }
            break;
         }
      }
      
      if (t==NULL) {
         /* not found -- append it... */
         struct dirent_list *dl = direntListInsert(prev, de, ext, flash);
         if (prev==NULL) top = dl;
      }
   }
   return top;
}

/* add a dirent to the list, return new top of list... */
static struct dirent_list *direntListAdd(struct dirent_list *top,
                                         struct lfis_dirent *de,
                                         struct lfis_extent *ext,
                                         void *flash) {
   if (de->ino!=0xffffffff &&
       crc32(de, sizeof(struct lfis_dirent)-4)==de->crc32) {
      struct dirent_list *t, *prev;
      const unsigned seqn = ext->inodes[de->ino].seqn;

      for (t=top, prev=NULL; t!=NULL; prev=t, t=t->next) {
         if (strcmp(t->dirent->name, de->name)==0) {
            const unsigned oseqn = t->ext->inodes[t->dirent->ino].seqn;
            
            /* found a match -- check seqn... */
            if (seqn > oseqn) {
               struct dirent_list *around = t->next;
               
               while (around!=NULL && direntListSeqn(around) == seqn) {
                  struct dirent_list *dlt = around;
                  around = around->next;
                  dlt->next = NULL;
                  direntListFree(dlt);
               }
               
               t->dirent = de;
               t->ext = ext;
               t->flash = flash;
            }
            else if (oseqn == seqn && t->dirent->action == LFIS_ACTION_ADD) {
               struct dirent_list *tn = t->next;

               if (t->next==NULL ||
                   tn->ext->inodes[tn->dirent->ino].seqn!=seqn) {
                  /* insert it... */
                  t = direntListInsert(t, de, ext, flash);
               }
            }
            break;
         }
      }
      
      if (t==NULL) {
         /* not found -- append it... */
         struct dirent_list *dl = direntListInsert(prev, de, ext, flash);
         if (prev==NULL) top = dl;
      }
   }

   return top;
}

/* flash is 64k sector based... */
int flashErase(void *flash, unsigned ssector, unsigned esector) {
   unsigned i;
   for (i=ssector; i<=esector; i++) {
      if (flash_erase(blockAddress(flash, i*LFIS_BLOCKS_PER_SECTOR), 
                      LFIS_SECTOR_SIZE - 1)) {
         return -1;
      }
   }
   
   return 0;
}

int flashUnlock(void *flash, unsigned ssector, unsigned esector) {
   unsigned i;

   for (i=ssector; i<=esector; i++) {
      if (flash_unlock(blockAddress(flash, i*LFIS_BLOCKS_PER_SECTOR), 
                       LFIS_SECTOR_SIZE - 1)) {
         fprintf(stderr, "flash unlock %p sector %u: unable to unlock\r\n",
                 flash, i);
         return -1;
      }
   }
   
   return 0;
}

unsigned flashSectors(void *flash)   { return LFIS_CHIP_SECTORS; }
unsigned flashChips(void)            { return LFIS_CHIPS; }

/* create direntList from an array of dirents... */
static struct dirent_list *direntListCreate(struct lfis_dirent *de,
                                            int n,
                                            struct lfis_extent *ext,
                                            void *flash) {
   int i;
   struct dirent_list *old = NULL;
   for (i=0; i<n; i++) {
      if (de[i].ino < lengthof(ext->inodes)) {
         if (de[i].action == LFIS_ACTION_ADD) {
            old = direntListAdd(old, de + i, ext, flash);
         }
         else if (de[i].action == LFIS_ACTION_RM) {
            old = direntListRemove(old, de + i, ext, flash);
         }
         else {
            printf("dirent list create: yikes!\r\n");
         }
      }
   }
   
   return old;
}

static struct dirent_list *direntListRoot(struct lfis_extent *ext, 
                                          void *flash) {
   return 
      direntListFilterDeletes(direntListCreate(ext->root, 
                                               lengthof(ext->root), 
                                               ext, flash));
}

/* create master dirent list... */
static struct dirent_list *lfisDirentList(struct lfis_inmem *mem) {
   struct dirent_list *root = NULL;
   int i;
   for (i=0; i<lengthof(mem->exts); i++) {
      struct lfis_extent *ext = mem->exts[i].ext;
      void *flash = mem->exts[i].flash;

      if (ext!=NULL) {
         struct dirent_list *dl;
         struct dirent_list *dlsave =
            direntListCreate(ext->root, lengthof(ext->root), ext, flash);
            
         for (dl=dlsave; dl!=NULL; dl=dl->next) {
            root = direntListAdd(root, dl->dirent, ext, flash);
         }
         direntListFree(dlsave);
      }
   }
   return direntListFilterDeletes(root);
}

static void extentInfoInit(struct lfis_extent_info *ei, 
                           struct lfis_extent *ext, void *flash) {
   ei->ext   = ext;
   ei->flash = flash;
}

static int inodeAvailable(const struct lfis_ino *ino) {
   unsigned char *a = (unsigned char *) ino;
   int i;
   for (i=0; i<sizeof(struct lfis_ino); i++) {
      if (a[i]!=0xff) return 0;
   }
   return 1;
}

/* find the extent data structure on the flash, or return -1 if
 * it's not there...
 */
static struct lfis_extent *extentScan(void *flash) {
   struct lfis_extent *ret = NULL;
   unsigned j;
   
   for (j=0; j<LFIS_CHIP_BLOCKS; j+=LFIS_BLOCKS_PER_SECTOR) {
      struct lfis_extent *ext = blockAddress(flash, j);
      if (ext->magic == LFIS_MAGIC_EXT && crc32(ext, 7*4)==ext->crc32) {
         return ext;
      }
   }
   return ret;
}

static unsigned extentLastSeqn(struct lfis_extent *ext) {
   int i;
   unsigned seqn = 0;
   
   if (ext==NULL) return 0;
   
   for (i=0; i<lengthof(ext->inodes); i++) {
      struct lfis_ino *ino = ext->inodes + i;
   
      if (ino->block!=0xffffffff && 
          ino->seqn > seqn &&
          crc32(ino, sizeof(struct lfis_ino)-4)==ino->crc32) {
         seqn = ino->seqn;
      }
   }

   return seqn;
}

/* get the lfis in memory structure... */
static struct lfis_inmem *lfis_mem = NULL;
static struct lfis_inmem *lfisInMem(void) {
   if (lfis_mem==NULL) {
      struct lfis_inmem *m = smalloc(lfis_inmem);
      int i;

      if (m==NULL) return NULL;
      
      memset(m, 0, sizeof(m));

      /* set flash address... */
      flash_get_limits(NULL, NULL, NULL); /* <- initialize simulator */

      m->seqn = 0;
      for (i=0; i<lengthof(m->exts); i++) {
         void *flash = flash_chip_addr(i);
         unsigned nseqn;
         
         extentInfoInit(m->exts + i, extentScan(flash), flash);
         nseqn = extentLastSeqn(m->exts[i].ext);
         if (nseqn>m->seqn) m->seqn = nseqn;
      }
      
      /* set pointer... */
      lfis_mem=m;
   }
   return lfis_mem;
}

/* add extent data structure to flash, return the sector that
 * we used or -1 on error.  ext is an in-memory copy of the
 * extent...
 */
static unsigned extentAddExtent(void *flash, struct lfis_extent *ext) {
   const unsigned sector = ext->sblock/LFIS_BLOCKS_PER_SECTOR + 7;
   void *addr = blockAddress(flash, sector*LFIS_BLOCKS_PER_SECTOR);
   
   if (flash_write(addr, ext, 8*4)) {
      flashErase(flash, sector, sector);
      return (unsigned) -1;
   }

   return sector;
}

/* allocate nbytes worth of blocks from extent ei.  do _not_
 * start gc if we're full -- rather return -1...
 */
static unsigned extentAllocBlocks(struct lfis_extent *ext, int nbytes) {
   const int nblocks = (nbytes + LFIS_BLOCK_SIZE - 1) / LFIS_BLOCK_SIZE;
   const unsigned extblocks = ext->eblock - ext->sblock + 1;
   char *blks;
   unsigned i;

   if (nblocks<=0 || (blks=(char *) malloc(extblocks))==NULL) return -1;

   memset(blks, 0, extblocks);
   for (i=0; i<lengthof(ext->inodes); i++) {
      if (crc32(ext->inodes + i, 7*4) == ext->inodes[i].crc32 &&
          ext->inodes[i].block!=(unsigned) -1) {
         const int n = 
            (ext->inodes[i].length + LFIS_BLOCK_SIZE - 1)/LFIS_BLOCK_SIZE;
         memset(blks + ext->inodes[i].block, 1, n);
      }
   }

   while (i + nblocks - 1 < extblocks) {
      if (blks[i]==0) {
         int j = i+1;
         
         while ( j - i < nblocks && blks[j] == 0 ) {
            j++;
         }

         if ( j-i == nblocks ) {
            /* found */
            free(blks);
            return i;
         }
         else {
            printf("skipped: i=%d, j=%d\r\n", i, j);
            i = j;
         }
      }
      else {
         i++;
      }
   }

   free(blks);
   return (unsigned) -1;
}

static void flashUnlockExtent(void *flash, const struct lfis_extent *ext) {
   static const struct lfis_extent *exts[LFIS_CHIPS];
   const unsigned ssector = ext->sblock/LFIS_BLOCKS_PER_SECTOR;
   const unsigned esector = ext->eblock/LFIS_BLOCKS_PER_SECTOR;

   int i;

   for (i=0; i<lengthof(exts); i++) {
      if (ext==exts[i]) return;
   }

   flashUnlock(flash, ssector, esector);
   for (i=0; i<lengthof(exts); i++) {
      if (exts[i]==(void *) 0) exts[i] = flash;
   }
}


/* add an entry, but don't add it to a directory
 *
 * ino is a bit tricky, if we're not fixed, we allocate
 * blocks for the inode, otherwise, we keep the block
 * listed in the ino, we fill in the blk_crc32 and
 * the crc32 as well, but the rest _must_ be filled
 * in...
 */
static int extentAddEntNoDir(struct lfis_extent *ext,
                             void *flash,
                             const struct lfis_ino *ino,
                             const void *data /* maybe NULL */) {
   struct lfis_ino nino = *ino;
   int i = 0;

   flashUnlockExtent(flash, ext);

   if (data!=NULL) nino.blk_crc32 = crc32(data, nino.length);
   
   while (1) {
      /* allocate blocks -- if we're not fixed... */
      if ((nino.mode & LFIS_MODE_FIXED)==0) {
         if (data!=NULL) {
            nino.block = extentAllocBlocks(ext, nino.length);
            if (nino.block==(unsigned) -1) return -1;
         }
      }
      
      /* allocate/write inode to extent */
      nino.crc32 = crc32(&nino, sizeof(nino)-4);
      while (i<lengthof(ext->inodes)) {
         if (inodeAvailable(ext->inodes + i)) {
            if (flash_write(ext->inodes + i, &nino, sizeof(nino))==0) {
               /* found one! */
               break;
            }
         }
         i++;
      }
      
      /* out of inodes? */
      if (i==lengthof(ext->inodes)) return -1;
      
      /* write data */
      if (data == NULL || nino.length==0 || 
          (nino.mode & LFIS_MODE_BAD) != 0 ||
          flash_write(blockAddress(flash, nino.block), data, nino.length)==0) {
         break;
      }
   }
   
   return (i==lengthof(ext->inodes)) ? -1 : i;
}

static int direntEmpty(const struct lfis_dirent *de) {
   return de->action == LFIS_ACTION_EMPTY;
}

/* return entry number or -1 on error... */
static int extentAddDirent(struct lfis_extent *ext,
                           void *flash,
                           const struct lfis_dirent *dep) {
   int i;
   struct lfis_dirent de = *dep;
   
   de.crc32 = crc32(&de, sizeof(de) - 4);

   for (i=0; i<lengthof(ext->root); i++) {
      /* only deletes are allowed to go in the final spot, so
       * that we can guarantee that they make it...
       */
      if (direntEmpty(ext->root + i) &&
          (i<lengthof(ext->root)-1 || de.action==LFIS_ACTION_RM)) {
         if (flash_write(ext->root + i, &de, sizeof(de))==0) {
            return i;
         }
      }
   }

   return -1;
}

/* add an entry to the root directory
 *
 * return inode index, or -1 on error... 
 *
 * seqn is also passed and updated...
 *
 * this is an on-flash routine only, in-memory data is _not_ updated...
 */
static int extentAddEnt(struct lfis_extent *ext,
                        void *flash,
                        const char *nm /* ignored if dir_ino==NULL */,
                        struct lfis_ino *ino /* in-memory */,
                        const void *data /* maybe NULL (length==0) */) {
   int iino;
   int i;
   struct lfis_dirent de;
   
   if ((iino = extentAddEntNoDir(ext, flash, ino, data)) < 0) return -1;
   
   /* find a spot on the root directory... */
   de.action = LFIS_ACTION_ADD;
   de.ino = iino;
   strncpy(de.name, nm, sizeof(de.name)); de.name[sizeof(de.name)-1]=0;

   if ((i=extentAddDirent(ext, flash, &de))<0) return -1;
   
   return iino;
}

/* erase an extent, start with a clean extent...
 *
 * return block of extent...
 */
static unsigned extentClean(void *flash, struct lfis_extent *ext) {
   const unsigned ssector = ext->sblock/LFIS_BLOCKS_PER_SECTOR;
   const unsigned esector = ext->eblock/LFIS_BLOCKS_PER_SECTOR;
   unsigned sector;
   
   /* validate parameters... 
    */
   if ( (ext->sblock % LFIS_BLOCKS_PER_SECTOR) != 0 ||
        ((ext->eblock+1) % LFIS_BLOCKS_PER_SECTOR) != 0) {
      fprintf(stderr, 
              "extent format %p: invalid start/end blocks: %u->%u\r\n", 
              flash, ext->sblock, ext->eblock);
      return -1;
   }
   
   if (ssector >= LFIS_CHIP_SECTORS || esector >= LFIS_CHIP_SECTORS ||
       esector < esector) {
      fprintf(stderr, 
              "extent format %p: invalid start/end sectors: %u->%u\r\n", 
              flash, ssector, esector);
      return -1;
   }

   if (flashUnlock(flash, ssector, esector)) {
      fprintf(stderr, "unable to unlock chip %p sector %u-%u\r\n", 
             flash, ssector, esector);
      return -1;
   }
   
   if (flashErase(flash, ssector, esector)) {
      fprintf(stderr, "unable to erase chip %p sector %u-%u\r\n", 
             flash, ssector, esector);
      return -1;
   }

   if ((sector=extentAddExtent(flash, ext))==(unsigned) -1) {
      fprintf(stderr, "unable to add extent\r\n");
      return -1;
   }

   return sector * LFIS_BLOCKS_PER_SECTOR;
}

static void extentGC(struct lfis_extent *ext, void *flash) {
   /* allocate flash chip in ram... */
   void *rflash = 
      malloc( (ext->eblock - ext->sblock + 1) * LFIS_BLOCK_SIZE);
   struct lfis_extent *rext;
   struct dirent_list *dl, *dlsave, *dlp;

   if (rflash==NULL) return;

   dlsave = direntListRoot(ext, flash);

   /* copy data to ram... */
   for (dl=dlsave; dl!=NULL; dl=dl->next) {
      struct lfis_ino *ino = dl->ext->inodes + dl->dirent->ino;

      if ( (ino->mode & LFIS_MODE_BAD) == 0 ) {
         memcpy(blockAddress(rflash, ino->block),
                blockAddress(flash, ino->block), ino->length);
      }
   }
   direntListFree(dlsave);
   
   /* scan for flash ext... */
   if ((rext = extentScan(rflash))==NULL) {
      free(rflash);
      return;
   }

   /* reload root dir in ram... */
   dlsave = direntListRoot(rext, rflash);
   
   /* validate crcs of non-fixed data... */
   for (dlp=NULL, dl=dlsave; dl!=NULL; dlp=dl, dl=dl->next) {
      struct lfis_ino *ino = dl->ext->inodes + dl->dirent->ino;

      if ( (ino->mode & LFIS_MODE_FIXED) == 0 && ino->length>0) {
         const unsigned crc = 
            crc32(blockAddress(rflash, ino->block), ino->length);

         if (crc!=ino->blk_crc32) {
            struct dirent_list *t = dl;
            /* dlp is not null because iceboot is always first... */
            dlp->next = dl->next;
            t->next = NULL;
            direntListFree(t);
            dl = dlp;
         }
      }
   }

   /* erase flash info... */
   ext = blockAddress(flash, extentClean(flash, rext));

   /* copy fixed data first... */
   for (dl=dlsave; dl!=NULL; dl=dl->next) {
      struct lfis_ino *ino = dl->ext->inodes + dl->dirent->ino;

      if (ino->mode & LFIS_MODE_FIXED) {
         void *data;

         if (ino->mode & (LFIS_MODE_BAD|LFIS_MODE_EXTENT)) {
            data = NULL;
         }
         else {
            data = blockAddress(rflash, ino->block);
         }
         
         extentAddEnt(ext, flash, dl->dirent->name, ino, data);
      }
   }

   /* copy variable data next... */
   for (dl=dlsave; dl!=NULL; dl=dl->next) {
      struct lfis_ino *ino = dl->ext->inodes + dl->dirent->ino;

      if ((ino->mode & LFIS_MODE_FIXED)==0) {
         void *data = blockAddress(rflash, ino->block);

         extentAddEnt(ext, flash, dl->dirent->name, ino, data);
      }
   }

   /* all done, clean up... */
   direntListFree(dlsave);
   free(rflash);
}

/* can not fail... */
static void extentRemoveEnt(struct lfis_extent *ext,
                            void *flash,
                            unsigned iino,
                            const char *nm /* ignored if dir_ino==NULL */) {
   struct lfis_dirent de;
   int i;
   
   de.action = LFIS_ACTION_RM;
   strncpy(de.name, nm, sizeof(de.name)); de.name[sizeof(de.name)-1]=0;
   de.ino = iino;

   if ((i=extentAddDirent(ext, flash, &de))<0) {
      extentGC(ext, flash);
      (void) extentAddDirent(ext, flash, &de); /* can not fail... */
   }
}

/* initialize an extent from the in-memory copy ext -- the
 * root directory is ino number zero... 
 */
static int extentFormat(void *flash, struct lfis_extent *ext) {
   struct lfis_ino ino;
   unsigned iino;
   unsigned block;
   
   if ((block=extentClean(flash, ext))==(unsigned) -1) {
      fprintf(stderr, "extent format %p: unable to add extent\r\n", flash);
      return -1;
   }

   /* now we use the flash copy... */
   ext = blockAddress(flash, block);

   /* add iceboot... */
   memset(&ino, 0xff, sizeof(ino));
   ino.block  = 0;
   ino.length = 7 * LFIS_SECTOR_SIZE;
   ino.mode   = LFIS_MODE_FIXED | LFIS_MODE_READ;
   ino.seqn = 0;
   iino=extentAddEnt(ext, flash, "iceboot", &ino, NULL);
   if (iino==-1) {
      fprintf(stderr, "extent format %p: unable to add iceboot entry\r\n",
              flash);
      return -1;
   }
   iino++;

   /* add extents file... */
   memset(&ino, 0xff, sizeof(ino));
   ino.block = block;
   ino.length = LFIS_SECTOR_SIZE;
   ino.mode = LFIS_MODE_EXTENT|LFIS_MODE_FIXED;
   ino.seqn = 1;
   iino = extentAddEnt(ext, flash, "extents", &ino, NULL);
   if (iino==-1) {
      fprintf(stderr, "extent format %p: unable to add extents entry\r\n",
              flash);
      return -1;
   }

   return 0;
}

int fisInit(void) {
   struct lfis_inmem *mem = lfisInMem();
   int i;

   /* initialize in-memory copies... */
   for (i=0; i<lengthof(mem->exts); i++) {
      struct lfis_extent ext;

      memset(&ext, 0xff, sizeof(ext));
      ext.magic = LFIS_MAGIC_EXT;
      ext.sblock = 0;
      ext.eblock = LFIS_CHIP_BLOCKS - 1;
      ext.crc32 = crc32(&ext, 7*4);
      if (extentFormat(mem->exts[i].flash, &ext)) {
         fprintf(stderr, "fisInit: unable to format extent %d\r\n", i);
         return -1;
      }
   }

   /* we need to reload the inmem structure... */
   if (lfis_mem!=NULL) free(lfis_mem);
   lfis_mem=NULL;

   return 0;
}

/* find root directory ino, find dir allocation, get dir blocks
 * iterate through them...
 */
void fisList(void) {
   struct lfis_inmem *mem = lfisInMem();

   if (mem!=NULL) {
      struct dirent_list *root = lfisDirentList(mem);
      struct dirent_list *dl;
      unsigned cnt = 0;
      struct lfis_dirent *de = NULL;
      struct lfis_ino *ino = NULL;
      unsigned addr = 0U;

      for (dl=root; dl!=NULL; dl=dl->next) {
         if (de==NULL) {
            de = dl->dirent;
            ino = dl->ext->inodes + de->ino;
            addr = (unsigned) blockAddress(dl->flash, ino->block);
         }

         if (dl->next==NULL ||
             direntListSeqn(dl->next) != direntListSeqn(dl)) {
            printf("%c%c%c%c%c%c%c %2u %8u %4u %8u 0x%08x %s\n", 
                   (ino->mode & LFIS_MODE_EXTENT) ? 'e' : '-',
                   (ino->mode & LFIS_MODE_BAD)    ? 'b' : '-',
                   (ino->mode & LFIS_MODE_FIXED)  ? 'f' : '-',
                   (ino->mode & LFIS_MODE_DIR)    ? 'd' : '-',
                   (ino->mode & LFIS_MODE_READ)   ? 'r' : '-',
                   (ino->mode & LFIS_MODE_WRITE)  ? 'w' : '-',
                   (ino->mode & LFIS_MODE_EXEC)   ? 'x' : '-',
                   cnt + 1, ino->seqn, de->ino, ino->length, addr, de->name);
            cnt = 0;
            de = NULL;
         }
         else {
            /* duplicate... */
            cnt++;
         }
      }
      
      direntListFree(root);
   }
}

/* lookup dirents with name... */
static struct dirent_list *lfisLookup(struct lfis_inmem *mem, const char *nm) {
   struct dirent_list *ret = NULL;
   struct dirent_list *root = lfisDirentList(mem), *dl;
   for (dl = root; dl!=NULL; dl=dl->next) {
      if (strcmp(dl->dirent->name, nm)==0) {
         ret = direntListAdd(ret, dl->dirent, dl->ext, dl->flash);
      }
      else if (ret!=NULL) {
         break;
      }
   }
   direntListFree(root);
   
   return ret;
}

/* find address and length of name:
 *
 *   lookup ino
 */
void *fisLookup(const char *name, unsigned long *data_length) {
   struct lfis_inmem *mem = lfisInMem();
   void *ret = NULL;

   *data_length = 0;

   if (mem!=NULL) {
      struct dirent_list *dl, *dlsave = lfisLookup(mem, name);
      
      for (dl = dlsave; dl!=NULL; dl = dl->next) {
         struct lfis_ino *ino = dl->ext->inodes + dl->dirent->ino;

         if (ino->mode & LFIS_MODE_READ) {
            void *p = blockAddress(dl->flash, ino->block);
            const unsigned crc = crc32(p, ino->length);
            
            if (crc==ino->blk_crc32) {
               *data_length = ino->length;
               ret = blockAddress(dl->flash, ino->block);
               break;
            }
            /* FIXME: should we remove the file?  nah, let gc do it... */
         }
      }
      
      direntListFree(dlsave);
   }

   return ret;
}

int fisDelete(const char *name) {
   struct lfis_inmem *mem = lfisInMem();
   int ret = 1;

   if (mem!=NULL) {
      struct dirent_list *dl, *dlsave = lfisLookup(mem, name);

      /* found it! */
      for (dl=dlsave; dl!=NULL; dl=dl->next) {
         struct lfis_ino *ino = dl->ext->inodes + dl->dirent->ino;
         const unsigned mask = LFIS_MODE_WRITE|LFIS_MODE_READ;

         /* kinda funky semantics, but we're stuck with it for now... */
         if ((ino->mode & mask)==mask) {
            extentRemoveEnt(dl->ext, dl->flash, dl->dirent->ino, name);
            ret = 0;
         }
      }
      direntListFree(dlsave);
   }
   return ret;
}

static int extentAddEntGC(struct lfis_extent *ext, void *flash,
                          const char *name, struct lfis_ino *ino,
                          void *data) {
   int err;
         
   if ((err= extentAddEnt(ext, flash, name, ino, data))<0) {
      extentGC(ext, flash);
      err = extentAddEnt(ext, flash, name, ino, data);
   }
   return err;
}

static int lfisSetModeBits(const char *name, unsigned mode) {
   struct lfis_inmem *mem = lfisInMem();
   int ret = 1;

   if (mem!=NULL) {
      struct dirent_list *dl, *dlsave = lfisLookup(mem, name);

      mem->seqn++;
      for (dl=dlsave; dl!=NULL; dl=dl->next) {
         struct lfis_ino *ino = dl->ext->inodes + dl->dirent->ino;
         struct lfis_ino nino = *ino;

         nino.mode |= mode;
         nino.seqn = mem->seqn;
         if (extentAddEntGC(dl->ext, dl->flash, 
                            dl->dirent->name, &nino, NULL)>=0) {
            ret = 0;
         }
      }
      direntListFree(dlsave);
   }

   return ret;
}

static int lfisClearModeBits(const char *name, unsigned mode) {
   struct lfis_inmem *mem = lfisInMem();
   int ret = 1;

   if (mem!=NULL) {
      struct dirent_list *dl, *dlsave = lfisLookup(mem, name);

      mem->seqn++;
      for (dl=dlsave; dl!=NULL; dl=dl->next) {
         struct lfis_ino *ino = dl->ext->inodes + dl->dirent->ino;
         struct lfis_ino nino = *ino;

         nino.mode &= ~mode;
         nino.seqn = mem->seqn;
         if (extentAddEntGC(dl->ext, dl->flash, 
                            dl->dirent->name, &nino, NULL)>=0) {
            ret = 0;
         }
      }
      direntListFree(dlsave);
   }

   return ret;
}

/* locking is now internal -- by default files are 
 * created read-only, unlock makes them read/write
 */
int fisUnlock(const char *name) {
   return lfisSetModeBits(name, LFIS_MODE_WRITE);
}

int fisLock(const char *name) {
   return lfisClearModeBits(name, LFIS_MODE_WRITE);
}

/* try to write to both extents, if possible...
 *
 * if we already have a fixed entry for name, we need
 * to check len is <= location.length and write len bytes to
 * the fixed location(s)...
 */
int fisCreate(const char *name, void *addr, int len) {
   struct lfis_inmem *mem = lfisInMem();
   int ret = 1;

   if (mem!=NULL) {
      int i;
      struct dirent_list *dlsave = lfisLookup(mem, name), *dl;

      if (dlsave!=NULL &&
          (dlsave->ext->inodes[dlsave->dirent->ino].mode & LFIS_MODE_FIXED)) {
         for (dl=dlsave; dl!=NULL; dl=dl->next) {
            struct lfis_ino *ino = dl->ext->inodes + dl->dirent->ino;
            
            if (ino->length >= len && 
                (ino->block % LFIS_BLOCKS_PER_SECTOR)==0 &&
                (ino->length % LFIS_SECTOR_SIZE)==0) {
               /* erase sectors, and write data... */
               const unsigned ssector = ino->block / LFIS_BLOCKS_PER_SECTOR;
               const unsigned esector = 
                  ssector + (len + LFIS_SECTOR_SIZE - 1)/LFIS_SECTOR_SIZE;
               if (flashUnlock(dl->flash, ssector, esector) ||
                   flashErase(dl->flash, ssector, esector)) {
                  fprintf(stderr, "fis create: unable to erase fixed\r\n");
               }
               else {
                  void *ptr = blockAddress(dl->flash, ino->block);
                  if (flash_write(ptr, addr, len)) {
                     fprintf(stderr, "fis create: unable to write fixed\r\n");
                  }
               }
            }
         }
      }
      else {
         mem->seqn++;
         for (i=0; i<lengthof(mem->exts); i++) {
            struct lfis_extent *ext = mem->exts[i].ext;

            if (ext!=NULL) {
               struct lfis_ino ino;
               ino.block = (unsigned) -1;
               ino.length = len;
               ino.mode = LFIS_MODE_READ;
               ino.seqn = mem->seqn;
               
               if (extentAddEntGC(mem->exts[i].ext, mem->exts[i].flash, 
                                  name, &ino, addr)>=0) {
                  ret = 0;
               }
            }
         }
      }
      
      direntListFree(dlsave);
   }
   return ret;
}

void fisGC(void) {
   struct lfis_inmem *mem = lfisInMem();

   if (mem!=NULL) {
      int i;
   
      for (i=0; i<lengthof(mem->exts); i++) {
         struct lfis_extent *ext = mem->exts[i].ext;
         if (ext!=NULL) extentGC(ext, mem->exts[i].flash);
      }
   }
}

#if defined(TESTING)

#include <stdio.h>

int main(void) {
   struct lfis_extent extent;
   
   printf("struct lfis_dirent %u\n", sizeof(struct lfis_dirent));
   printf("struct lfis_ino %u\n", sizeof(struct lfis_ino));
   printf("struct lfis_extent %u = SECTOR_SIZE %u\n", 
          sizeof(struct lfis_extent), LFIS_SECTOR_SIZE);
   printf("lengthof(inodes) = %u\n", lengthof(extent.inodes));
   printf("lengthof(root) = %u\n", lengthof(extent.root));
   return 0;
}

#endif
