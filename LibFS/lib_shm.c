#include "lib_shm.h"
#include "runtime.h"
#include "../config/shm_type.h"
#include "../config/config.h"

void init_lib_shm(int reg_id)
{
    for(int i = 0; i < MAX_UNIT_NUM; i++)
        lib_shm_info.used_flag[i] = SHM_NOT_USED;
    lib_shm_info.find_cur = 0;
    pthread_spin_init(&(lib_shm_info.used_info_lock), PTHREAD_PROCESS_SHARED);
    
    // Create shared memory
    int shm_key = KET_MAGIC_OFFSET;
    int shm_sp_size = sizeof(shm_sp_pt);
    int permission_code = 0666;
    int shmid = shmget((key_t)shm_key, shm_sp_size, permission_code);
  	if (shmid == -1)
  	{ 
		printf("shmat(%d) failed\n",shm_key); 
		exit(0);
	}

    // Link to shared memory
  	lib_shm_info.libsp_shm_pt = shmat(shmid, NULL, 0);
}

int find_empty_shm_unit()
{
    pthread_spin_lock(&(lib_shm_info.used_info_lock));
    for(int i = 0; i < MAX_UNIT_NUM; i++)
    {
        // int now_cur = (lib_shm_info.find_cur + i)%MAX_UNIT_NUM;
        int now_cur = i;
        if(__sync_fetch_and_sub(&(lib_shm_info.used_flag[now_cur]), 0) == SHM_NOT_USED)
        {
            lib_shm_info.find_cur = (now_cur+1)%MAX_UNIT_NUM;
            __sync_fetch_and_or(&(lib_shm_info.used_flag[now_cur]), SHM_USED);
            pthread_spin_unlock(&(lib_shm_info.used_info_lock));
            return now_cur;
        }
    }
    pthread_spin_unlock(&(lib_shm_info.used_info_lock));
use_info_error:
    printf("shm use info error!\n");
    exit(0);
}

void sendreq_by_shm(void* para_sp_pt, int para_len, void* data_pt, int data_len)
{
    int unit_id = find_empty_shm_unit();
    shm_sp_pt shm_pt = lib_shm_info.libsp_shm_pt;
    // printf("unit_id %d\n",unit_id);

    int64_t* para_int64_pt = (int64_t*)para_sp_pt;
    if(data_pt != NULL)
    {
        int64_t sp_offset = shm_pt->req_unit[unit_id].align_4k_off;
        memcpy(shm_pt->req_unit[unit_id].req_data_sp + sp_offset, data_pt, data_len);
        // memcpy(aligned_ds_addr + pos_4kib, data_pt, data_len);
    }
    if(para_sp_pt == NULL)
        goto para_sp_error;
    memcpy(shm_pt->req_unit[unit_id].req_para_sp, para_sp_pt, para_len);

    __sync_fetch_and_or(&(shm_pt->req_unit[unit_id].send_req_flag), SHM_INFO_ON);

    while(__sync_fetch_and_add(&(shm_pt->req_unit[unit_id].recv_req_flag), 0) == SHM_RESPONSE_OFF)
    {
        continue;
    }
    __sync_fetch_and_and(&(shm_pt->req_unit[unit_id].recv_req_flag), SHM_RESPONSE_OFF);

    __sync_fetch_and_and(&(lib_shm_info.used_flag[unit_id]), SHM_NOT_USED);

    return;
para_sp_error:
    printf("para space point is NULL!\n");
    exit(0);
}

int sendreq_and_recv_by_shm(void* para_sp_pt, int para_len, void* data_pt, int data_len, void* recv_data_sp)
{
    int unit_id = find_empty_shm_unit();
    shm_sp_pt shm_pt = lib_shm_info.libsp_shm_pt;

    if(para_sp_pt == NULL)
        goto para_sp_error;
    memcpy(shm_pt->req_unit[unit_id].req_para_sp, para_sp_pt, para_len);
    int64_t* para_int64_pt = (int64_t*)para_sp_pt;
    if(data_pt != NULL)
    {
        int64_t sp_offset = shm_pt->req_unit[unit_id].align_4k_off;
        memcpy(shm_pt->req_unit[unit_id].req_data_sp + sp_offset, data_pt, data_len);
    }

    __sync_fetch_and_or(&(shm_pt->req_unit[unit_id].send_req_flag), SHM_INFO_ON);

    int recv_len = 0;
    while(__sync_fetch_and_add(&(shm_pt->req_unit[unit_id].recv_req_flag), 0) == SHM_RESPONSE_OFF)
    {
        continue;
    }
    recv_len = shm_pt->req_unit[unit_id].recv_data_len;
    memcpy(recv_data_sp, shm_pt->req_unit[unit_id].recv_data_sp, recv_len);
    __sync_fetch_and_and(&(shm_pt->req_unit[unit_id].recv_req_flag), SHM_RESPONSE_OFF);

    __sync_fetch_and_and(&(lib_shm_info.used_flag[unit_id]), SHM_NOT_USED);

    return recv_len;
para_sp_error:
    printf("para space point is NULL!\n");
    exit(0);
}

void close_lib_shm()
{
    pthread_spin_lock(&(lib_shm_info.used_info_lock));
    for(int i = 0; i < MAX_UNIT_NUM; i++)
    {
        while(__sync_fetch_and_sub(&(lib_shm_info.used_flag[i]), 0) == SHM_USED)
        {
            continue;
        }
    }
    shmdt(lib_shm_info.libsp_shm_pt);
    pthread_spin_unlock(&(lib_shm_info.used_info_lock));
    pthread_spin_destroy(&(lib_shm_info.used_info_lock));
}