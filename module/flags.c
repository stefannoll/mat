#include "flags.h"
#include <linux/mat.h>
#include "utilities.h"



#ifdef MAT_PERF_NO_THROTTLING_FLAG
static ssize_t dev_attr_perf_no_throttling_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char* const buf_begin = buf; /* used by MAT_WRITE_BUF */
    bool  buf_full = false;            /* used by MAT_WRITE_BUF */
    MAT_WRITE_BUF( "%d\n", gk_mat_perf_no_throttling);
    MAT_MDBG_FUNC( "bytes=%ld", (buf - buf_begin) );
    return (buf - buf_begin);
}
static ssize_t dev_attr_perf_no_throttling_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int tmp;

    tmp = -1;
    sscanf(buf, "%d", &tmp);
    if(tmp == 0 || tmp == 1)
    {
        MAT_MDBG_FUNC( "count=%ld tmp=%d", count, tmp );
        gk_mat_perf_no_throttling = tmp;
        return count;
    }
    else
    {
        return -EINVAL;
    }
}
/* create device attribute dev_attr_<name> */
static DEVICE_ATTR(perf_no_throttling, S_IRUSR | S_IWUSR, dev_attr_perf_no_throttling_show, dev_attr_perf_no_throttling_store);
#endif /* MAT_PERF_NO_THROTTLING_FLAG */



#ifdef MAT_PERF_FORCE_LPEBS_FLAG
static ssize_t dev_attr_perf_force_lpebs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char* const buf_begin = buf; /* used by MAT_WRITE_BUF */
    bool  buf_full = false;            /* used by MAT_WRITE_BUF */
    MAT_WRITE_BUF("%d\n", gk_mat_perf_force_lpebs);
    MAT_MDBG_FUNC( "bytes=%ld", (buf - buf_begin) );
    return (buf - buf_begin);
}
static ssize_t dev_attr_perf_force_lpebs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int tmp;

    tmp = -1;
    sscanf(buf, "%d", &tmp);
    if(tmp == 0 || tmp == 1)
    {
        MAT_MDBG_FUNC( "count=%ld tmp=%d", count, tmp );
        gk_mat_perf_force_lpebs = tmp;
        return count;
    }
    else
    {
        return -EINVAL;
    }
}
/* create device attribute dev_attr_<name> */
static DEVICE_ATTR(perf_force_lpebs, S_IRUSR | S_IWUSR, dev_attr_perf_force_lpebs_show, dev_attr_perf_force_lpebs_store);
#endif /* MAT_PERF_FORCE_LPEBS_FLAG */



#ifdef MAT_GET_ADDR_FLAG
static ssize_t dev_attr_get_addr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char* const buf_begin = buf; /* used by MAT_WRITE_BUF */
    bool  buf_full = false;            /* used by MAT_WRITE_BUF */
    MAT_WRITE_BUF( "%d\n", gk_mat_get_addr);
    MAT_MDBG_FUNC( "bytes=%ld", (buf - buf_begin) );
    return (buf - buf_begin);
}
static ssize_t dev_attr_get_addr_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int tmp;

    tmp = -1;
    sscanf(buf, "%d", &tmp);
    if(tmp == 0 || tmp == 1)
    {
        MAT_MDBG_FUNC( "count=%ld tmp=%d", count, tmp );
        gk_mat_get_addr = tmp;
        return count;
    }
    else
    {
        return -EINVAL;
    }
}
/* create device attribute dev_attr_<name> */
static DEVICE_ATTR(get_addr, S_IRUSR | S_IWUSR, dev_attr_get_addr_show, dev_attr_get_addr_store);
#endif /* MAT_GET_ADDR_FLAG */



#ifdef MAT_PHYS_ADDR_FLAG
static ssize_t dev_attr_phys_addr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char* const buf_begin = buf; /* used by MAT_WRITE_BUF */
    bool  buf_full = false;            /* used by MAT_WRITE_BUF */
    MAT_WRITE_BUF( "%d\n", gk_mat_phys_addr);
    MAT_MDBG_FUNC( "bytes=%ld", (buf - buf_begin) );
    return (buf - buf_begin);
}
static ssize_t dev_attr_phys_addr_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int tmp;

    tmp = -1;
    sscanf(buf, "%d", &tmp);
    if(tmp == 0 || tmp == 1)
    {
        MAT_MDBG_FUNC( "count=%ld tmp=%d", count, tmp );
        gk_mat_phys_addr = tmp;
        return count;
    }
    else
    {
        return -EINVAL;
    }
}
/* create device attribute dev_attr_<name> */
static DEVICE_ATTR(phys_addr, S_IRUSR | S_IWUSR, dev_attr_phys_addr_show, dev_attr_phys_addr_store);
#endif /* MAT_PHYS_ADDR_FLAG */



int flags_setup_devattr(void)
{
    int rval = 0;
#ifdef MAT_PERF_NO_THROTTLING_FLAG
    rval = device_create_file(gm_device, &dev_attr_perf_no_throttling);
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to create %s", dev_attr_perf_no_throttling.attr.name );
        return -1;
    }
    MAT_MDBG_FUNC( "created %s", dev_attr_perf_no_throttling.attr.name );
#endif /* MAT_PERF_NO_THROTTLING_FLAG */

#ifdef MAT_PERF_FORCE_LPEBS_FLAG
    rval = device_create_file(gm_device, &dev_attr_perf_force_lpebs);
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to create %s", dev_attr_perf_force_lpebs.attr.name );
        return -1;
    }
    MAT_MDBG_FUNC( "created %s", dev_attr_perf_force_lpebs.attr.name );
#endif /* MAT_PERF_FORCE_LPEBS_FLAG */

#ifdef MAT_GET_ADDR_FLAG
    rval = device_create_file(gm_device, &dev_attr_get_addr);
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to create %s", dev_attr_get_addr.attr.name );
        return -1;
    }
    MAT_MDBG_FUNC( "created %s", dev_attr_get_addr.attr.name );
#endif /* MAT_GET_ADDR_FLAG */

#ifdef MAT_PHYS_ADDR_FLAG
    rval = device_create_file(gm_device, &dev_attr_phys_addr);
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to create %s", dev_attr_phys_addr.attr.name );
        return -1;
    }
    MAT_MDBG_FUNC( "created %s", dev_attr_phys_addr.attr.name );
#endif /* MAT_PHYS_ADDR_FLAG */
    return rval;
}



void flags_reset(void)
{
#ifdef MAT_PERF_NO_THROTTLING_FLAG
    gk_mat_perf_no_throttling = 0;
#endif /* MAT_PERF_NO_THROTTLING_FLAG */
#ifdef MAT_PERF_FORCE_LPEBS_FLAG
    gk_mat_perf_force_lpebs = 0;
#endif /* MAT_PERF_FORCE_LPEBS_FLAG */
#ifdef MAT_GET_ADDR_FLAG
    gk_mat_get_addr = 0;
#endif /* MAT_GET_ADDR_FLAG */
#ifdef MAT_PHYS_ADDR_FLAG
    gk_mat_phys_addr = 0;
#endif /* MAT_PHYS_ADDR_FLAG */
}

