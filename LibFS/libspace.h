/* manage libfs metadata */
#ifndef LIBSPACE_H
#define LIBSPACE_H

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <inttypes.h>

// block type
#define MEM_ADDRESS      0
#define SSD_ADDRESS      1
#define NVM_ADDRESS      2

// max block per extent
#define MAX_INODE_EXT_SIZE           256
#define MAX_IDXND_EXT_SIZE           (1024*2)
#define MAX_VIRND_EXT_SIZE           (1024*4)
#define MAX_BUFMETA_EXT_SIZE         (1024*2)
#define MAX_PAGE_EXT_SIZE            (1024*15)
#define MAX_BLOCK_EXT_SIZE           (1024*15)

#define MAX_DEALLOC_BLK              1024

#define MIN_EXT_ID                   1
#define INODE_EXT                    1
#define IDXND_EXT                    2
#define VIRND_EXT                    3
#define BUFMETA_EXT                  4
#define PAGE_EXT                     5
#define BLOCK_EXT                    6
#define MAX_EXT_ID                   6

// return type
#define RET_BLK_ID           0
#define RET_BLK_ADDR         1

// parameter type
#define PAR_BLK_ID           0
#define PAR_BLK_ADDR         1

struct alloc_extent_info_t
{
    int64_t ext_blk_capacity;                 
    int64_t used_blk_num;                     
    int64_t blk_type;                         
    int64_t last_cur;                         
    int64_t* blk_id_arr;                      
    int32_t* blk_used_flag_arr;               
    pthread_spinlock_t alloc_lock;                 
};
typedef struct alloc_extent_info_t alloc_ext_info_t;
typedef alloc_ext_info_t* alloc_ext_info_pt;

struct dealloc_extent_info_t
{
    int64_t ext_blk_capacity;                  
    int64_t dealloc_blk_num;               
    int64_t blk_type;                        
    int64_t* blk_id_arr;                      
    pthread_spinlock_t dealloc_lock;                  
};
typedef struct dealloc_extent_info_t dealloc_ext_info_t;
typedef dealloc_ext_info_t* dealloc_ext_info_pt;

alloc_ext_info_t lib_alloc_info[MAX_EXT_ID+1];
dealloc_ext_info_t lib_dealloc_info[MAX_EXT_ID+1];
int64_t ext_size_arr[MAX_EXT_ID+1];



int init_all_ext();


int return_all_ext();




int64_t require_inode_id();


int64_t require_index_node_id();


int64_t require_virindex_node_id();


int64_t require_buffer_metadata_id();


int64_t require_nvm_page_id();


int64_t require_ssd_block_id();




void release_inode(int64_t inode_id);


void release_index_node(int64_t idx_id);


void release_virindex_node(int64_t virnd_id);


void release_buffer_metadata(int64_t buf_id);


void release_nvm_page(int64_t page_id);


void release_ssd_block(int64_t block_id);


#endif