/* flash routines...
 *
 * pointers are "from", "to"
 */
int flash_lock(const void *from, const void *to);
int flash_unlock(const void *from, const void *to);
int flash_write(void *from, const void *to, int cnt);
int flash_erase(const void *from, const void *to);
