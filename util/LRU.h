#ifndef LRU_H
#define LRU_H

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
#include <malloc.h>

#define DEFAULT_LRU_MAX_ITEM                            (1024 * 24)

#define LRU_FULL                           1
#define LRU_NOT_FULL                       0
#define CAN_NOT_INSERT                     -1

struct LRU_node_t
{
    int64_t key;                                        
    void* value;                                        
    int64_t value_len;                                  
    struct LRU_node_t* pre_pt;                           
    struct LRU_node_t* next_pt;                          
};
typedef struct LRU_node_t LRU_node_t;
typedef LRU_node_t* LRU_node_pt;

struct LRU_t
{
    int64_t LRU_item_num;                               
    int64_t max_LRU_item;                                
    struct static_hashtable* key_to_lrund;               
    LRU_node_pt LRU_list_head;                           
    LRU_node_pt LRU_list_end;                          
    pthread_spinlock_t lru_lock;                       
};
typedef struct LRU_t LRU_t;
typedef LRU_t* LRU_pt;


LRU_pt create_LRU_module(int64_t max_LRU_item);


void free_LRU_module(LRU_pt free_lru);


int add_LRU_node(LRU_pt now_lru, int64_t LRU_key, void* value, int64_t value_len);


void* get_eliminate_LRU_node(LRU_pt now_lru);


void* get_and_eliminate_LRU_node(LRU_pt now_lru);

#endif