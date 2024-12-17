#ifndef LIB_LOG_H
#define LIB_LOG_H


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "runtime.h"

// lib log主要是为用户态进程提供写入日志的接口
// 一种是写入一定数量的日志然后提交
// 还有一种是每一个操作的日志都会被提交

// 每一个操作都对应的有编号，提交就是往设备上写入，在用户态直接执行
// 操作写入之后需要加尾部，没有尾部的操作是无效的

// 需要的接口，增加日志
// 提交日志
// 手动提交同步请求
// 内部函数需要申请释放日志区的空间

#define MEMLOG_SPACE_SIZE           (1024*1024*8)
#define MEMLOG_HEAD_SIZE            8

#define SYNC_LOG_MODE               1
#define NOT_SYNC_LOG_MODE           0

#define LOG_SPACE_USED              1
#define LOG_SPACE_NOT_USED          0

struct log_memspace
{
    int64_t log_len;             
    int64_t used_flag;                     
    void* log_space;                       
    void* now_write_pt;                    
};
typedef struct log_memspace log_msp_t;
typedef log_msp_t* log_msp_pt;

struct log_device_space
{
    int64_t dev_log_id;                     
    int64_t dev_log_baseaddr;              
    int64_t dev_log_woffset;               
    int64_t dev_log_wpt;                     
    int64_t dev_log_wrange;                  

    pthread_spinlock_t wlog_lock;            
};
typedef struct log_device_space log_dev_sp_t;
typedef log_dev_sp_t* log_dev_sp_pt;

log_dev_sp_t dev_log_info;                     
log_msp_t log_per_op[ORCH_MAX_FD];                
int memlog_alloc_cur;                        


void init_mem_log();


void free_mem_log();


int regist_log_memspace();


void return_log_memspace(int log_space_id);


void memlog_to_dev(int log_space_id);

void write_create_log(int64_t create_blkid, int blk_type);

void write_delete_log(int64_t delete_blkid, int blk_type);

void write_change_log(int64_t change_blkid, int blk_type, 
                        void* log_data, int32_t off, int32_t len);

#endif