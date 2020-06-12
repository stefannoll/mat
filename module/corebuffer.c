#include "corebuffer.h"

#include <linux/slab.h>    /* vmalloc, vfree */
#include <linux/device.h>  /* device (attriutes) */
#include <linux/uaccess.h> /* copy_to/from_user() */
#include <linux/cpumask.h> /* nr_cpu_ids, num_online_cpus */

#include "utilities.h"

#if 0
u64 g_debug_inc = 0;
#endif

#ifdef MAT_ADDR_BUFFERS
ssize_t gm_corebuffer_bytes = 0;



static u64 create_buffer(struct mat_buffer* buffer, u64 capacity)
{
    if(!buffer)
    {
        MAT_MERR_FUNC( "NULL buffer" );
        return 0;
    }

    buffer->data     = NULL;
    buffer->size     = 0;
    buffer->capacity = 0;
    buffer->data     = (u64*) vmalloc( sizeof(u64)*capacity );
    if(!buffer->data)
    {
        MAT_MERR_FUNC( "failed vmalloc %lld bytes", capacity );
        return 0;
    }
    MAT_MDBG_FUNC( "vmalloc %lld bytes", capacity );
    buffer->capacity = capacity;

    return capacity;
}



static void destoy_buffer(struct mat_buffer* buffer)
{
    if(buffer && buffer->data)
    {
        buffer->capacity = 0;
        buffer->size     = 0;
        MAT_MDBG_FUNC( "vfree buffer->data=%px", buffer->data );
        vfree( buffer->data );
        buffer->data     = NULL;
    }
}



static u64 create_buffers(struct mat_buffers* buffers, u64 capacity)
{
    u32 idx;
    u64 ret;
    struct mat_buffer* buffer;

    buffers->buf_idx = 0;
    ret = 0;
    for(idx=0; idx<MAT_BUF_NUM; idx++)
    {
        buffer = &(buffers->buffers[idx]);
        ret += create_buffer(buffer, capacity);
    }
    return ret;
}



static void destoy_buffers(struct mat_buffers* buffers)
{
    u32 idx;
    struct mat_buffer* buffer;

    buffers->buf_idx = 0;
    for(idx=0; idx<MAT_BUF_NUM; idx++)
    {
        buffer = &(buffers->buffers[idx]);
        destoy_buffer(buffer);
    }
}



static void destoy_allbuffers(void)
{
    const int CPUS = num_online_cpus();
    int cpu;
    for(cpu=0; cpu<CPUS; cpu++)
    {
        struct mat_buffers* buffers = per_cpu_ptr(&cpu_mat_buffers, cpu);
        destoy_buffers( buffers );
    }
}



/*
 * device attribute functions for managing per-core buffers
 */
static ssize_t dev_attr_buffers_enabled_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char* const buf_begin = buf; /* used by MAT_WRITE_BUF */
    bool  buf_full = false;            /* used by MAT_WRITE_BUF */
    MAT_MDBG_FUNC();

    if(gk_mat_buffers_enabled >= 0)
    {
        MAT_WRITE_BUF( "%d\n", gk_mat_buffers_enabled);
    }

    MAT_MDBG_FUNC( "bytes=%ld", (buf - buf_begin) );
    return (buf - buf_begin);
}
static ssize_t dev_attr_buffers_enabled_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int arg;
    ssize_t tmp;

    MAT_MDBG_FUNC( "count=%ld", count );

    tmp = sscanf(buf, "%d", &arg);
    if(tmp != 1)
    {
        return -EINVAL;
    }

    if(arg < 0 || arg > 1)
    {
        return -EINVAL;
    }
    gk_mat_buffers_enabled = arg;
    MAT_MDBG_FUNC( "count=%ld arg=%d tmp=%ld gk_mat_buffers_enabled=%d", count, arg, tmp, gk_mat_buffers_enabled );
    return count;
}
/* create device attribute dev_attr_<name> */
static DEVICE_ATTR(buffers_enabled, S_IRUSR | S_IWUSR, dev_attr_buffers_enabled_show, dev_attr_buffers_enabled_store);

