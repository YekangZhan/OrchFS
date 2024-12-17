#ifndef MIGRATE_H
#define MIGRATE_H

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

#define DEFAULT_MIGRATE_NUM                         1024
#define MIGRATE_PERCENTAGE                          10
#define CAN_USE_PERCENTAGE                          10

#define DO_MIGRATE                                  1
#define SLEEP                                       0 

struct LRU_node_info_t
{
    int64_t ino_id;
    int64_t offset;
    int64_t nvm_page_addr[10];
};
typedef struct LRU_node_info_t LRU_node_info_t;
typedef LRU_node_info_t* LRU_node_info_pt;

struct migrate_info_t
{
    struct LRU_t* LRU_info;                     // LRU
    int64_t migrate_num;                        
    int64_t nvm_page_used;          
    int64_t all_nvm_page;                  
    int64_t can_use_page_num;             
    int64_t mig_threshold;                      // nvm page threshold for migration
    int64_t mig_state;          
    struct timespec last_migrate_time;    

    int64_t all_mig_blk;
    int64_t max_page_use;
};
typedef struct migrate_info_t migrate_info_t;
typedef migrate_info_t* migrate_info_pt;


migrate_info_pt init_migrate_info();


void add_migrate_node(migrate_info_pt mig_info, struct offset_info_t* off_info, 
                    int64_t ino_id, int64_t new_page_num);

int do_migrate_operation(migrate_info_pt mig_info);


void* wait_and_exec_migrate(void* para_arg);


void free_migrate_info(migrate_info_pt mig_info_pt);

#endif