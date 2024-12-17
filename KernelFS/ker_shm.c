#include "ker_shm.h"
#include "../config/shm_type.h"
#include "../config/config.h"
#include "execio_thdpool.h"

void init_kernel_shm(int reg_id)
{
    int shm_key = reg_id;
    ker_shm_info.shm_key = shm_key;
    int shm_sp_size = sizeof(shm_sp_t);
    int permission_code = 0666;
    ker_shm_info.shm_id = shmget((key_t)shm_key, shm_sp_size, permission_code | IPC_CREAT);
  	if (ker_shm_info.shm_id  == -1)
  	{ 
		printf("kernel shmat(%d) failed\n",shm_key); 
        printf("%d\n", errno);
		exit(0);
	}
    ker_shm_info.kersp_shm_pt = shmat(ker_shm_info.shm_id, NULL, 0);
    memset(ker_shm_info.kersp_shm_pt, 0x00, shm_sp_size);

    shm_sp_pt shm_pt = ker_shm_info.kersp_shm_pt;
    for(int i = 0; i < MAX_UNIT_NUM; i++)
    {
        int64_t send_sp_addr = shm_pt->req_unit[i].req_data_sp;
        shm_pt->req_unit[i].align_4k_off = (((send_sp_addr >> BW_4KiB)+1) << BW_4KiB) - send_sp_addr;
    }
}

void* do_recv_req(void* para_arg)
{
    while(1)
    {
        shm_sp_pt shm_pt = ker_shm_info.kersp_shm_pt;
        int now_close_flag = __sync_fetch_and_add(&(shm_pt->shut_down_flag), 0);
        if(now_close_flag == 1)
            break;
        for(int i = 0; i < MAX_UNIT_NUM; i++)
        {
            int mesg_flag = __sync_fetch_and_add(&(shm_pt->req_unit[i].send_req_flag), 0);
            if (mesg_flag == SHM_INFO_ON)
            {
                __sync_fetch_and_and(&(shm_pt->req_unit[i].send_req_flag), SHM_INFO_OFF);
                int64_t* para_int64_pt = (int64_t*)shm_pt->req_unit[i].req_para_sp;
                int64_t op_type = para_int64_pt[0];
                int64_t offset = para_int64_pt[1], op_len = para_int64_pt[2];
                if(op_type == SHM_READ)
                {
                    ker_add_io_task(i, EXEC_READ, shm_pt->req_unit[i].recv_data_sp, op_len, offset);
                }
                else if(op_type == SHM_WRITE)
                {
                    ker_add_io_task(i, EXEC_WRITE, shm_pt->req_unit[i].req_data_sp, op_len, offset);
                }
                // printf("op_type1 %lld\n",op_type);
            }
            else
                continue;
        }
    }
}

void reply_message(int unit_id, int reply_len)
{
    shm_sp_pt shm_pt = ker_shm_info.kersp_shm_pt;

    shm_pt->req_unit[unit_id].recv_data_len = reply_len;
    __sync_fetch_and_or(&(shm_pt->req_unit[unit_id].recv_req_flag), SHM_RESPONSE_ON);
    return;
}

void close_kernel_shm()
{
    shm_sp_pt shm_pt = ker_shm_info.kersp_shm_pt;
    __sync_fetch_and_add(&(shm_pt->shut_down_flag), 1);

  	shmdt(ker_shm_info.kersp_shm_pt);

  	if (shmctl(ker_shm_info.shm_id, IPC_RMID, 0) == -1)
  	{ 
		int shm_key = ker_shm_info.shm_key;
        printf("shmctl %d failed\n",shm_key); 
		exit(0);
	}
}