static ssize_t dev_attr_buffers_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    /*
     * use @cpu to control output.
     * @gm_cpu >= 0 && @gm_cpu < CPUS: give detailed info about buffers one core
     * @gm_cpu < 0: show brief summary of all cores
     */
    const char* const buf_begin = buf; /* used by MAT_WRITE_BUF */
    bool  buf_full = false;            /* used by MAT_WRITE_BUF */
    const int CPUS = num_online_cpus();
    u64 total_capacity;
    u64 total_size;
    MAT_MDBG_FUNC();

    total_capacity = 0;
    total_size = 0;
    if(gm_cpu < 0)
    {
        int cpu, idx;
        struct mat_buffers* buffers;
        u64 cpu_capacity;
        u64 cpu_size;
        for(cpu=0; cpu<CPUS; cpu++)
        {
            buffers = per_cpu_ptr(&cpu_mat_buffers, cpu);
            if( buffers )
            {
                cpu_capacity = 0;
                cpu_size = 0;
                for(idx=0; idx<MAT_BUF_NUM; idx++)
                {
                    struct mat_buffer* buffer = &(buffers->buffers[idx]);
                    cpu_capacity     += buffer->capacity;
                    cpu_size         += buffer->size;
                }
                if( cpu_size && cpu_size == cpu_capacity )
                {
                    MAT_WRITE_BUF("CPU %2d: %2d# capacity=%lld (%lld MiB) size=%lld (%lld MiB) (full)\n",
                        cpu, MAT_BUF_NUM, cpu_capacity, (cpu_capacity*8)/(1024*1024), cpu_size, (cpu_size*8)/(1024*1024));
                }
                else
                {
                    MAT_WRITE_BUF("CPU %2d: %2d# capacity=%lld (%lld MiB) size=%lld (%lld MiB)\n",
                        cpu, MAT_BUF_NUM, cpu_capacity, (cpu_capacity*8)/(1024*1024), cpu_size, (cpu_size*8)/(1024*1024));
                }
                total_capacity += cpu_capacity;
                total_size += cpu_size;
            }
            else
            {
                MAT_WRITE_BUF("CPU %2d: %px\n", cpu, buffers);
            }
        }
    }
    else if(gm_cpu >= 0 && gm_cpu < CPUS)
    {
        int cpu, idx;
        struct mat_buffers* buffers;
        cpu = gm_cpu;
        buffers = per_cpu_ptr(&cpu_mat_buffers, cpu);
        if( buffers )
        {
            for(idx=0; idx<MAT_BUF_NUM; idx++)
            {
                struct mat_buffer* buffer = &(buffers->buffers[idx]);
                char mark = ' ';
                if( idx == buffers->buf_idx )
                {
                    mark = '*';
                }
                if(buffer)
                {
                    if(buffer->capacity && buffer->capacity == buffer->size)
                    {
                        MAT_WRITE_BUF("CPU %2d %1cBUF %2d: data=%px capacity=%lld (%lld MiB) size=%lld (%lld MiB) (full)\n",
                            cpu, mark, idx, buffer->data, buffer->capacity, (buffer->capacity*8)/(1024*1024), buffer->size, (buffer->size*8)/(1024*1024));
                    }
                    else
                    {
                        MAT_WRITE_BUF("CPU %2d %1cBUF %2d: data=%px capacity=%lld (%lld MiB) size=%lld (%lld MiB)\n",
                            cpu, mark, idx, buffer->data, buffer->capacity, (buffer->capacity*8)/(1024*1024), buffer->size, (buffer->size*8)/(1024*1024));
                    }
                    total_capacity += buffer->capacity;
                    total_size += buffer->size;
                }
                else
                {
                    MAT_WRITE_BUF("CPU %2d %1cBUF %2d: %px\n", cpu, mark, idx, buffers);
                }
            }
        }
        else
        {
            MAT_WRITE_BUF("CPU %2d: %px\n", cpu, buffers);
        }
    }

    if(total_capacity)
    {
        MAT_WRITE_BUF("total_capacity: %16lld (%10lld MiB)\n", total_capacity, (total_capacity*8 / (1024*1024)));
    }
    else
    {
        MAT_WRITE_BUF("total_capacity: %16lld (%10lld MiB)\n", total_capacity, total_capacity);
    }
    if(total_capacity)
    {
        MAT_WRITE_BUF("total_size:     %16lld (%10lld MiB)\n", total_size, (total_size*8 / (1024*1024)));
    }
    else
    {
        MAT_WRITE_BUF("total_size:     %16lld (%10lld MiB)\n", total_size, total_size);
    }

    MAT_MDBG_FUNC( "bytes=%ld", (buf - buf_begin) );
    return (buf - buf_begin);
}
static ssize_t dev_attr_buffers_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    /*
     * We allow 3 different input types:
     * "0":   delete all buffers on every core
     * "N":   allocate all buffers with capacity of N elements on every core
     * "X N": allocate all buffers with capacity of N elements on core X
     */
    const int CPUS = num_online_cpus();
    s64 arg1, arg2;
    ssize_t bytes;
    struct mat_buffers* buffers;

    MAT_MDBG_FUNC();

    arg1 = -1;
    arg2 = -1;
    bytes = sscanf(buf, "%lld%lld", &arg1, &arg2);
    MAT_MDBG_FUNC( "count=%ld arg1=%lld arg2=%lld bytes=%ld per core", count, arg1, arg2, bytes );



    /* delete all buffers */
    if(arg1 == 0)
    {
        int cpu;
        MAT_MDBG_FUNC( "destoy_buffers for every core" );
        for(cpu=0; cpu<CPUS; cpu++)
        {
            buffers = per_cpu_ptr(&cpu_mat_buffers, cpu);
            destoy_buffers( buffers );
        }
        return count;
    }



    /* allocate all buffers on every core */
    if(arg1 > 0 && arg2 == -1 && arg1 * CPUS * MAT_BUF_NUM * 8 <= BUFFER_LIMIT) // limit total buffer size
    {
        const u64 buffer_capacity = arg1;
        int cpu;
        MAT_MDBG_FUNC( "destoy_buffer for every core" );
        for(cpu=0; cpu<CPUS; cpu++)
        {
            buffers = per_cpu_ptr(&cpu_mat_buffers, cpu);
            destoy_buffers( buffers );
        }

        MAT_MDBG_FUNC( "create_buffer for every core" );
        for(cpu=0; cpu<CPUS; cpu++)
        {
            buffers = per_cpu_ptr(&cpu_mat_buffers, cpu);
            if( !create_buffers(buffers, buffer_capacity) )
            {
                MAT_MERR_FUNC( "failed alloc_buffer(%px, %lld)", buffers, buffer_capacity );
                return -EINVAL;
            }
        }

        return count;
    }



    /* allocate all buffers on single core */
    if(arg1 > 0 && arg1 < CPUS && arg2 > 0)
    {
        const int cpu = arg1;
        const u64 buffer_capacity = arg2;
        u64 total_bytes = 0;

        MAT_MDBG_FUNC( "destoy_buffer on core %d", cpu );
        buffers = per_cpu_ptr(&cpu_mat_buffers, cpu);
        destoy_buffers( buffers );

        /* check total total size */
        {
            int i;
            for(i=0; i<CPUS; i++)
            {
                u32 idx;
                buffers = per_cpu_ptr(&cpu_mat_buffers, i);
                for(idx=0; idx<MAT_BUF_NUM; idx++)
                {
                    struct mat_buffer* buffer = &(buffers->buffers[idx]);
                    total_bytes += buffer->capacity * sizeof(u64);
                }
            }
            MAT_MDBG_FUNC( "total_bytes=%llu", total_bytes );
        }
        if( total_bytes + (buffer_capacity * sizeof(u64) * MAT_BUF_NUM) > BUFFER_LIMIT) // limit total buffer size
        {
            MAT_MERR_FUNC( "total buffer too large" );
            return -EINVAL;
        }

        MAT_MDBG_FUNC( "create_buffers on core %d", cpu );
        buffers = per_cpu_ptr(&cpu_mat_buffers, cpu);
        if( !create_buffers(buffers, buffer_capacity) )
        {
            MAT_MERR_FUNC( "failed create_buffers(%px, %lld)", buffers, buffer_capacity );
            return -EINVAL;
        }

        return count;
    }

    return -EINVAL;
}
/* create device attribute dev_attr_<name> */
static DEVICE_ATTR(buffers, S_IRUSR | S_IWUSR, dev_attr_buffers_show, dev_attr_buffers_store);

