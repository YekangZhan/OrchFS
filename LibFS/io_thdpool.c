#include "io_thdpool.h"
#include "runtime.h"
#include "lib_shm.h"
#include "migrate.h"

#include "../config/config.h"
#include "../KernelFS/device.h"

// void print_data(void* buf, int len)
// {
// 	for(int i = 0; i < len; i++)
// 	{
// 		char* char_buf = (char*)buf;
// 		printf("%d ",(int)char_buf[i]);
// 	}
// 	printf("\n");
// }

static long calc_diff(struct timespec start, struct timespec end){
	return (end.tv_sec * (long)(1000000000) + end.tv_nsec) -
	(start.tv_sec * (long)(1000000000) + start.tv_nsec);
}

void* exec_io(void* para_arg)
{
	io_pth_frame_pt arg = (io_pth_frame_pt)para_arg;
	io_pth_pool_pt pool = arg->pool;
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
				long time2 = calc_diff(lock_begin_time, lock_end_time);
				if(time2 > 5*1000*1000)
				{
					sem_wait(pool->task_list_sem + nowtid);
					break;
				}
			}
		}

		if(pool->shutdown == 1 && __sync_fetch_and_sub(&(pool->task_group_num[nowtid]), 0) == 0)
		{
			// printf("thd: %d end\n",nowtid);
			sem_post(&(pool->shutdown_sem));
			break;
		}

        // Obtain task group number
        int this_task_id = (pool->task_list_head[nowtid])->task_group_id;
        // Get the number of tasks for the current thread in the task group
        int this_task_ops = (pool->task_group_info[this_task_id]).task_num[nowtid];
        
        for(int i = 1; i <= this_task_ops; i++)
		{
			// Retrieve the current task
            io_task_pt now_task_pt = pool->task_list_head[nowtid];
			pool->task_list_head[nowtid] = pool->task_list_head[nowtid]->next;

			// do task
			if(now_task_pt->io_type == WRITE_OP)
			{
				io_task_pt nt_pt = now_task_pt;
				if(nt_pt->io_len < 0 || nt_pt->io_data_buf == NULL)
					goto cannot_write;
				// printf("write: %lld %lld %lld\n",nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
				// write_data_to_devs(nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
				int64_t* para_sp_pt = malloc(3 * sizeof(int64_t));

				para_sp_pt[0] = SHM_WRITE; 
				para_sp_pt[1] = nt_pt->io_start_addr;
				para_sp_pt[2] = nt_pt->io_len;
				sendreq_by_shm(para_sp_pt, 3*sizeof(int64_t), nt_pt->io_data_buf, nt_pt->io_len);
				free(para_sp_pt);
            }
			if(now_task_pt->io_type == READ_OP)
			{
				io_task_pt nt_pt = now_task_pt;
				if(nt_pt->io_len < 0 || nt_pt->io_data_buf == NULL)
					goto cannot_read;
				// printf("read: %lld %lld %lld\n",nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
				// read_data_from_devs(nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
				int64_t* para_sp_pt = malloc(3 * sizeof(int64_t));
				para_sp_pt[0] = SHM_READ; 
				para_sp_pt[1] = nt_pt->io_start_addr;
				para_sp_pt[2] = nt_pt->io_len;
				sendreq_and_recv_by_shm(para_sp_pt, 3*sizeof(int64_t), NULL, 0, nt_pt->io_data_buf);
				free(para_sp_pt);
            }

			free(now_task_pt);
			now_task_pt = NULL;
		}
		__sync_fetch_and_sub(&(pool->task_group_num[nowtid]), 1);
		pthread_spin_unlock((pool->task_group_info[this_task_id]).task_done_lock + nowtid);
        // sem_post((pool->task_group_info[this_task_id]).task_done_sem + nowtid);
	}
	return NULL;
cannot_read:
	printf("parameter error -- cannot_read\n");
	exit(1);
cannot_write:
	printf("parameter error -- cannot_write\n");
	exit(1);
}


int query_task_id(io_pth_pool_pt pool)
{
	int cur = rand() % MAX_TASK_GROUP_NUM;
	while(1)
	{
		int try_sem_info = __sync_fetch_and_or(&((pool->task_group_info[cur]).used_flag), 1);
		if(try_sem_info == 0)
		{
			for(int i = 0; i < IO_TOTAL_THREADS; i++)
				(pool->task_group_info[cur]).task_num[i] = 0;
			return cur;
		}
		cur = (cur+1) % MAX_TASK_GROUP_NUM;
	}
}

