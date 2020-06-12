#include "rangecounter.h"

#include <linux/device.h>  /* device (attriutes) */
#include <linux/cpumask.h> /* nr_cpu_ids, num_online_cpus */
#include <linux/mat.h>

#include "utilities.h"



#ifdef MAT_ADDR_RANGE_COUNTERS
static ssize_t dev_attr_samples_total_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    /* read device attribute (one line + \n) */
    const char* const buf_begin = buf; /* used by MAT_WRITE_BUF */
    bool  buf_full = false;            /* used by MAT_WRITE_BUF */
    const int CPUS = num_online_cpus();
    u64 samples_total;
    int i;

    MAT_MDBG_FUNC();
    /* sum samples counters of each core */
    samples_total = 0;
    for(i=0; i<CPUS; i++)
    {
        struct mat_range_cnt* rcnt = per_cpu_ptr(&cpu_mat_range_cnts, i);
        samples_total += rcnt->cnts[0];
        samples_total += rcnt->cnts[1];
        samples_total += rcnt->cnts[2];
        samples_total += rcnt->cnts[3];
        samples_total += rcnt->cnts[4];
        samples_total += rcnt->cnts[5];
    }

    MAT_WRITE_BUF( "%lld\n", samples_total);
    MAT_MDBG_FUNC( "bytes=%ld", (buf - buf_begin) );
    return (buf - buf_begin);
}
static ssize_t dev_attr_samples_total_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int tmp;
    const int CPUS = num_online_cpus();
    int i;

    MAT_MDBG_FUNC();
    tmp = 1;
    sscanf(buf, "%d", &tmp);
    if(tmp == 0)
    {
        MAT_MDBG_FUNC( "count=%ld tmp=%d", count, tmp );
        /* if 0 reset sample range counter of each core */
        for(i=0; i<CPUS; i++)
        {
            struct mat_range_cnt* rcnt = per_cpu_ptr(&cpu_mat_range_cnts, i);
            rcnt->cnts[0] = 0;
            rcnt->cnts[1] = 0;
            rcnt->cnts[2] = 0;
            rcnt->cnts[3] = 0;
            rcnt->cnts[4] = 0;
            rcnt->cnts[5] = 0;
        }

        return count;
    }
    /* otherwise report invalid argument and do nothing */
    else
    {
        return -EINVAL;
    }
}
/* create device attribute dev_attr_<name> */
static DEVICE_ATTR(samples_total, S_IRUSR | S_IWUSR, dev_attr_samples_total_show, dev_attr_samples_total_store);

