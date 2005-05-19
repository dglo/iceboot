#ifndef FIS_HEADER_INCLUDE
#define FIS_HEADER_INCLUDE

struct fis_image_desc {
    unsigned char name[16];      // Null terminated name
    void *flash_base;    // Address within FLASH of image
    void *mem_base;      // Address in memory where it executes
    unsigned long size;          // Length of image
    void *entry_point;   // Execution entry point
    unsigned long data_length;   // Length of actual data
    unsigned char _pad[256-(16+4*sizeof(unsigned long)+3*sizeof(void *))];
    unsigned long desc_cksum;    // Checksum over image descriptor
    unsigned long file_cksum;    // Checksum over image data
};

void fisList(void);
const struct fis_image_desc *fisLookup(const char *name);
int fisDelete(const char *name);
int fisUnlock(const char *name);
int fisLock(const char *name);
int fisCreate(const char *name, void *addr, int len);
int fisInit(void);

#endif