int get_send_thread_id(io_pth_pool_pt pool, io_task_pt task_list_head, int alloced_time[])
{
	int64_t min_queue_len = 1e9, min_queue_pos = 0;
	for(int i = 0; i < MAX_SSD_THREADS; i++)
	{
		int64_t queue_len = __sync_fetch_and_sub(&(pool->task_group_num[pool->alloc_thd_cur]), 0);
		queue_len += alloced_time[pool->alloc_thd_cur];
		if(queue_len < min_queue_len)
		{
			min_queue_len = queue_len; min_queue_pos = pool->alloc_thd_cur;
		}
		if(queue_len == 0)
		{
			min_queue_pos = pool->alloc_thd_cur;
			pool->alloc_thd_cur = (pool->alloc_thd_cur+1) % MAX_SSD_THREADS;
			break;
		}
		pool->alloc_thd_cur = (pool->alloc_thd_cur+1) % MAX_SSD_THREADS;
	}
	return min_queue_pos;
	// pool->alloc_thd_cur = (pool->alloc_thd_cur+1) % MAX_SSD_THREADS;
	// return pool->alloc_thd_cur;
operation_type_error:
	printf("operation block type error!\n");
	exit(0);
}

void init_io_thread_pool()
{
    io_pth_pool_pt pool = &orch_io_scheduler;
	pool->shutdown = 0;
	sem_init(&(pool->shutdown_sem), 0, 0);
	sem_init(&(pool->migrate_sem), 0, 0);
    
    // Initialize NVM available threads
	sem_init(&(pool->nvm_rw_sem), 0, MAX_NVM_THREADS);

    // Initialize task queue
	pthread_spin_init(&(pool->append_lock), PTHREAD_PROCESS_SHARED);
	for(int i = 0; i < IO_TOTAL_THREADS; i++)
	{
		pool->task_list_head[i] = malloc(sizeof(io_task_t));
        if(pool->task_list_head[i] == NULL)
            goto alloc_error;
		pool->task_list_head[i]->next = NULL;
		pool->task_list_end[i] = pool->task_list_head[i];
		sem_init(pool->task_list_sem + i, 0, 0);
		// pool->mytask_list_sem[i] = 0;
	}
    for(int i = 0; i < IO_TOTAL_THREADS; i++)
        pool->task_group_num[i] = 0;
	
    // Initialize task group information
    for(int i = 0; i < MAX_TASK_GROUP_NUM; i++)
    {
        (pool->task_group_info[i]).used_flag = GROUP_NOT_USE;
		for(int j = 0; j < IO_TOTAL_THREADS; j++)
        	pthread_spin_init((pool->task_group_info[i]).task_done_lock + j, PTHREAD_PROCESS_SHARED);
    }
    pool->tid_info_pt = (pthread_t *)malloc(sizeof(pthread_t) * IO_TOTAL_THREADS);
	pool->migthd_info_pt = (pthread_t *)malloc(sizeof(pthread_t) * MIGRATE_THREADS);
	if(pool->tid_info_pt == NULL || pool->migthd_info_pt == NULL)
        goto alloc_error;
    for(int i = 0; i < IO_TOTAL_THREADS; i++)
	{
		frames[i].pool = pool;
		frames[i].tid = i;
		int ret = pthread_create(&(pool->tid_info_pt)[i], NULL, exec_io, &frames[i]);
		if(ret != 0)
			goto thd_create_error;
	}
	for(int i = 0; i < MIGRATE_THREADS; i++)
	{
		mig_frames[i].pool = pool;
		mig_frames[i].mig_info_pt = orch_rt.mig_rtinfo_pt;
		// fprintf(stderr,"check info0: %lld\n",mig_frames[i].mig_info_pt->migrate_num);
		mig_frames[i].tid = i + IO_TOTAL_THREADS;
		int ret = pthread_create(&(pool->migthd_info_pt)[i], NULL, wait_and_exec_migrate, &mig_frames[i]);
		if(ret != 0)
			goto thd_create_error;
	}
    return;
alloc_error:
    perror("malloc memery error");
	exit(1);
thd_create_error:
	perror("create thread error");
	return;
}


