#include "utilities.h"
#include <linux/mat.h>

/* global variables */
struct device *gm_device = NULL;
int gm_cpu = -1;

#ifdef MAT_MODULE_DEBUG
int gm_module_debug = 0;
#endif /* MAT_MODULE_DEBUG */



#ifdef MAT_MODULE_DEBUG
static ssize_t dev_attr_module_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char* const buf_begin = buf; /* used by MAT_WRITE_BUF */
    bool  buf_full = false;            /* used by MAT_WRITE_BUF */

    MAT_WRITE_BUF("MAT_MODULE_DEBUG\n");
#ifdef MAT_KERNEL_DEBUG
    MAT_WRITE_BUF("MAT_KERNEL_DEBUG\n");
#endif /* MAT_KERNEL_DEBUG */
#ifdef MAT_KERNEL_DEBUG_FLAG
    MAT_WRITE_BUF("MAT_KERNEL_DEBUG_FLAG\n");
#endif /* MAT_KERNEL_DEBUG_FLAG */
#ifdef MAT_PERF_NO_THROTTLING_FLAG
    MAT_WRITE_BUF("MAT_PERF_NO_THROTTLING_FLAG\n");
#endif /* MAT_PERF_NO_THROTTLING_FLAG */
#ifdef MAT_PERF_FORCE_LPEBS_FLAG
    MAT_WRITE_BUF("MAT_PERF_FORCE_LPEBS_FLAG\n");
#endif /* MAT_PERF_FORCE_LPEBS_FLAG */
#ifdef MAT_GET_ADDR
    MAT_WRITE_BUF("MAT_GET_ADDR\n");
#endif /* MAT_GET_ADDR */
#ifdef MAT_GET_ADDR_FLAG
    MAT_WRITE_BUF("MAT_GET_ADDR_FLAG\n");
#endif /* MAT_GET_ADDR_FLAG */
#ifdef MAT_PHYS_ADDR_FLAG
    MAT_WRITE_BUF("MAT_PHYS_ADDR_FLAG\n");
#endif /* MAT_PHYS_ADDR_FLAG */
#ifdef MAT_ADDR_RANGE_COUNTERS
    MAT_WRITE_BUF("MAT_ADDR_RANGE_COUNTERS\n");
#endif /* MAT_ADDR_RANGE_COUNTERS */
#ifdef MAT_ADDR_BUFFERS
    MAT_WRITE_BUF("MAT_ADDR_BUFFERS\n");
#endif /* MAT_ADDR_BUFFERS */

    MAT_WRITE_BUF("gm_module_debug=%d\n", gm_module_debug);
    MAT_WRITE_BUF("gm_cpu=%d\n", gm_cpu);

#ifdef MAT_KERNEL_DEBUG_FLAG
    MAT_WRITE_BUF("gk_mat_kernel_debug=%d\n", gk_mat_kernel_debug);
#endif /* MAT_KERNEL_DEBUG_FLAG */

#ifdef MAT_PERF_NO_THROTTLING_FLAG
    MAT_WRITE_BUF("gk_mat_perf_no_throttling=%d\n", gk_mat_perf_no_throttling);
#endif /* MAT_PERF_NO_THROTTLING_FLAG */

#ifdef MAT_PERF_FORCE_LPEBS_FLAG
    MAT_WRITE_BUF("gk_mat_perf_force_lpebs=%d\n", gk_mat_perf_force_lpebs);
#endif /* MAT_PERF_FORCE_LPEBS_FLAG */

#ifdef MAT_GET_ADDR_FLAG
    MAT_WRITE_BUF("gk_mat_get_addr=%d\n", gk_mat_get_addr);
#endif /* MAT_GET_ADDR_FLAG */
#ifdef MAT_PHYS_ADDR_FLAG
    MAT_WRITE_BUF("gk_mat_phys_addr=%d\n", gk_mat_phys_addr);
#endif /* MAT_PHYS_ADDR_FLAG */

#ifdef MAT_ADDR_BUFFERS
    MAT_WRITE_BUF("gk_mat_buffers_enabled=%d\n", gk_mat_buffers_enabled);
