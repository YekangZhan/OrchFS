#ifndef LIB_SHM_H
#define LIB_SHM_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h> 
#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>

#include "../config/shm_type.h"

#define SHM_USED                    1
#define SHM_NOT_USED                0

struct lib_shm_info_t
{
    int used_flag[MAX_UNIT_NUM];
    int find_cur;                              
    pthread_spinlock_t used_info_lock;         
    void* libsp_shm_pt;                        
};
typedef struct lib_shm_info_t lib_shm_info_t;
typedef lib_shm_info_t* lib_shm_info_pt;

lib_shm_info_t lib_shm_info;

/* Initialize shared memory in user space
 * @return                               无
 */
void init_lib_shm(int reg_id);


void sendreq_by_shm(void* para_sp_pt, int para_len, void* data_pt, int data_len);


int sendreq_and_recv_by_shm(void* para_sp_pt, int para_len, void* data_pt, int data_len, void* recv_data_sp);


void close_lib_shm();

#endif