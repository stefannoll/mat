#ifndef _MAT_COREBUFFER_H
#define _MAT_COREBUFFER_H

#include <linux/mat.h>

#define BUFFER_LIMIT (0x10000000000ull) /* let size of all CPU buffers not exceed 128 GiB */

#ifdef MAT_ADDR_BUFFERS
extern ssize_t gm_corebuffer_bytes;

int corebuffer_setup_devattr(void);
void corebuffer_reset(void);
size_t corebuffer_read(char **p_buf, size_t *p_len, loff_t **p_off);
#endif /* MAT_ADDR_BUFFERS */

#endif /* _MAT_COREBUFFER_H */