#endif /* MAT_ADDR_BUFFERS */

    MAT_WRITE_BUF("PAGE_SIZE     %ld\n", PAGE_SIZE);
    // MAT_WRITE_BUF("VMALLOC_START 0x%lx\n", VMALLOC_START);
    // MAT_WRITE_BUF("VMALLOC_END   0x%lx\n", VMALLOC_END);
    // MAT_WRITE_BUF("VMALLOC SIZE  0x%lx (%ld GiB)\n", (VMALLOC_END - VMALLOC_START), (VMALLOC_END - VMALLOC_START)/(1024*1024*1024) );

    MAT_MDBG_FUNC( "bytes=%ld", (buf - buf_begin) );
    return (buf - buf_begin);
}
static ssize_t dev_attr_module_debug_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int tmp;

    tmp = -1;
    sscanf(buf, "%d", &tmp);
    if(tmp == 0 || tmp == 1)
    {
        gm_module_debug = tmp;
        return count;
    }
    else
    {
        return -EINVAL;
    }
}
/* create device attribute dev_attr_<name> */
static DEVICE_ATTR(module_debug, S_IRUSR | S_IWUSR, dev_attr_module_debug_show, dev_attr_module_debug_store);
#endif /* MAT_MODULE_DEBUG */



#ifdef MAT_KERNEL_DEBUG_FLAG
static ssize_t dev_attr_kernel_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char* const buf_begin = buf; /* used by MAT_WRITE_BUF */
    bool  buf_full = false;            /* used by MAT_WRITE_BUF */
    MAT_WRITE_BUF( "%d\n", gk_mat_kernel_debug);
    MAT_MDBG_FUNC( "bytes=%ld", (buf - buf_begin) );
    return (buf - buf_begin);
}
static ssize_t dev_attr_kernel_debug_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int tmp;

    tmp = -1;
    sscanf(buf, "%d", &tmp);
    if(tmp == 0 || tmp == 1)
    {
        MAT_MDBG_FUNC( "count=%ld tmp=%d", count, tmp );
        gk_mat_kernel_debug = tmp;
        return count;
    }
    else
    {
        return -EINVAL;
    }
}
/* create device attribute dev_attr_<name> */
static DEVICE_ATTR(kernel_debug, S_IRUSR | S_IWUSR, dev_attr_kernel_debug_show, dev_attr_kernel_debug_store);
#endif /* MAT_KERNEL_DEBUG_FLAG */



static ssize_t dev_attr_cpu_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char* const buf_begin = buf; /* used by MAT_WRITE_BUF */
    bool  buf_full = false;            /* used by MAT_WRITE_BUF */
    MAT_MDBG_FUNC();

    if(gm_cpu >= 0)
    {
        MAT_WRITE_BUF( "%d\n", gm_cpu);
    }

    MAT_MDBG_FUNC( "bytes=%ld", (buf - buf_begin) );
    return (buf - buf_begin);
}
static ssize_t dev_attr_cpu_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    /* possible input are -1 (all cpus) or [0, CPUS-1] */
    const int CPUS = num_online_cpus();
    int arg;
    ssize_t tmp;

    MAT_MDBG_FUNC( "count=%ld", count );

    tmp = sscanf(buf, "%d", &arg);
    if(tmp < 1 || tmp > 2)
    {
        return -EINVAL;
    }

    if(arg < -1 || arg >= CPUS)
    {
        return -EINVAL;
    }
    gm_cpu = arg;

    MAT_MDBG_FUNC( "count=%ld arg=%d tmp=%ld gm_cpu=%d", count, arg, tmp, gm_cpu );
    return count;
}
/* create device attribute dev_attr_<name> */
static DEVICE_ATTR(cpu, S_IRUSR | S_IWUSR, dev_attr_cpu_show, dev_attr_cpu_store);



int utilities_setup_devattr(void)
{
    int rval = 0;
    rval = device_create_file(gm_device, &dev_attr_cpu);
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to create %s", dev_attr_cpu.attr.name );
        return -1;
    }
    MAT_MDBG_FUNC( "created %s", dev_attr_cpu.attr.name );

#ifdef MAT_MODULE_DEBUG
    rval = device_create_file(gm_device, &dev_attr_module_debug);
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to create %s", dev_attr_module_debug.attr.name );
        return -1;
    }
    MAT_MDBG_FUNC( "created %s", dev_attr_module_debug.attr.name );
#endif /* MAT_MODULE_DEBUG */

#ifdef MAT_KERNEL_DEBUG_FLAG
    rval = device_create_file(gm_device, &dev_attr_kernel_debug);
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to create %s", dev_attr_kernel_debug.attr.name );
        return -1;
    }
    MAT_MDBG_FUNC( "created %s", dev_attr_kernel_debug.attr.name );
#endif /* MAT_KERNEL_DEBUG_FLAG */
    return rval;
}

void utilities_reset(void)
{
#ifdef MAT_KERNEL_DEBUG_FLAG
    gk_mat_kernel_debug = 0;
#endif /* MAT_KERNEL_DEBUG_FLAG */
}

