#ifndef INDEX_H
#define INDEX_H

#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#include "../config/config.h"

// node type code
#define IDX_ROOT                      0
#define NOT_LEAF_NODE                 1
#define LEAF_NODE                     2
#define VIR_LEAF_NODE                 3
#define STRATA_NODE                   4
#define SSD_BLOCK                     5
#define EMPTY_BLK_TYPE                -1
#define EMPTY_BLKID                   -1

// #define INDEX_LAYER                   6
// #define LAYER_BITWIDTH                6
// #define MAX_ZIPPED_LAYER              6
// #define KEY_LEN                       42

#define LAYER_BITWIDTH                7
#define MAX_ZIPPED_LAYER              5
#define KEY_LEN                       42
#define NODE_SON_CAPACITY             (1<<LAYER_BITWIDTH)
#define VLN_SLOT_BITWIDTH             (ORCH_BLOCK_BW-ORCH_PAGE_BW)
#define VLN_SLOT_SUM                  (1<<VLN_SLOT_BITWIDTH)

#define root_id_t                     int64_t
#define index_id_t                    int64_t
#define viridx_id_t                   int64_t
#define ssd_blk_id_t                  int64_t
#define nvm_page_id_t                 int64_t
#define bufmeta_id_t                  int64_t

#define LOCK_INIT                     1
#define LOCK_NOT_INIT                 0

#define MAX_BUF_SEG_NUM               12

struct index_node_t
{  
    int64_t ndtype;                                     // Node type: leafnode, index root...
    int64_t zipped_layer;                               // The number of compressed layers in the index
    uint64_t virnd_flag[2];                             // Is the son node a virtual leaf node
    uint64_t bit_lock[2];                               // bit lock
    int64_t son_blk_id[NODE_SON_CAPACITY];              // The block ID of the son node
};
typedef struct index_node_t idx_nd_t;
typedef idx_nd_t* idx_nd_pt;

struct index_root_t
{  
    int64_t ndtype;                                     // Node type
    int64_t idx_inode_id;                               // The inode corresponding to this index
    int64_t idx_entry_blkid;                            // The entrance to the index
    int64_t max_blk_offset;                                     
};
typedef struct index_root_t idx_root_t;
typedef idx_root_t* idx_root_pt;

struct virtual_node_t
{  
    uint64_t ndtype;                                    // Node type
    uint64_t ssd_dev_addr;                              // Block address on SSD
    int64_t max_pos;                                    
    int64_t nvm_page_id[VLN_SLOT_SUM];                  // NVM page ID used for caching
    int64_t buf_meta_id[VLN_SLOT_SUM];                  // The metadata of the NVM page
};
typedef struct virtual_node_t vir_nd_t;
typedef vir_nd_t* vir_nd_pt;

struct offset_info_t
{
    int64_t ndtype;                                     // Node type
    int64_t ssd_dev_addr;                               // Block address on SSD
    int64_t nvm_page_id[VLN_SLOT_SUM];                  
    int64_t buf_meta_id[VLN_SLOT_SUM];                  
    int64_t offset_ans;                                 // file offset
};
typedef struct offset_info_t off_info_t;
typedef off_info_t* off_info_pt;


root_id_t create_index(int64_t inode_id);


int append_single_ssd_block(root_id_t root_id, int64_t inode_id, ssd_blk_id_t ssd_blk_id);


int append_ssd_blocks(root_id_t root_id, int64_t inode_id, 
                        int64_t app_blk_num, ssd_blk_id_t ssd_blk_id_arr[]);

int append_single_nvm_page(root_id_t root_id, int64_t inode_id, nvm_page_id_t nvm_page_id);


int append_nvm_pages(root_id_t root_id, int64_t inode_id, 
                    int32_t app_blk_num, nvm_page_id_t nvm_page_id_arr[]);


off_info_t query_offset_info(root_id_t root_id, int64_t inode_id, int64_t blk_offset);


void change_ssd_blk_info(root_id_t root_id, int64_t inode_id, 
                        int64_t blk_offset, int64_t changed_blkid);


void change_nvm_page_info(root_id_t root_id, int64_t inode_id, 
                        int64_t blk_offset, int32_t pos, int64_t changed_pageid);


void change_virnd_to_ssdblk(root_id_t root_id, int64_t inode_id, int64_t blk_offset, 
                            int64_t changed_blkid);


int insert_strata_page_and_metabuf(root_id_t root_id, int64_t inode_id, int64_t blk_offset, 
                                int32_t pos, nvm_page_id_t nvm_page_id, bufmeta_id_t buf_id);


void delete_all_index(root_id_t root_id, int64_t inode_id);


void delete_part_index(root_id_t root_id, int64_t inode_id, int64_t blk_offset);


void lock_range_lock(root_id_t root_id, int64_t inode_id, int64_t blk_start_off, int64_t blk_end_off);


void unlock_range_lock(root_id_t root_id, int64_t inode_id, int64_t blk_start_off, int64_t blk_end_off);


void sync_all_index(idx_nd_pt now_idx_pt, int64_t inode_id);



#endif