#ifndef DEV_H
#define DEV_H

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/statfs.h>
#include <sys/vfs.h>
#include <linux/magic.h>
#include <linux/falloc.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <inttypes.h>
#include <time.h>
#include <x86intrin.h>
#include <stdarg.h> 
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <malloc.h>

#ifdef __GNUC__
#define likely(x)       __builtin_expect((x), 1)
#define unlikely(x)     __builtin_expect((x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

/***********************************************
 * clear cache utility
 ***********************************************/
#define cache_wb_one(addr) _mm_clwb(addr)

#define cache_wb(target, size)              \
    for(size_t i=0; i<size; i+=64){         \
        _mm_clwb((void*)target + i);        \
    }                                     

/* Convensions */
#define PAGE_MASK       (~((uint64_t)0x0fff))
#define FLUSH_ALIGN     (uint64_t)64
#define ALIGN_MASK   	(FLUSH_ALIGN - 1)

#define PMEM_LEN        (118LL << 30)

#define MPK_DEFAULT		0
#define MPK_META		1
#define MPK_FILE		2

enum ioctl_types{
    IOCTL_INIT = 16,
    IOCTL_READY,
    IOCTL_RESET,
    IOCTL_PSWAP,
    IOCTL_PREFAULT,
    IOCTL_COW,
    IOCTL_COPYTEST,
};

struct ioctl_init {
    // to kernel: virtual memory size
    uint64_t size;
    // from kernel: physical space total
    uint64_t space_total;
    // from kernel: physical space remaining
    uint64_t space_remain;
    // from kernel: protection tag for metadata
    uint8_t mpk_meta;
    // from kernel: protection tag for file data
    uint8_t mpk_file;
    // from kernel: protection tag for default
    uint8_t mpk_default; 
};
typedef struct ioctl_init ioctl_init_t;

struct dev_basic_info
{
    int ssd_read_fd;
    int ssd_write_fd;
    int nvm_fd;
    void* nvm_base_addr;
    char mpk[3];
};
typedef struct dev_basic_info dev_info_t;
typedef dev_info_t* dev_info_pt;

int16_t pre_flag_list[3*1024*1024];

dev_info_t fs_dev_info;

void device_init();

void device_close();

void read_data_from_devs(void* dst, int64_t len, int64_t offset);

void write_data_to_devs(void* src, int64_t len, int64_t offset);

void shm_write_data_to_devs(void* src, int64_t src_off, int64_t len, int64_t offset);

#endif
