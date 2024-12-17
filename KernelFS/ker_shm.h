#ifndef KERNEL_SHM_H
#define KERNEL_SHM_H

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
#include <assert.h>
#include <errno.h>

#include "../config/shm_type.h"

struct ker_shm_info_t
{
    int shm_id;                               
    int shm_key;                        
    void* kersp_shm_pt;                       
};
typedef struct ker_shm_info_t ker_shm_info_t;
typedef ker_shm_info_t* ker_shm_info_pt;

ker_shm_info_t ker_shm_info;

void init_kernel_shm(int reg_id);

void* do_recv_req(void* para_arg);

void reply_message(int unit_id, int reply_len);

void close_kernel_shm();

#endif