void destroy_io_thread_pool()
{
	io_pth_pool_pt pool = &orch_io_scheduler;

	pool->shutdown = 1;
	pool->alloc_thd_cur = 0;

	// Activate all threads and wait for the thread task to end
	for(int i = 0; i < IO_TOTAL_THREADS; i++)
	{
		sem_post(pool->task_list_sem + i);
	}
	for(int i = 0; i < MIGRATE_THREADS; i++)
	{
		sem_post(&(pool->migrate_sem));
	}
	fprintf(stderr,"close message sent!\n");

	// Waiting for the thread to send a close confirmation request
	for(int i = 0; i < IO_TOTAL_THREADS; i++)
	{
		sem_wait(&(pool->shutdown_sem));
	}
	for(int i = 0; i < MIGRATE_THREADS; i++)
	{
		sem_wait(&(pool->shutdown_sem));
	}
	fprintf(stderr,"reply message received!\n");
	
	// join threads
    for(int i = 0; i < IO_TOTAL_THREADS; i++)
	{
		pthread_join(pool->tid_info_pt[i], NULL);
	}
	fprintf(stderr,"join threads!\n");
	
	// free
	for(int i = 0; i < IO_TOTAL_THREADS; i++)
	{
		free(pool->task_list_head[i]);
		pool->task_list_head[i] = NULL;
		pool->task_list_end[i] = NULL;
	}
    free(pool->tid_info_pt);
	fprintf(stderr,"clear space!\n");
}


int add_task(io_task_pt ssd_task_list_head, io_task_pt nvm_task_list_head, 
			int ssd_task_num, int nvm_task_num, int fd)
{
	io_pth_pool_pt pool = &orch_io_scheduler;
	
	pthread_spin_lock(&(pool->append_lock));
    
	int task_group_id = query_task_id(pool);     
	
	int task_thd_flag[IO_TOTAL_THREADS] = {0};
	int need_op_thdq[IO_TOTAL_THREADS], thdq_cur = -1;
	int alloc_time[IO_TOTAL_THREADS] = {0};

	io_task_pt now_task_pt = ssd_task_list_head;
	// printf("tsk_num: %d\n",task_num);
	pool->alloc_thd_cur = 0;
	for(int i = 0; i < ssd_task_num; i++)
	{
		// Get the queue number to add to
		int send_thd_id = get_send_thread_id(pool, now_task_pt, alloc_time);
		alloc_time[send_thd_id]++;
		
		// Add task to queue
		memcpy(pool->task_list_end[send_thd_id], now_task_pt, sizeof(io_task_t));
		pool->task_list_end[send_thd_id]->task_group_id = task_group_id;
		pool->task_list_end[send_thd_id]->next = malloc(sizeof(io_task_t)); 
		pool->task_list_end[send_thd_id] = pool->task_list_end[send_thd_id]->next;
		pool->task_list_end[send_thd_id]->next = NULL;

		now_task_pt = now_task_pt->next;

		if(task_thd_flag[send_thd_id] == 0)
		{
			task_thd_flag[send_thd_id] = 1;
			need_op_thdq[++thdq_cur] = send_thd_id;
		}

		(pool->task_group_info[task_group_id]).task_num[send_thd_id] += 1;
	}

	for(int i = 0; i <= thdq_cur; i++)
	{
		int thd_id_store = need_op_thdq[i];
		__sync_fetch_and_add(&(pool->task_group_num[thd_id_store]), 1);
		pthread_spin_lock((pool->task_group_info[task_group_id]).task_done_lock + thd_id_store);

		sem_post(pool->task_list_sem + thd_id_store);

		// while(sem_post(pool->task_list_sem + thd_id_store) == EOVERFLOW)
        // {
		// 	continue;
		// }
	}
	pthread_spin_unlock(&(pool->append_lock));
	return task_group_id;
}


void wait_task_done(int task_group_id)
{
	io_pth_pool_pt pool = &orch_io_scheduler;
	for(int i = 0; i < IO_TOTAL_THREADS; i++)
	{
		if((pool->task_group_info[task_group_id]).task_num[i] > 0)
		{
			// sem_wait((pool->task_group_info[task_group_id]).task_done_sem + i);
			pthread_spin_lock((pool->task_group_info[task_group_id]).task_done_lock + i);
			pthread_spin_unlock((pool->task_group_info[task_group_id]).task_done_lock + i);
		}
	}
	__sync_fetch_and_and(&((pool->task_group_info[task_group_id]).used_flag), GROUP_NOT_USE);
	// (pool->task_group_info[task_group_id]).used_flag = GROUP_NOT_USE;
}