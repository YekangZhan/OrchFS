#ifndef META_CACHE_H
#define META_CACHE_H

#include <stdint.h>
#include <stdio.h>
#include <malloc.h>
#include <inttypes.h>
#include <pthread.h>
#include <assert.h>


// Maximum cache of 4096 files for metadata
#define MAX_CACHE_FILES                1024


#define INODE_CODE                     (0LL << 48)
#define INDEX_CODE                     (1LL << 48)
#define VIR_INDEX_CODE                 (2LL << 48)
#define BUFMETA_CODE                   (3LL << 48)

// max block per cache extent
#define INODE_CACHE_EXTBLKS            128
#define IDXND_CACHE_EXTBLKS            256
#define VIRND_CACHE_EXTBLKS            128
#define BUFMETA_CACHE_EXTBLKS          128

// cache id
#define MIN_CACHE_ID                   0
#define INODE_CACHE                    0
#define IDXND_CACHE                    1
#define VIRND_CACHE                    2
#define MAX_CACHE_ID                   2
// #define BUFMETA_CACHE                  3


// valid flag
#define INVALID                        0
#define VALID                          1
#define CACHE_CLOSE                    2

// create flag
#define CREATE                         1
#define NOT_CREATE                     0

// meta_cache_t full
#define FULL                           1
#define NOT_FULL                       0

struct cache_data_t
{
    int64_t seg_blknum;                                  
    int64_t unit_size;                                    
    int64_t seg_num;                                   
    int32_t* sync_flist;                                  
    char** meta_memsp_pt;                                
    pthread_spinlock_t* seg_lock_pt;                 
};
typedef struct cache_data_t cache_data_t;
typedef cache_data_t* cache_data_pt;

cache_data_t cache_data[MAX_CACHE_ID+1];



void init_metadata_cache();


void close_metadata_cache();


void sync_inode(int64_t inode_id);

void sync_index_blk(int64_t indexid);

void sync_virnd_blk(int64_t virnodeid);



void create_file_metadata_cache(int64_t inode_id);


void close_file_metadata_cache(int64_t inode_id);


void delete_file_metadata_cache(int64_t inode_id);



void* inodeid_to_memaddr(int64_t inodeid);


void* indexid_to_memaddr(int64_t inodeid, int64_t indexid, int create_flag);


void* virnodeid_to_memaddr(int64_t inodeid, int64_t virnodeid, int create_flag);


// void* bufmetaid_to_memaddr(int64_t inodeid, int64_t bufmetaid, int create_flag);


int64_t bufmetaid_to_devaddr(int64_t buf_meta_id);


int64_t nvmpage_to_devaddr(int64_t nvm_page_id);


int64_t ssdblk_to_devaddr(int64_t ssd_blk_id);



#endif