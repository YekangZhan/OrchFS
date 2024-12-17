#ifndef RADIXTREE_H
#define RADIXTREE_H

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <malloc.h>
#include <time.h>

#define RDTREE_SON_NUM              128

#define DIRTY                       1
#define NOT_DIRTY                   0

struct radix_node
{
    struct radix_node* next_level_pt[128];
    void* value;
    int64_t value_len;
    int64_t dirty_flag;
};
typedef struct radix_node radix_node_t;
typedef radix_node_t* radix_node_pt;

/* init a radix tree
 */
radix_node_pt init_radix_tree();

/* insert an element to the radix tree
 * @root:                 The root node of the radix tree
 * @key:                  The key inserted into the index
 * @key_len:              The length of the key
 * @value:                The value inserted into the index
 * @value_len:            The length of the value
 * @return                
 */
void insert_kv_into_rdtree(radix_node_pt root, void* key, int64_t key_len, 
                            void* value, int64_t value_len);

/* 向radix tree查找元素
 * @root:                 The root node of the radix tree
 * @key:                  The key inserted into the index
 * @key_len:              The length of the key
 * @return                
 */
void* search_from_rdtree(radix_node_pt root, void* key, int64_t key_len);

/* 从radix tree中删除元素
 * @root:                 The root node of the radix tree
 * @key:                  The key inserted into the index
 * @key_len:              The length of the key
 * @return                
 */
void delete_kv_from_rdtree(radix_node_pt root, void* key, int64_t key_len);

/* 改变radix tree的value
 * @root:                 The root node of the radix tree
 * @key:                  The key inserted into the index
 * @key_len:              The length of the key
 * @value:                The value inserted into the index
 * @value_len:            The length of the value
 * @return                
 */
void change_kv_in_rdtree(radix_node_pt root, void* key, int64_t key_len, 
                        void* value, int64_t value_len);

/* 释放radix tree
 * @rdtree_root:
 */
void free_radix_tree(radix_node_pt rdtree_root);

#endif