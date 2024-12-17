#ifndef ALLOC_H
#define ALLOC_H

#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

// inode type id
#define START_BMP_ID         1
#define INODE_BMP            1
#define IDX_NODE_BMP         2
#define VIR_NODE_BMP         3
#define BUFMETA_BMP          4
#define PAGE_BMP             5
#define BLOCK_BMP            6
#define END_BMP_ID           6

// return type
#define RET_BLK_ID           0
#define RET_BLK_ADDR         1

// parameter type
#define PAR_BLK_ID           0
#define PAR_BLK_ADDR         1

#define MEM_PAGE_SIZE        4096

// The bitmap structure in memory
struct memory_bitmap_info_t
{
    void* bmp_start_pt;                     // Bitmap information in memory
    // read from super block
    uint64_t dev_start_addr;                // The starting position of the bitmap in the device
    uint64_t bmp_alloc_cur;                 // Currently allocated location
    uint64_t bmp_used_num;                  // The number of bits already used
    uint64_t bmp_alloc_range;               // The currently allocated right boundary
    // read from config
    uint64_t bmp_capacity;                  // Capacity of Bitmap
    uint64_t blk_area_start;                    
    uint64_t blk_size;                     
};
typedef struct memory_bitmap_info_t mem_bmp_t;
typedef mem_bmp_t* mem_bmp_pt;

mem_bmp_t mem_bmp_arr[END_BMP_ID + 1]; 

void init_mem_bmp();


uint32_t sync_type_mem_bmp(int32_t bmp_type);


uint32_t sync_all_mem_bmp();


uint32_t sync_mem_bmp(int32_t bmp_type, int64_t bit_off, int64_t bit_len);


uint32_t delete_mem_bmp();

/*---------------------------------inode----------------------------------*/
/* alloc nvm page
 * @return_type:        Type of return value, block ID or device address
 * @return              
 */
uint64_t alloc_single_inode(int return_type);

/* alloc many inodes
 * @alloc_blks:         The number of blocks that need to be allocated
 * @addr_list:          Data structure for storing allocation block information
 * @return_type:        Type of return value, block ID or device address
 * @return              
 */
void alloc_inodes(int64_t alloc_blk_num, uint64_t addr_list[], int return_type);

/* dealloc an inode
 * @dealloc_blk_info:   The address or block ID of the block that needs to be released
 * @par_type:           Type of parameter, block ID or device address
 * @return               
 */
void dealloc_inode(uint64_t dealloc_blk_info, int par_type);

/*---------------------------------index----------------------------------*/
/* alloc an index node
 * @return_type:        Type of parameter, block ID or device address
 * @return              
 */
uint64_t alloc_single_idx_node(int return_type);

/* alloc many index nodes
 * @alloc_blks:         The number of blocks that need to be allocated
 * @addr_list:          Data structure for storing allocation block information
 * @return_type:        Type of return value, block ID or device address
 * @return               
 */
void alloc_idx_nodes(int64_t alloc_blk_num, uint64_t addr_list[], int return_type);

/* dealloc a index node
 * @dealloc_blk_info:   The address or block ID of the block that needs to be released
 * @par_type:           Type of parameter, block ID or device address
 * @return              
 */
void dealloc_idx_node(uint64_t dealloc_blk_info, int par_type);

/*------------------------------vir index--------------------------------*/
/* alloc a virtural index node
 * @return_type:        Type of return value, block ID or device address
 * @return              
 */
uint64_t alloc_single_viridx_node(int return_type);

/* alloc many virtural index nodes
 * @alloc_blks:         The number of blocks that need to be allocated
 * @addr_list:          Data structure for storing allocation block information
 * @return_type:        Type of return value, block ID or device address
 * @return              
 */
void alloc_viridx_nodes(int64_t alloc_blk_num, uint64_t addr_list[], int return_type);

/* dealloc a virtural index node
 * @dealloc_blk_info:   The address or block ID of the block that needs to be released
 * @par_type:           Type of parameter, block ID or device address
 * @return              
 */
void dealloc_viridx_node(uint64_t dealloc_blk_info, int par_type);

/*-------------------------------buf meta--------------------------------*/
/* alloc a buffer metadata node
 * @return_type:        Type of return value, block ID or device address
 * @return              
 */
uint64_t alloc_single_bufmeta_node(int return_type);

/* alloc many buffer metadata nodes
 * @alloc_blks:         The number of blocks that need to be allocated
 * @addr_list:          Data structure for storing allocation block information
 * @return_type:        Type of return value, block ID or device address
 * @return              
 */
void alloc_bufmeta_nodes(int64_t alloc_blk_num, uint64_t addr_list[], int return_type);

/* dealloc a buffer metadata node
 * @dealloc_blk_info:   The address or block ID of the block that needs to be released
 * @par_type:           Type of parameter, block ID or device address
 * @return               
 */
void dealloc_bufmeta_node(uint64_t dealloc_blk_info, int par_type);

/*----------------------------------nvm-----------------------------------*/
/* alloc nvm page
 * @return_type:        Type of return value, block ID or device address
 * @return              
 */
uint64_t alloc_single_nvm_page(int return_type);

/* alloc many nvm pages
 * @alloc_blks:         The number of blocks that need to be allocated
 * @addr_list:          Data structure for storing allocation block information
 * @return_type:        Type of return value, block ID or device address
 * @return               
 */
void alloc_nvm_pages(int64_t alloc_blk_num, uint64_t addr_list[], int return_type);

/* dealloc a nvm page
 * @dealloc_blk_info:   The address or block ID of the block that needs to be released
 * @par_type:           Type of return value, block ID or device address
 * @return              
 */
void dealloc_nvm_page(uint64_t dealloc_blk_info, int par_type);

/*----------------------------------ssd-----------------------------------*/
/* alloc ssd block
 * @return_type:        Type of return value, block ID or device address
 * @return              
 */
uint64_t alloc_single_ssd_block(int return_type);

/* alloc many ssd blocks
 * @alloc_blks:         The number of blocks that need to be allocated
 * @addr_list:          Data structure for storing allocation block information
 * @return_type:        Type of return value, block ID or device address
 * @return              
 */
void alloc_ssd_blocks(int64_t alloc_blk_num, uint64_t addr_list[], int return_type);

/* dealloc a ssd block
 * @dealloc_blk_info:   The address or block ID of the block that needs to be released
 * @par_type:           Type of parameter, block ID or device address
 * @return               
 */
void dealloc_ssd_block(uint64_t dealloc_blk_info, int par_type);


#endif