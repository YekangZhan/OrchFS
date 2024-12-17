#ifndef KERNEL_THD_H
#define KERNEL_THD_H

/* The kernelFS thread pool includes three listening threads and two sending threads*/

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> 
#include <malloc.h>

// The ID of each thread
#define RECV_START_TID                   0
#define RECV_ALLOC_OP_TID                0
#define RECV_DEALLOC_OP_TID              1
#define RECV_LOG_OP_TID                  2
#define RECV_CACHE_OP_TID                3
#define RECV_REGISTER_TID                4
#define RECV_END_TID                     4

#define OP_START_TID                     5
#define ALLOC_TID                        5
#define DEALLOC_TID                      6
#define LOG_TID                          7
#define CACHE_OP_TID                     8
#define RIGESTER_TID                     9
#define OP_ENDT_TID                      9

// The thread num
#define RECV_THREADS                     5
#define OP_THREADS                       5
#define TOTAL_THREADS                    10

// erro info
#define TYPE_ERROR                       1
#define TYPE_NOT_ERROR                   0

struct thread_task_t
{
    void* para_space;                 // The space to store parameter
    int64_t func_type;                // The type of the func
    int64_t error_flag;
    struct thread_task_t* next;
};
typedef struct thread_task_t thd_task_t;
typedef thd_task_t* thd_task_pt;

struct pth_frame_t
{
    int64_t tid;
};
typedef struct pth_frame_t pth_frame_t;

struct kernel_thread_pool 
{
	int shutdown;                            
    int break_thd_num;                       
    sem_t destroy_flag;                      

    pthread_t* recv_thd_info;                            

    pthread_t* op_thd_info;                              
    thd_task_pt task_list_head[OP_THREADS];              
    thd_task_pt task_list_end[OP_THREADS];               
    pthread_spinlock_t task_app_lock[OP_THREADS];        
    sem_t task_queue_sem[OP_THREADS];                    
    int task_num_inq[OP_THREADS];                        
};
typedef struct kernel_thread_pool kth_pool_t;
typedef kth_pool_t* kth_pool_pt;

kth_pool_t orch_kth_pool;
pth_frame_t frames[TOTAL_THREADS];

int init_kernel_thd_pool();

void destroy_kernel_thd_pool();

void add_task_to_queue(int queue_id, thd_task_pt add_info_pt);


#endif 