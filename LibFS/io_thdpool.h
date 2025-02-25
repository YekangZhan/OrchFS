#ifndef LIB_THDPOOL_H
#define LIB_THDPOOL_H

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> 
#include <inttypes.h>

#include "../config/config.h"


#define GROUP_USE                  1
#define GROUP_NOT_USE              0

#define MIGRATE_THREADS            1
#define MAX_NVM_THREADS            ORCH_CONFIG_NVMTHD
#define MAX_SSD_THREADS            ORCH_CONFIG_SSDTHD
#define IO_TOTAL_THREADS           ORCH_CONFIG_SSDTHD
#define MAX_TASK_GROUP_NUM         4096

#define READ_OP                    0b0001
#define WRITE_OP                   0b0010
#define STRATA_OP                  0b0100

#define dev_addr_t                 int64_t

struct io_task
{
    int64_t task_group_id;                           // the task group to which the request belongs
    dev_addr_t io_start_addr;                        // target address on the device
    int64_t io_len;                                  // io length
    void* io_data_buf;                               // io buffer
    int io_type;                                     // io type: read/write
    struct io_task* next;                            
};
typedef struct io_task io_task_t;
typedef io_task_t* io_task_pt;

struct io_task_group
{
    pthread_spinlock_t task_done_lock[IO_TOTAL_THREADS]; 
    int task_num[IO_TOTAL_THREADS];          
    int used_flag;                                          
};
typedef struct io_task_group io_task_group_t;
typedef io_task_group_t* io_task_group_pt;

struct io_pthread_pool
{
	/* The purpose of using "thread pool" in nvm is
     to control the number of concurrent accesses*/
    sem_t nvm_rw_sem;
    int alloc_thd_cur;
    
    // task queue
    io_task_pt task_list_head[IO_TOTAL_THREADS];
    io_task_pt task_list_end[IO_TOTAL_THREADS];
    sem_t task_list_sem[IO_TOTAL_THREADS];
    // int64_t mytask_list_sem[IO_TOTAL_THREADS];

    int task_group_num[IO_TOTAL_THREADS];                    // The number of task groups in the current queue
    io_task_group_t task_group_info[MAX_TASK_GROUP_NUM];

    sem_t migrate_sem;                                       // Migration thread wake-up semaphore

	pthread_spinlock_t append_lock;
    pthread_t* tid_info_pt;
    pthread_t* migthd_info_pt;                               // Information related to migrating threads
	int shutdown;
    sem_t shutdown_sem;
};
typedef struct io_pthread_pool io_pth_pool_t;
typedef io_pth_pool_t* io_pth_pool_pt;

struct io_pth_frame
{
    int64_t tid;
    io_pth_pool_pt pool;
};
typedef struct io_pth_frame io_pth_frame_t;
typedef io_pth_frame_t* io_pth_frame_pt;

struct migrate_pth_frame
{
    int64_t tid;
    io_pth_pool_pt pool;
    struct migrate_info_t* mig_info_pt;
};
typedef struct migrate_pth_frame mig_pth_frame_t;
typedef mig_pth_frame_t* mig_pth_frame_pt;

io_pth_pool_t orch_io_scheduler;
io_pth_frame_t frames[IO_TOTAL_THREADS];
mig_pth_frame_t mig_frames[MIGRATE_THREADS];

/* Initialize IO thread pool
 */
void init_io_thread_pool();

/* Destroy IO thread pool
 */
void destroy_io_thread_pool();

/* Add tasks to the thread pool
 */
int add_task(io_task_pt ssd_task_list_head, io_task_pt nvm_task_list_head, 
			int ssd_ask_num, int nvm_task_num, int fd);

/* Waiting for a specific task to be completed
 * @task_group_id                        The ID of the waiting task
 * @return                               无
 */
void wait_task_done(int task_group_id);

#endif
