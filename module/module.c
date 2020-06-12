/*
 * @author  Stefan Noll <stefan.noll@cs.tu-dortmund.de>
 * @date    2019
 * @version 0.1
 * @brief   Linux Kernel Module (LKM) for managing data structures of kernel.
 */

/*
 * CODESTYLE.
 * use prefix "gm_" for global variables of module
 * use prefix "gk_mat_" for global variables of kernel
 * use prefix "mat_" for variables/functions of kernel
 * use prefix "MAT_" for any #define of module/kernel
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/uaccess.h> /* copy_to/from_user() */
#include <linux/cpumask.h> /* nr_cpu_ids, num_online_cpus */

#include "utilities.h"
#include "flags.h"
#include "rangecounter.h"
#include "corebuffer.h"



MODULE_LICENSE("GPL"); /* use GPL exported symbols from kernel */
MODULE_AUTHOR("Stefan Noll <stefan.noll@cs.tu-dortmund.de>");
MODULE_DESCRIPTION("Manage the kernel's data structures used for collecting memory addresses with PEBS.");
MODULE_VERSION("0.1");

/* prototypes for device functions */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

/* handles for managing device */
static struct cdev *gm_cdev;
static struct class *gm_class;
static dev_t gm_dev;
/* atomic counter for counting active calls to open at any time */
static atomic_t gm_device_open_count;

/* structure holding all of the device functions */
static struct file_operations gm_fileops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};

/* called when opening device */
static int device_open(struct inode *inode, struct file *file)
{
    MAT_MDBG_FUNC();
    /* if device is already open, return busy */
    if(atomic_read(&gm_device_open_count))
    {
        return -EBUSY;
    }
    atomic_inc(&gm_device_open_count);
    try_module_get(THIS_MODULE);
    return 0;
}

/* called when closing device */
static int device_release(struct inode *inode, struct file *file)
{
    MAT_MDBG_FUNC();
    atomic_dec(&gm_device_open_count);
    module_put(THIS_MODULE);
    return 0;
}

/* called when reading from device */
static ssize_t device_read(struct file *flip, char *buf, size_t len, loff_t *off)
{
#ifdef MAT_ADDR_BUFFERS
    return corebuffer_read(&buf, &len, &off);
#endif /* MAT_ADDR_BUFFERS */
    return -ENODATA;
}

/* called when writing to device */
static ssize_t device_write(struct file *flip, const char *buf, size_t len, loff_t *off)
{
    /* read-only device */
    MAT_MDBG_FUNC( "len=%ld off=%ld", len, (long int)*off );
    return -EROFS;
}



/* LKM initialization function */
static int __init init_lkm_module(void)
{
    int rval;

    MAT_MDBG_FUNC();
    MAT_MDBG_FUNC( "@THIS_MODULE=%px NR_CPUS=%d #cpuids=%d #cpus=%d PAGE_SIZE=%ld", THIS_MODULE, NR_CPUS, nr_cpu_ids, num_online_cpus(), PAGE_SIZE );
    atomic_set(&gm_device_open_count, 0);

    /* create device in /dev with read/write option */

    /* allocate device region with @DEVICE_NAME */
    rval = alloc_chrdev_region(&gm_dev, 1, 1, DEVICE_NAME);
    if (rval != 0)
    {
        MAT_MERR_FUNC( "alloc_chrdev_region" );
        goto cdev_alloc_err;
    }
    MAT_MDBG_FUNC( "major=%d minor=%d", MAJOR(gm_dev), MINOR(gm_dev) );

    /* allocate device */
    gm_cdev = cdev_alloc();
    if (!gm_cdev)
    {
        MAT_MERR_FUNC( "cdev_alloc" );
        goto cdev_alloc_err;
    }
    /* initialize device with file options */
    cdev_init(gm_cdev, &gm_fileops);
    /* add device to kernel */
    rval = cdev_add(gm_cdev, gm_dev, 1);
    if (rval < 0)
    {
        MAT_MERR_FUNC( "cdev_add" );
        goto cdev_add_err;
    }

    /* create device (attributes) in /sys/devices/virtual */

    /* create device class */
    gm_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(gm_class))
    {
        MAT_MERR_FUNC( "class_create %s", CLASS_NAME);
        goto class_err;
    }
    /* create device */
    gm_device = device_create(gm_class, NULL, gm_dev, NULL, DEVICE_NAME);
    if (IS_ERR(gm_device))
    {
        MAT_MERR_FUNC( "device_create %s", DEVICE_NAME);
        goto device_err;
    }
    MAT_MDBG_FUNC( "created %s", DEVICE_PATH );

    /* create device attributes */
    rval = utilities_setup_devattr();
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to setup device attributes for flags" );
        goto device_err;
    }

    rval = flags_setup_devattr();
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to setup device attributes for flags" );
        goto device_err;
    }

#ifdef MAT_ADDR_RANGE_COUNTERS
    rval = rangecounter_setup_devattr();
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to setup device attributes for per-core range counters" );
        goto device_err;
    }
#endif /* MAT_ADDR_RANGE_COUNTERS */

#ifdef MAT_ADDR_BUFFERS
    rval = corebuffer_setup_devattr();
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to setup device attributes for per-core buffers" );
        goto device_err;
    }
#endif /* MAT_ADDR_BUFFERS */

    return 0;

device_err:
    /* delete device */
    device_destroy(gm_class, gm_dev);
class_err:
    /* delete class */
    class_unregister(gm_class);
    class_destroy(gm_class);
cdev_add_err:
    /* delete device */
    unregister_chrdev_region(gm_dev, 1);
    cdev_del(gm_cdev);
cdev_alloc_err:
    return -EFAULT;
}

/* LKM cleanup function */
static void __exit exit_lkm_module(void)
{
    MAT_MDBG_FUNC();

    device_destroy(gm_class, gm_dev);
    class_unregister(gm_class);
    class_destroy(gm_class);
    unregister_chrdev_region(gm_dev, 1);
    cdev_del(gm_cdev);

    utilities_reset();
    flags_reset();
#ifdef MAT_ADDR_BUFFERS
    corebuffer_reset();
#endif /* MAT_ADDR_BUFFERS */
}

/* register the initialization and cleanup function of the LKM */
module_init(init_lkm_module);
module_exit(exit_lkm_module);
