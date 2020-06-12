#ifndef _MAT_UTILITIES_H
#define _MAT_UTILITIES_H

#include <linux/module.h>
#include <linux/device.h>
#include <linux/mat_config.h>

#ifndef CLASS_NAME
   #error "CLASSS_NAME not defined"
#endif /* CLASS_NAME */
#ifndef DEVICE_NAME
   #error "DEVICE_NAME not defined"
#endif /* DEVICE_NAME */
#define DEVICE_PATH "/dev/"DEVICE_NAME
#define ATTRIBUTES_PATH "/sys/devices/virtual/"CLASS_NAME"/"DEVICE_NAME

/* device used to register attributes for IO using sysfs */
extern struct device *gm_device;
/* selected CPU to change behavior of various functions */
extern int gm_cpu;

#define MAT_MODULE_DEBUG // enable debugging in this module
#ifdef MAT_MODULE_DEBUG
extern int gm_module_debug;
#define MAT_MDBG(X) do {if(gm_module_debug){X}} while(0)
#else
#define MAT_MDBG(X) do {} while(0)
#endif /* MAT_MODULE_DEBUG */

/* macro to print source code and runtime information for printf debugging */
#define MAT_MDBG_FUNC(fmt, ...) MAT_MDBG( printk(KERN_INFO DEVICE_NAME ": %s " fmt "\n", __func__, ##__VA_ARGS__); )
#define MAT_MERR_FUNC(fmt, ...) printk(KERN_ERR DEVICE_NAME ": %s " fmt "\n", __func__, ##__VA_ARGS__);

// #define MAT_DEAD_CODE_LKM
/*
 * macro for calling snprintf to write formatted string
 * to "buf". we make sure to write only PAGE_SIZE-1 to
 * buffer in total. needs "buf_begin" to store start of buf.
 * if PAGE_SIZE-1 would be exceeded, we print @limit_msg at
 * last line of the buffer.
 * snprintf: if return value is greater or equal to size, input was truncated
 * WARNING: assume that @buf, @buf_full and @buf_begin are not modified by somebody else
 */
#define MAT_WRITE_BUF( ... ) \
do { \
    const char limit_msg[] = "\n[...] (print limit reached)\n"; \
    const size_t written_bytes = buf - buf_begin; \
    const size_t available_bytes = PAGE_SIZE-1-sizeof(limit_msg) - written_bytes; \
    int buf_tmp; \
    if(!buf_full) \
    { \
        buf_tmp = snprintf(buf, available_bytes, __VA_ARGS__); \
        if( buf_tmp >= available_bytes ) \
        { \
            buf_tmp = snprintf(buf, sizeof(limit_msg), limit_msg); \
            buf_full = true; \
        } \
        buf += buf_tmp; \
    } \
} while(false)

int utilities_setup_devattr(void);
void utilities_reset(void);

/*
 * control with printk rate:
 * /proc/sys/kernel/printk_ratelimit
 * /proc/sys/kernel/printk_ratelimit_burst
 * printk_ratelimited( "" );
 */

#endif /* _MAT_UTILITIES_H */
