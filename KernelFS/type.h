#ifndef ORCHFS_TYPE_H
#define ORCHFS_TYPE_H

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

#include "../config/config.h"


#define ORCHK_MAX_NAME             230
#define KDIRENT_SIZE              256

#define KINVALID_T                0x00
#define KDIRENT_FILE_T            0x01
#define KSIMPLE_FILE_T            0x02

#define KER_DIR_FILE                   0b00000001
#define KER_SIMPLE_FILE                0b00000010

struct orch_kfs_dirent
{
    int64_t d_ino;                   //8B
    int64_t d_off;                    //8B
    signed short int d_namelen;         //2B
    int8_t d_type;                      //1B
    char d_name[ORCHK_MAX_NAME + 1];
};
typedef struct orch_kfs_dirent orch_kfs_dirent_t;
typedef orch_kfs_dirent_t* orch_kfs_dirent_pt;

/* inode.
 */
struct orch_kfs_inode
{
	// 64-bit fields
    int64_t        i_number;             // inode id
    int64_t        i_size;               // file size
    int64_t        i_idxroot;            // the address of the index root
    int64_t        i_nlink;              

	// 32-bit fields
    uid_t          i_uid;                
    gid_t          i_gid;                
    mode_t         i_mode;               
    int32_t        i_type;               // the type of the file

    // times
    struct timespec i_atim;              /* Time of last access */
    struct timespec i_mtim;              /* Time of last modification */
    struct timespec i_ctim;              /* Time of last status change */
    //56B

    //char        padding[16];
    pthread_spinlock_t     i_lock;
};
typedef struct orch_kfs_inode orch_kfs_inode_t;
typedef orch_kfs_inode_t* orch_kfs_inode_pt;

/* Super block
 * This struct is in pmm.
 * Size shall be less than 512B.
 */
struct orch_super_blk{
    // 64-bit
    uint64_t    root_inode;
    int64_t     magic_num[3];

    // start of inode, kept in one cacheline, 256-bit
    int64_t      bmp_dev_addr_list[8];                       // The start address of the bitmap
    int64_t      bmp_alloc_cur_list[8];                      // The allocate point
    int64_t      bmp_used_num_list[8];                       // The bit used
    int64_t      bmp_alloc_range_list[8];                    // Allocate range
};
typedef struct orch_super_blk orch_super_blk_t;
typedef orch_super_blk_t* orch_super_blk_pt;

extern int now_registered_pid;

int64_t kernel_inode_create(int i_type);

static void korch_time_stamp(struct timespec * time)
{
    clock_gettime(CLOCK_REALTIME, time);
}


#endif