static ssize_t dev_attr_samples_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    /* read device attribute (one line + \n) */
    const char* const buf_begin = buf; /* used by MAT_WRITE_BUF */
    bool  buf_full = false;            /* used by MAT_WRITE_BUF */
    const int CPUS = num_online_cpus();
    int i;
    u64 counter;
    u64 total_cnt = 0;
    u64 total_rcnt_0 = 0;
    u64 total_rcnt_1 = 0;
    u64 total_rcnt_2 = 0;
    u64 total_rcnt_3 = 0;
    u64 total_rcnt_4 = 0;
    u64 total_rcnt_5 = 0;

    MAT_MDBG_FUNC();

    /* get total count for every range to print summary line */
    total_cnt = 0;
    for(i=0; i<CPUS; i++)
    {
        struct mat_range_cnt* rcnt = per_cpu_ptr(&cpu_mat_range_cnts, i);
        total_rcnt_0 += rcnt->cnts[0];
        total_rcnt_1 += rcnt->cnts[1];
        total_rcnt_2 += rcnt->cnts[2];
        total_rcnt_3 += rcnt->cnts[3];
        total_rcnt_4 += rcnt->cnts[4];
        total_rcnt_5 += rcnt->cnts[5];
        total_cnt += total_rcnt_0 + total_rcnt_1 +
                   total_rcnt_2 + total_rcnt_3 +
                   total_rcnt_4 + total_rcnt_5;
    }

    if(total_cnt && CPUS)
    {
        MAT_WRITE_BUF(" %3s | <= 0x%016llx[%d] "
                "| <= 0x%016llx[%d] | <= 0x%016llx[%d] | <= 0x%016llx[%d] "
                "| <= 0x%016llx[%d] | <= 0x%016llx[%d]\n",
                "CPU",
                0x0ull,                mat_addr_range(0x0ull),
                0x40000000ull,         mat_addr_range(0x40000000ull),
                0x7d0000000000ull,     mat_addr_range(0x7d0000000000ull),
                0x7ff000000000ull,     mat_addr_range(0x7ff000000000ull),
                0xfff000000000000ull,  mat_addr_range(0xfff000000000000ull),
                0xffffffffffffffffull, mat_addr_range(0xffffffffffffffffull));
        MAT_WRITE_BUF("-----+--------------------------+"
            "--------------------------+--------------------------+"
            "--------------------------+--------------------------+"
            "--------------------------\n");

        if(gm_cpu < 0)
        {
            MAT_WRITE_BUF(" %3s | %24lld | %24lld | %24lld "
                    "| %24lld | %24lld | %24lld\n",
                    "ALL",
                    total_rcnt_0,
                    total_rcnt_1,
                    total_rcnt_2,
                    total_rcnt_3,
                    total_rcnt_4,
                    total_rcnt_5);
            MAT_WRITE_BUF("-----+--------------------------+"
                "--------------------------+--------------------------+"
                "--------------------------+--------------------------+"
                "--------------------------\n");
        }
    }

    if(gm_cpu < 0)
    {
        /* sum samples counters of each core */
        for(i=0; i<CPUS; i++)
        {
            struct mat_range_cnt* rcnt = per_cpu_ptr(&cpu_mat_range_cnts, i);
            counter  = rcnt->cnts[0];
            counter += rcnt->cnts[1];
            counter += rcnt->cnts[2];
            counter += rcnt->cnts[3];
            counter += rcnt->cnts[4];
            counter += rcnt->cnts[5];
            if( counter )
            {
                MAT_WRITE_BUF(" %3d | %24lld | %24lld | %24lld "
                        "| %24lld | %24lld | %24lld\n",
                        i,
                        rcnt->cnts[mat_addr_range(0x0ull)],
                        rcnt->cnts[mat_addr_range(0x40000000ull)],
                        rcnt->cnts[mat_addr_range(0x7d0000000000ull)],
                        rcnt->cnts[mat_addr_range(0x7ff000000000ull)],
                        rcnt->cnts[mat_addr_range(0xfff000000000000ull)],
                        rcnt->cnts[mat_addr_range(0xffffffffffffffffull)]);
            }
        }
    }
    else if(gm_cpu >= 0 && gm_cpu < CPUS)
    {
        struct mat_range_cnt* rcnt = per_cpu_ptr(&cpu_mat_range_cnts, gm_cpu);
        counter  = rcnt->cnts[0];
        counter += rcnt->cnts[1];
        counter += rcnt->cnts[2];
        counter += rcnt->cnts[3];
        counter += rcnt->cnts[4];
        counter += rcnt->cnts[5];
        if( counter )
        {
            MAT_WRITE_BUF(" %3d | %24lld | %24lld | %24lld "
                    "| %24lld | %24lld | %24lld\n",
                    gm_cpu,
                    rcnt->cnts[mat_addr_range(0x0ull)],
                    rcnt->cnts[mat_addr_range(0x40000000ull)],
                    rcnt->cnts[mat_addr_range(0x7d0000000000ull)],
                    rcnt->cnts[mat_addr_range(0x7ff000000000ull)],
                    rcnt->cnts[mat_addr_range(0xfff000000000000ull)],
                    rcnt->cnts[mat_addr_range(0xffffffffffffffffull)]);
        }
    }

    if( buf == buf_begin )
    {
        MAT_WRITE_BUF( "%d\n", 0);
    }

    MAT_MDBG_FUNC( "bytes=%ld", (buf - buf_begin) );
    return (buf - buf_begin);
}
static ssize_t dev_attr_samples_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    const int CPUS = num_online_cpus();
    int tmp;
    int i;

    MAT_MDBG_FUNC();
    tmp = 1;
    sscanf(buf, "%d", &tmp);
    if(tmp == 0)
    {
        MAT_MDBG_FUNC( "count=%ld tmp=%d", count, tmp );
        /* if 0, reset sample counter of each core */
        for(i=0; i<CPUS; i++)
        {
            struct mat_range_cnt* rcnt = per_cpu_ptr(&cpu_mat_range_cnts, i);
            rcnt->cnts[0] = 0;
            rcnt->cnts[1] = 0;
            rcnt->cnts[2] = 0;
            rcnt->cnts[3] = 0;
            rcnt->cnts[4] = 0;
            rcnt->cnts[5] = 0;
        }

        return count;
    }
    /* otherwise report invalid argument and do nothing */
    else
    {
        return -EINVAL;
    }
}
/* create device attribute dev_attr_<name> */
static DEVICE_ATTR(samples, S_IRUSR | S_IWUSR, dev_attr_samples_show, dev_attr_samples_store);

static ssize_t dev_attr_samples_total_nz_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    /* read device attribute (one line + \n) */
    const char* const buf_begin = buf; /* used by MAT_WRITE_BUF */
    bool  buf_full = false;            /* used by MAT_WRITE_BUF */
    const int CPUS = num_online_cpus();
    u64 samples_total_nz;
    int i;

    MAT_MDBG_FUNC();
    /* sum samples counters of each core */
    samples_total_nz = 0;
    for(i=0; i<CPUS; i++)
    {
        struct mat_range_cnt* rcnt = per_cpu_ptr(&cpu_mat_range_cnts, i);
        /* skip counters for address 0x0 */
        samples_total_nz += rcnt->cnts[1];
        samples_total_nz += rcnt->cnts[2];
        samples_total_nz += rcnt->cnts[3];
        samples_total_nz += rcnt->cnts[4];
        samples_total_nz += rcnt->cnts[5];
    }

    MAT_WRITE_BUF( "%lld\n", samples_total_nz);
    MAT_MDBG_FUNC( "bytes=%ld", (buf - buf_begin) );
    return (buf - buf_begin);
}
static ssize_t dev_attr_samples_total_nz_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -EROFS;
}
/* create device attribute dev_attr_<name> */
static DEVICE_ATTR(samples_total_nz, S_IRUSR | S_IWUSR, dev_attr_samples_total_nz_show, dev_attr_samples_total_nz_store);




int rangecounter_setup_devattr(void)
{
    int rval;
    rval = device_create_file(gm_device, &dev_attr_samples_total);
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to create %s", dev_attr_samples_total.attr.name );
        return -1;
    }
    MAT_MDBG_FUNC( "created %s", dev_attr_samples_total.attr.name );
    
    rval = device_create_file(gm_device, &dev_attr_samples);
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to create %s", dev_attr_samples.attr.name );
        return -1;
    }
    MAT_MDBG_FUNC( "created %s", dev_attr_samples.attr.name );

    rval = device_create_file(gm_device, &dev_attr_samples_total_nz);
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to create %s", dev_attr_samples_total_nz.attr.name );
        return -1;
    }
    MAT_MDBG_FUNC( "created %s", dev_attr_samples_total_nz.attr.name );
    return rval;
}
#endif /* MAT_ADDR_RANGE_COUNTERS */

