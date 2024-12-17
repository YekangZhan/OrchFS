#ifndef EXEC_IO_THDPOOL_H
#define EXEC_IO_THDPOOL_H

// 内核线程池包括三个监听线程，两个发送线程
// 还有若干的执行线程，执行线程包括每一个分配和回收一个线程，消化日志一个线程
// 其中，分配线程还需要调用内核的发送函数

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> 
#include <malloc.h>

//线程数统计
#define POLLING_THREADS                  1
#define EXEC_THREADS                     32
#define TOTAL_EXEC_THREADS               (POLLING_THREADS + EXEC_THREADS)

// 错误信息
#define TYPE_ERROR                       1
#define TYPE_NOT_ERROR                   0

#define EXEC_READ                        0
#define EXEC_WRITE                       1

struct ker_io_task
{
    int64_t io_start_addr;                         
    int64_t io_len;                                 
    int64_t data_buf_off;                            
    int64_t data_buf_len;                          
    void* io_data_buf;                             
    int io_type;                                    
    struct io_task* next;                          
};
typedef struct ker_io_task ker_io_task_t;
typedef ker_io_task_t* ker_io_task_pt;

struct ker_io_pth_pool
{   
    ker_io_task_pt task_list_head[EXEC_THREADS];                
    ker_io_task_pt task_list_end[EXEC_THREADS];                
    sem_t task_list_sem[EXEC_THREADS];                          

    pthread_t* tid_info_pt;                                   
	int shutdown;                                             
    sem_t shutdown_sem;
};
typedef struct ker_io_pth_pool ker_io_pth_pool_t;
typedef ker_io_pth_pool_t* ker_io_pth_pool_pt;

struct ker_io_pth_frame
{
    int64_t tid;
    ker_io_pth_pool_pt pool;
};
typedef struct ker_io_pth_frame ker_io_pth_frame_t;
typedef ker_io_pth_frame_t* ker_io_pth_frame_pt;


ker_io_pth_pool_t orch_io_exec;
ker_io_pth_frame_t kio_frames[TOTAL_EXEC_THREADS];


void init_ker_io_thread_pool();


void destroy_ker_io_thread_pool();


void ker_add_io_task(int queue_id, int task_type, void* data_sp, int64_t len, int64_t offset);

#endif