static ssize_t dev_attr_buffers_bytes_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char* const buf_begin = buf; /* used by MAT_WRITE_BUF */
    bool  buf_full = false;            /* used by MAT_WRITE_BUF */
    const int CPUS = num_online_cpus();
    MAT_MDBG_FUNC();

    if(gm_cpu >= 0 && gm_cpu < CPUS)
    {
        u32 idx;
        struct mat_buffers* buffers = per_cpu_ptr(&cpu_mat_buffers, gm_cpu);
        gm_corebuffer_bytes = 0;
        for(idx=0; idx<MAT_BUF_NUM; idx++)
        {
            struct mat_buffer *buffer = &(buffers->buffers[idx]);
            if( buffer )
            {
                gm_corebuffer_bytes += buffer->size * sizeof(u64);
            }
        }
    }
    else
    {
        gm_corebuffer_bytes = 0;
    }

    if(gm_corebuffer_bytes >= 0)
    {
        MAT_WRITE_BUF( "%ld\n", gm_corebuffer_bytes);
    }

    MAT_MDBG_FUNC( "bytes=%ld", (buf - buf_begin) );
    return (buf - buf_begin);
}
static ssize_t dev_attr_buffers_bytes_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -EROFS;
}
/* create device attribute dev_attr_<name> */
static DEVICE_ATTR(buffers_bytes, S_IRUSR | S_IWUSR, dev_attr_buffers_bytes_show, dev_attr_buffers_bytes_store);



