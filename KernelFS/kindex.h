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
#define VLN_SLOT_BITWIDTH             2
#define VLN_SLOT_SUM                  (1<<VLN_SLOT_BITWIDTH)
// 索引本身最多42位,数据块最多支持4MB,组合起来就是64位

#define root_id_t                     int64_t
#define index_id_t                    int64_t
#define viridx_id_t                   int64_t
#define ssd_blk_id_t                  int64_t
#define nvm_page_id_t                 int64_t
#define bufmeta_id_t                  int64_t

#define LOCK_INIT                     1
#define LOCK_NOT_INIT                 0

#define MAX_BUF_SEG_NUM               12

struct ker_index_node_t
{  
    int64_t ndtype;                                     // Node type: leafnode, index root...
    int64_t zipped_layer;                               // The number of compressed layers in the index
    uint64_t virnd_flag[2];                             // Is the son node a virtual leaf node
    uint64_t bit_lock[2];                               // bit lock
    int64_t son_blk_id[NODE_SON_CAPACITY];              // The block ID of the son node
};
typedef struct ker_index_node_t ker_idx_nd_t;
typedef ker_idx_nd_t* ker_idx_nd_pt;

struct ker_index_root_t
{  
    int64_t ndtype;                                    
    int64_t idx_inode_id;                          
    int64_t idx_entry_blkid;                      
    int64_t max_blk_offset;                                   
};
typedef struct ker_index_root_t ker_idx_root_t;
typedef ker_idx_root_t* ker_idx_root_pt;

struct ker_virtual_node_t
{  
    uint64_t ndtype;                            
    uint64_t ssd_dev_addr;                        
    int64_t max_pos;                               
    int64_t nvm_page_id[VLN_SLOT_SUM];                 
    int64_t buf_meta_id[VLN_SLOT_SUM];                 
};
typedef struct ker_virtual_node_t ker_vir_nd_t;
typedef ker_vir_nd_t* ker_vir_nd_pt;

struct ker_offset_info_t
{
    int64_t ndtype;                                   
    int64_t ssd_dev_addr;                               
    int64_t nvm_page_id[VLN_SLOT_SUM];                   
    int64_t buf_meta_id[VLN_SLOT_SUM];       
    int64_t offset_ans;
};
typedef struct ker_offset_info_t ker_off_info_t;
typedef ker_off_info_t* ker_off_info_pt;

root_id_t create_index_with_page(int64_t inode_id, nvm_page_id_t page_id);


#endif