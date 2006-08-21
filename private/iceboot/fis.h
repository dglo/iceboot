#ifndef LFIS_HEADER_INCLUDED
#define LFIS_HEADER_INCLUDED

/* fis.h, log structured flash filesystem...
 */

/* magic numbers
 */
#define LFIS_MAGIC_EXT  0x087cb072

/* flash sector info -- 8Mb, 64k sectors, 2 chips...
 */
#define LFIS_LOG2_SECTOR_SIZE 16
#define LFIS_SECTOR_SIZE      (1<<LFIS_LOG2_SECTOR_SIZE)
#define LFIS_LOG2_BYTES       23
#define LFIS_LOG2_SECTORS     (LFIS_LOG2_BYTES - LFIS_LOG2_SECTOR_SIZE)
#define LFIS_SECTORS          (1<<LFIS_LOG2_SECTORS)
#define LFIS_LOG2_CHIPS       1
#define LFIS_CHIPS            (1<<LFIS_LOG2_CHIPS)
#define LFIS_CHIP_SECTORS     (1<<(LFIS_LOG2_SECTORS-LFIS_LOG2_CHIPS))

/* block info -- change w/ great care!
 */
#define LFIS_LOG2_BLOCK_SIZE   9
#define LFIS_BLOCK_SIZE        (1<<LFIS_LOG2_BLOCK_SIZE)
#define LFIS_LOG2_BLOCKS_PER_SECTOR \
                               (LFIS_LOG2_SECTOR_SIZE - LFIS_LOG2_BLOCK_SIZE)
#define LFIS_BLOCKS_PER_SECTOR (1<<(LFIS_LOG2_BLOCKS_PER_SECTOR))
#define LFIS_LOG2_BLOCKS       (LFIS_LOG2_BYTES - LFIS_LOG2_BLOCK_SIZE)
#define LFIS_BLOCKS            (1<<LFIS_LOG2_BLOCKS)
#define LFIS_CHIP_BLOCKS       (1<<(LFIS_LOG2_BLOCKS - LFIS_LOG2_CHIPS))
#define LFIS_BAD_BLOCK_LENGTH  (LFIS_BLOCKS/32)
#define LFIS_LOG2_RESERVE_BYTES 16
#define LFIS_LOG2_RESERVE_BLOCKS \
                               (LFIS_LOG2_RESERVE_BYTES - LFIS_LOG2_BLOCK_SIZE)
#define LFIS_RESERVE_BLOCKS    (1<<LFIS_LOG2_RESERVE_BLOCKS)

/* misc...
 */
#define LFIS_MODE_EXEC         (1<<0)
#define LFIS_MODE_WRITE        (1<<1)
#define LFIS_MODE_READ         (1<<2)
#define LFIS_MODE_DIR          (1<<3)
#define LFIS_MODE_FIXED        (1<<4)
#define LFIS_MODE_BAD          (1<<5)
#define LFIS_MODE_EXTENT       (1<<6)

/* verify some lengths... */
#if LFIS_LOG2_BLOCK_SIZE < LFIS_LOG2_EXTENT_ALLOC
#error "block size is too small"
#endif

/* directory entry (32 bytes)... */
#define LFIS_NAME_SIZE (32 - 8 - 1)

#define LFIS_ACTION_ADD   0
#define LFIS_ACTION_RM    1
#define LFIS_ACTION_EMPTY 0xff

struct lfis_dirent {
   unsigned char action;
   char name[LFIS_NAME_SIZE];
   unsigned ino;
   unsigned crc32;
};

struct lfis_ino {
   unsigned block;          /* allocation (0xffffffff if no alloc)...  */
   unsigned length;         /* in bytes                                */
   unsigned mode;           /* 0ebf drwx                               */
   unsigned seqn;           /* sequence number                         */
   unsigned pad[2];
   unsigned blk_crc32;      /* crc32 of length bytes of the block data */
   unsigned crc32;          /* crc32 of the above fields...            */
};

/* this data structure describes an extent.  an extent
 * is the lowest level structure on the flash, it describes
 * contiguous regions of flash that can be used for the filesystem,
 * similar to partition tables.  all allocations saved in an extent 
 * are crc32 protected.  for iceboot, we have 2 extents,
 * one for each chip.  extents _must_ be sector aligned and have
 * a length which a multiple of the sector size.  the data structure
 * size should be a sector to ease gc...
 *
 * in a given extent, a given inode number only exists once, i.e.
 * we duplicate _across_ extents but not within an extent...
 */
struct lfis_extent {
   unsigned magic;    /* <- LFIS_MAGIC_EXT */
   unsigned sblock;   /* start block (sector aligned) */
   unsigned eblock;   /* end block (sector aligned) */
   unsigned pad[4];   /* all ones -- pad to sizeof(inode) */
   unsigned crc32;    /* protects the fields above...    */

   struct lfis_ino inodes[(LFIS_SECTOR_SIZE/2)/sizeof(struct lfis_ino) - 1];
   struct lfis_dirent root[(LFIS_SECTOR_SIZE/2)/sizeof(struct lfis_dirent)];
};

void fisList(void);
void *fisLookup(const char *name, unsigned long *data_length);
int fisDelete(const char *name);
int fisUnlock(const char *name);
int fisLock(const char *name);
int fisCreate(const char *name, void *addr, int len);
int fisInit(void);


#endif
