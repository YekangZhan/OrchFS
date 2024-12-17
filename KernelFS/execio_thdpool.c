#include "execio_thdpool.h"
#include "ker_shm.h"
#include "../config/shm_type.h"
#include "device.h"

static long exec_calc_diff(struct timespec start, struct timespec end){
	return (end.tv_sec * (long)(1000000000) + end.tv_nsec) -
	(start.tv_sec * (long)(1000000000) + start.tv_nsec);
}

void* ker_exec_io(void* para_arg)
{
	ker_io_pth_frame_pt arg = (ker_io_pth_frame_pt)para_arg;
	ker_io_pth_pool_pt pool = arg->pool;
	int nowtid = arg->tid;
	while(1)
	{
		struct timespec lock_begin_time;
		struct timespec lock_end_time;
    	clock_gettime(CLOCK_MONOTONIC, &lock_begin_time);
		while(1)
		{
			int sem_ret = sem_trywait(pool->task_list_sem + nowtid);
			if(sem_ret == 0)
			{
				break;
			}
			else
			{
				clock_gettime(CLOCK_MONOTONIC, &lock_end_time);
				long time2 = exec_calc_diff(lock_begin_time, lock_end_time);
				if(time2 > 5*1000*1000)
				{
					// printf("fffff %d\n", nowtid);
					sem_wait(pool->task_list_sem + nowtid);
					break;
				}
			}
		}

		if(pool->shutdown == 1)
		{
			sem_post(&(pool->shutdown_sem));
			break;
		}


        ker_io_task_pt now_task_pt = pool->task_list_head[nowtid];
		pool->task_list_head[nowtid] = pool->task_list_head[nowtid]->next;


		if(now_task_pt->io_type == EXEC_WRITE)
		{
			if(now_task_pt->io_len < 0 || now_task_pt->io_data_buf == NULL)
				goto cannot_write;
			// printf("write: %lld %lld %lld\n",nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
			shm_sp_pt shm_pt = ker_shm_info.kersp_shm_pt;
			int64_t buf_off = shm_pt->req_unit[nowtid].align_4k_off;
			shm_write_data_to_devs(now_task_pt->io_data_buf, buf_off, now_task_pt->io_len, now_task_pt->io_start_addr);
			reply_message(nowtid, 0);
        }
		if(now_task_pt->io_type == EXEC_READ)
		{
			if(now_task_pt->io_len < 0 || now_task_pt->io_data_buf == NULL)
				goto cannot_read;
			// printf("read: %lld %lld %lld\n",nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
			read_data_from_devs(now_task_pt->io_data_buf, now_task_pt->io_len, now_task_pt->io_start_addr);
			reply_message(nowtid, now_task_pt->io_len);
        }

		free(now_task_pt);
		now_task_pt = NULL;
	}
	return NULL;
cannot_read:
	printf("parameter error -- cannot_read\n");
	exit(1);
cannot_write:
	printf("parameter error -- cannot_write\n");
	exit(1);
}

void init_ker_io_thread_pool()
{
    ker_io_pth_pool_pt pool = &orch_io_exec;
	pool->shutdown = 0;
    
	for(int i = 0; i < EXEC_THREADS; i++)
	{
		pool->task_list_head[i] = malloc(sizeof(ker_io_task_t));
        if(pool->task_list_head[i] == NULL)
            goto alloc_error;
		pool->task_list_head[i]->next = NULL;
		pool->task_list_end[i] = pool->task_list_head[i];
		sem_init(pool->task_list_sem + i, 0, 0);
	}
    
    pool->tid_info_pt = (pthread_t *)malloc(sizeof(pthread_t) * TOTAL_EXEC_THREADS);
	if(pool->tid_info_pt == NULL)
        goto alloc_error;
    for(int i = 0; i < TOTAL_EXEC_THREADS; i++)
	{
		kio_frames[i].pool = pool;
		kio_frames[i].tid = i;
		if(i == TOTAL_EXEC_THREADS - 1)
		{
			int ret = pthread_create(&(pool->tid_info_pt)[i], NULL, do_recv_req, &kio_frames[i]);
			if(ret != 0)
				goto thd_create_error;
		}
		else
		{
			int ret = pthread_create(&(pool->tid_info_pt)[i], NULL, ker_exec_io, &kio_frames[i]);
			if(ret != 0)
				goto thd_create_error;
		}
	}
    return;
alloc_error:
    perror("malloc memery error -- init_ker_io_thread_pool\n");
	exit(1);
thd_create_error:
	perror("create thread error -- init_ker_io_thread_pool\n");
	return;
}


void destroy_ker_io_thread_pool()
{
	shm_sp_pt shm_pt = ker_shm_info.kersp_shm_pt;
	__sync_fetch_and_or(&(shm_pt->shut_down_flag), 1);
	
	ker_io_pth_pool_pt pool = &orch_io_exec;

	pool->shutdown = 1;


	for(int i = 0; i < EXEC_THREADS; i++)
	{
		sem_post(pool->task_list_sem + i);
	}


	for(int i = 0; i < EXEC_THREADS; i++)
	{
		sem_wait(&(pool->shutdown_sem));
	}
	

    for(int i = 0; i < TOTAL_EXEC_THREADS; i++)
	{
		pthread_join(pool->tid_info_pt[i], NULL);
	}
	

	for(int i = 0; i < TOTAL_EXEC_THREADS; i++)
	{
		free(pool->task_list_head[i]);
		pool->task_list_head[i] = NULL;
		pool->task_list_end[i] = NULL;
	}
    free(pool->tid_info_pt);
}

void ker_add_io_task(int queue_id, int task_type, void* data_sp, int64_t len, int64_t offset)
{
	ker_io_pth_pool_pt pool = &orch_io_exec;
	if(queue_id < 0 || queue_id >= 	MAX_UNIT_NUM)
		goto queue_id_error;
		

	pool->task_list_end[queue_id]->io_data_buf = data_sp;
	pool->task_list_end[queue_id]->io_len = len;
	pool->task_list_end[queue_id]->io_start_addr = offset;
	pool->task_list_end[queue_id]->io_type = task_type;

	pool->task_list_end[queue_id]->next = malloc(sizeof(ker_io_task_t)); 
	pool->task_list_end[queue_id] = pool->task_list_end[queue_id]->next;
	pool->task_list_end[queue_id]->next = NULL;


	sem_post(pool->task_list_sem + queue_id);
	return;
queue_id_error:
	printf("The queue id is error!\n");
	exit(0);
}
