#ifndef SHM_TYPE_H
#define SHM_TYPE_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h> 
#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#define KET_MAGIC_OFFSET                    0x356271

#define PARA_SPACE_SIZE                     4096
#define DATA_SPACE_SIZE                     (1024*1024)

#define MAX_UNIT_NUM                        32

#define SHM_READ                            1
#define SHM_WRITE                           0

#define SHM_INFO_ON                         1
#define SHM_INFO_OFF                        0
#define SHM_RESPONSE_ON                     1
#define SHM_RESPONSE_OFF                    0

struct req_unit_t
{
    int send_req_flag;
    int recv_req_flag;

    int recv_data_len;
    int align_4k_off;

    char req_para_sp[PARA_SPACE_SIZE];
    char req_data_sp[DATA_SPACE_SIZE];
    char recv_data_sp[DATA_SPACE_SIZE];
};
typedef struct req_unit_t req_unit_t;
typedef req_unit_t* req_unit_pt;

struct shm_space_t
{
    int req_unit_num; 
    int shut_down_flag; 
    req_unit_t req_unit[MAX_UNIT_NUM];
};
typedef struct shm_space_t shm_sp_t;
typedef shm_sp_t* shm_sp_pt;

#endif