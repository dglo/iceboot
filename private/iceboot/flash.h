/* flash routines -- implemented by flashdrv.c ...
 */
int flash_lock(const void *from, int bytes);
int flash_unlock(const void *from, int bytes);
int flash_write(void *to, const void *mem, int cnt);
int flash_erase(const void *from, int bytes);
const char *flash_errmsg(int stat);
int flash_verify_addr(const void *addr);

int flash_code_overlaps(void *from, void *to);
int flash_init(void);
int flash_get_limits(void *zero, void **flash_start, void **flash_end);
int flash_get_block_info(int *block_size);
void *flash_chip_addr(int );