/*
 * setup device attributes for managing per-core buffers
 */
int corebuffer_setup_devattr(void)
{
    int rval;
    rval = device_create_file(gm_device, &dev_attr_buffers_enabled);
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to create %s", dev_attr_buffers_enabled.attr.name );
        return -1;
    }
    MAT_MDBG_FUNC( "created %s", dev_attr_buffers_enabled.attr.name );

    rval = device_create_file(gm_device, &dev_attr_buffers);
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to create %s", dev_attr_buffers.attr.name );
        return -1;
    }
    MAT_MDBG_FUNC( "created %s", dev_attr_buffers.attr.name );

    gm_corebuffer_bytes = 0;
    rval = device_create_file(gm_device, &dev_attr_buffers_bytes);
    if (rval < 0)
    {
        MAT_MERR_FUNC( "failed to create %s", dev_attr_buffers_bytes.attr.name );
        return -1;
    }
    MAT_MDBG_FUNC( "created %s", dev_attr_buffers_bytes.attr.name );
    return 0;
}



void corebuffer_reset(void)
{
    gk_mat_buffers_enabled = 0;
    destoy_allbuffers();
}



size_t corebuffer_read(char **p_buf, size_t *p_len, loff_t **p_off)
{
    /* create temporary parameters */
    char* buf = *p_buf;
    size_t len = *p_len;
    loff_t* off = *p_off;
    size_t result = -ENODATA;

    if(gk_mat_buffers_enabled)
    {
        /*
         * we read data from multiple buffers of core with id @gm_cpu.
         * number of available bytes is the sum of all buffers, visible
         * by reading @gm_corebuffer_bytes from user
         * perspective. the offset @off refers to buffers space of
         * buffers 1 to N (in that order).
         */
        const int CPUS = num_online_cpus();
        ssize_t bytes;
        int rval;
        struct mat_buffers* buffers;
        struct mat_buffer* buffer;
        char* src;
        char* buffer_end;
        u64 buffer_off;
        u64 total_size;
        u32 idx, selected_idx;

        buffer_off = *off;
        bytes = 0;
        rval  = 0;
        MAT_MDBG_FUNC( "buf=%px len=%ld off=%lld bytes=%ld", buf, len, *off, bytes );

        /* make sure that selected CPU @gm_cpu is valid */
        if(gm_cpu < 0 || gm_cpu >= CPUS)
        {
            MAT_MERR_FUNC( "buf=%px len=%ld off=%lld bytes=%ld gm_cpu=%d CPUS=%d", buf, len, *off, bytes, gm_cpu, CPUS );
            result = -EFAULT; goto exit;
        }
        buffers = per_cpu_ptr(&cpu_mat_buffers, gm_cpu);

        if(!buffers)
        {
            MAT_MERR_FUNC( "buf=%px len=%ld off=%lld bytes=%ld buffers=%px", buf, len, *off, bytes, buffers) ;
            result = -ENODATA; goto exit;
        }

        /* compute size of all buffers together, make sure data exists */
        total_size = 0;
        for(idx=0; idx<MAT_BUF_NUM; idx++)
        {
            buffer = &(buffers->buffers[idx]);
            total_size += buffer->size;
            if(!buffer->data)
            {
                MAT_MERR_FUNC( "buf=%px len=%ld off=%lld bytes=%ld idx=%d data=%px", buf, len, *off, bytes, idx, buffer->data );
                result = -ENODATA; goto exit;
            }
        }

        /* check if requestes bytes @len are avaiable in buffers, excluding @off offset */
        if(len > (total_size * sizeof(u64)) - *off )
        {
            MAT_MERR_FUNC( "buf=%px len=%ld off=%lld bytes=%ld total_size=%lld", buf, len, *off, bytes, total_size );
            result = -EFAULT; goto exit;
        }

        /*
         * get index of buffer that corresponds to offset @off
         * we also save in @buffer_off the offset for the first buffer we read
         */
        selected_idx = 0;
        {
            u64 tmp = 0;
            for(idx=0; idx<MAT_BUF_NUM; idx++)
            {
                buffer = &(buffers->buffers[idx]);
                tmp += buffer->size * sizeof(u64);
                if(*off < tmp)
                {
                    selected_idx = idx;
                    break;
                }
                else
                {
                    buffer_off -= buffer->size * sizeof(u64);
                }
            }
        }

        buffer = &(buffers->buffers[selected_idx]);
        /* check that offset is inside buffer (redundant) */
        if(buffer_off >= (buffer->size * sizeof(u64)))
        {
            MAT_MERR_FUNC( "buf=%px len=%ld off=%lld bytes=%ld buffer_bytes=%lld buffer_off=%lld", buf, len, *off, bytes, (buffer->size * sizeof(u64)), buffer_off );
            result = -EFAULT; goto exit;
        }
        MAT_MDBG_FUNC( "buf=%px len=%ld off=%lld bytes=%ld data=%px end=%px selected_idx=%d buffer_off=%lld", buf, len, *off, bytes, buffer->data, buffer->data + buffer->size, selected_idx, buffer_off );

        /*
         * WARNING: we have u64 pointer but byte offset. they are incompatible.
         * set cursor @src to start reading from offset @buffer_off
         * set cursor @buffer_end to mark end of buffer
         */
        src        = (char*)buffer->data + buffer_off;
        buffer_end = (char*)buffer->data + (buffer->size * sizeof(u64));

        /* check cursor (redundant) */
        if(src >= buffer_end)
        {
            MAT_MERR_FUNC( "buf=%px len=%ld off=%lld bytes=%ld src=%px buffer_end=%px", buf, len, *off, bytes, src, buffer_end );
            result = -EFAULT; goto exit;
        }

        /*
         * read as long as we have read @bytes bytes from buffer
         * equal to @len bytes requested by user
         */
        while(bytes < len)
        {
            /* we copy the minimum of: bytes left in buffer and remaining bytes to read */
            const size_t copy_bytes = min((size_t)(buffer_end - src), (len - bytes));
            /*
             * copy_to_user returns #bytes left to copy. 0 means success.
             * see: include/linux/uaccess.h
             */
            rval = copy_to_user(buf, src, copy_bytes);
            /* error */
            if (rval < 0)
            {
                MAT_MERR_FUNC( "buf=%px len=%ld off=%lld bytes=%ld rval=%d src=%px", buf, len, *off, bytes, rval, src );
                result = -EFAULT; goto exit;
            }
            /* for whatever reason @rval bytes were not copied. */
            if( rval > 0 )
            {
                src   += copy_bytes - rval;
                /* we don't handle this case (not tested). abort. */
                MAT_MERR_FUNC( "rval=%d", rval );
                result = -EFAULT; goto exit;
            }
            bytes += copy_bytes - rval;
            buf += bytes;

            MAT_MDBG_FUNC( "idx=%u buf=%px len=%ld off=%lld bytes=%ld rval=%d\n", selected_idx, buf, len, *off, bytes, rval );

            if(bytes < len && rval == 0)
            {
                selected_idx++;
                /* test if we reached end of our buffers. */
                if(selected_idx >= MAT_BUF_NUM)
                {
                    MAT_MERR_FUNC( "buf=%px len=%ld off=%lld bytes=%ld rval=%d selected_idx=%d", buf, len, *off, bytes, rval, selected_idx );
                    result = -EFAULT; goto exit;
                }
                buffer = &(buffers->buffers[selected_idx]);
                src      = (char*)buffer->data;
                buffer_end = (char*)buffer->data + (buffer->size * sizeof(u64));
            }
        }

        *off += bytes;
        MAT_MDBG_FUNC( "buf=%px len=%ld off=%lld bytes=%ld", buf, len, *off, bytes );
        result = bytes; goto exit;
    }

    MAT_MERR_FUNC( "MAT_ADDR_BUFFERS disabled" );
exit:
    /* set parameters */
    *p_buf = buf;
    *p_len = len;
    *p_off = off;
    return result;
}
#endif /* MAT_ADDR_BUFFERS */

