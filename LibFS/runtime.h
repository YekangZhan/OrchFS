#ifndef LIB_RUNTIME_H
#define LIB_RUNTIME_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> 
#include <inttypes.h>
#include <pthread.h>
#include <dirent.h>

// #include "../config/config.h"

#define ORCH_MAX_FD                 (1024*64)
#define MAX_OPEN_INO_NUM           (1024*16)

#define FD_USED                    1
#define FD_NOT_USED                0

#define INOT_USED                  1
#define INOT_NOT_USED              0

#define bit_lock_t                 uint64_t

#define FETCH_AND_unSET_BIT(addr, num)                              \
__sync_and_and_fetch((char*) (((uint64_t)addr)+(num/8)),      \
~((char)0x01 << (num&0b111))) 

/* File descriptor
 */
struct orch_fd_t
{
    int                     open_inot_idx;            
    int                     used_flag;                   
    int64_t                 rw_offset;          
    struct dirent           temp_dirent;
    int                     flags;
};
typedef struct orch_fd_t orch_fd_t;
typedef orch_fd_t* orch_fd_pt;

struct open_inode_info
{
    struct orch_inode*       inode_pt;                 
    int64_t                  inode_id;               
    int         	         used_flag;                 
    int                      open_mode;                    // Open mode
    int                      fd_link_count;           
    pthread_rwlock_t         file_rw_lock;                 // Read/write lock for files
};
typedef struct open_inode_info open_ino_info_t;
typedef open_ino_info_t* open_ino_info_pt;

struct orchfs_runtime
{

    void* log_start_pt;
    void* bufmeta_start_pt;


    int64_t current_dir_inoid;
    int64_t mount_pt_inoid;

    // open Inode table
    struct static_hashtable* inoid_to_inotidx;
    open_ino_info_t open_inot[MAX_OPEN_INO_NUM];
    orch_fd_t fd_info[ORCH_MAX_FD];
    int now_fd_cur;
    int now_inot_cur;

    // Request locks for blocks from the kernel
    pthread_spinlock_t req_kernel_lock;               

    pthread_spinlock_t fd_lock;            
    pthread_spinlock_t inot_lock;                       

    /*The lock of directory cache*/
    pthread_rwlock_t  dir_cache_lock;                   
    struct dir_cache* dir_to_inoid_cache;             

    struct migrate_info_t* mig_rtinfo_pt;           
    
    int64_t index_time;
    int64_t io_time;
    int64_t pm_unit_rwsize;
    int64_t pm_page_rwsize;
    int64_t wback_time;
    int64_t pm_time;
    int64_t blk_time;

    int64_t used_pm_pages;
    int64_t used_pm_units;
    int64_t used_ssd_blks;
    int64_t used_tree_nodes;
    int64_t used_vir_nodes;
    int64_t wback_num;
};
typedef struct orchfs_runtime orchfs_rt_t;
typedef orchfs_rt_t* orchfs_rt_pt;

orchfs_rt_t orch_rt;

static void bitlock_acquire(bit_lock_t* bitlock, int lock_pos)
{
    uint64_t * target = bitlock + (lock_pos / 64);
    uint64_t offset = lock_pos % 64;
    while((((uint64_t)0b01 << offset) & __sync_fetch_and_or(target, (uint64_t)0b01 << offset)) != 0){
        // pthread_yield();
    }
}

static void bitlock_release(bit_lock_t* bitlock, int unlock_pos)
{
    FETCH_AND_unSET_BIT(bitlock, unlock_pos);
}

static void orch_time_stamp(struct timespec * time)
{
    clock_gettime(CLOCK_REALTIME, time);
}


void init_runtime_info();

void free_runtime_info();


void change_current_dir_ino(int64_t root_ino_id, int64_t inode_id);


void file_lock_rdlock(int fd);


void file_lock_wrlock(int fd);

void file_lock_unlock(int fd);


static void reqlock_lock()
{
    pthread_spin_lock(&(orch_rt.req_kernel_lock));
    // printf("lock\n");
}

static void reqlock_unlock()
{
    pthread_spin_unlock(&(orch_rt.req_kernel_lock));
    // printf("unlock\n");
}



struct orch_inode* fd_to_inodept(int fd);


int64_t fd_to_inodeid(int fd);


int64_t get_fd_file_offset(int fd);


void change_fd_file_offset(int fd, int64_t changed_offset);


int64_t get_unused_fd(int64_t inode_id, int mode);


void release_fd(int fd);

#endif