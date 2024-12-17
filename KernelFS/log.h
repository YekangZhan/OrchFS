#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "../config/log_config.h"

// #define MAX_OPEN_FILE                  1024


struct kernel_log_meta
{
    int64_t used_flag;                    
    int64_t submit_end_off;              
    int64_t sync_len;                     
    int64_t last_submit_time;             
    int64_t not_writed_len;               
    void* klog_data_sp;                 
};
typedef struct kernel_log_meta klog_meta_t;
typedef klog_meta_t* klog_meta_pt;

klog_meta_pt kmem_log_meta;
int64_t log_seg_num;

struct orch_array_t* file_log_index[MAX_OPEN_FILE];
struct static_hashtable* inoid_to_flidx;


void init_kernel_log();


int64_t alloc_log_segment_from_dev();


void sync_file_log(int64_t inode_id);


void submit_part_log_segment(int64_t seg_id, int64_t start_addr, int64_t submit_len);


void submit_log_segment(int64_t seg_id);


void close_kernel_log();

#endif