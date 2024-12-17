#include "balloc.h"
#include "../config/socket_config.h"
#include "kernel_func.h"
#include "kernel_socket.h"
#include "thdpool.h"

#ifdef __cplusplus       
extern "C"{
#endif

void do_recv_in_kernel(pth_frame_t* arg)
{
	int nowtid = arg->tid;
	// printf("nowtid: %d\n",nowtid);
	kth_pool_pt orch_ktp = &orch_kth_pool;
	if(nowtid == RECV_ALLOC_OP_TID)
	{
		listen_alloc_op();
	}
	else if(nowtid == RECV_DEALLOC_OP_TID)
	{
		listen_dealloc_op();
	}
	else if(nowtid == RECV_LOG_OP_TID)
	{
		listen_log_op();
	}
	else if(nowtid == RECV_CACHE_OP_TID)
	{
		listen_cache_op();
	}
	else if(nowtid == RECV_REGISTER_TID)
	{
		listen_register_op();
	}
}

void do_operation_in_kernel(pth_frame_t* arg)
{
    int nowtid = arg->tid;
	kth_pool_pt orch_ktp = &orch_kth_pool;
	while(1)
	{
		int arr_idx = nowtid - OP_START_TID;
		
		sem_wait(&(orch_ktp->task_queue_sem[arr_idx]));
		int now_task_num = __sync_add_and_fetch(&(orch_ktp->task_num_inq[arr_idx]), 0);
		if(orch_ktp->shutdown==1 && now_task_num == 0)
			break;
		
		// Get a task from the list head
		thd_task_pt now_task_pt = orch_ktp->task_list_head[arr_idx];
		if(now_task_pt == NULL)
			goto task_queue_error;
		
		// Pop the task from the list head
		orch_ktp->task_list_head[arr_idx] = orch_ktp->task_list_head[arr_idx]->next;

		// exec the task
		if(nowtid == ALLOC_TID)
		{
			call_alloc_func(now_task_pt->func_type, now_task_pt->para_space);
		}
		else if(nowtid == DEALLOC_TID)
		{
			call_dealloc_func(now_task_pt->func_type, now_task_pt->para_space);
		}
		else if(nowtid == LOG_TID)
		{
			call_log_func(now_task_pt->func_type, now_task_pt->para_space);
		}
		else if(nowtid == RIGESTER_TID)
		{
			call_register_func(now_task_pt->func_type, now_task_pt->para_space);
		}
		else if(nowtid == CACHE_OP_TID)
		{
			/*TODO*/
		}

		__sync_add_and_fetch(&(orch_ktp->task_num_inq[arr_idx]), -1);
		free(now_task_pt->para_space);
		free(now_task_pt);
	}
	int end_thd_num = __sync_add_and_fetch(&(orch_ktp->break_thd_num), 1);
	if(end_thd_num == TOTAL_THREADS)
		sem_post(&(orch_ktp->destroy_flag));
	return;
task_queue_error:
	printf("task queue error!\n");
	exit(1);
}

int init_kernel_thd_pool()
{
	kth_pool_pt orch_ktp = &orch_kth_pool;
    orch_ktp->shutdown = 0;
    
    // create receive thread
    orch_ktp->recv_thd_info = (pthread_t *)malloc(sizeof(pthread_t) * RECV_THREADS);
	if(orch_ktp->recv_thd_info == NULL)
        goto alloc_error;
    int recv_base_tid = 0;
    for(int i = 0; i < RECV_THREADS; i++)
	{
		int real_tid = recv_base_tid + i;
        frames[real_tid].tid = real_tid;
		int ret = pthread_create(orch_ktp->recv_thd_info + i, NULL, do_recv_in_kernel, &frames[real_tid]);
		if(ret != 0)
            goto create_thd_error;
	}
    
    // create operation thread
    orch_ktp->op_thd_info = (pthread_t *)malloc(sizeof(pthread_t) * OP_THREADS);
	if(orch_ktp->op_thd_info == NULL)
        goto alloc_error;
    for(int i = 0; i < OP_THREADS; i++)
    {
        sem_init(orch_ktp->task_queue_sem + i, 0, 0);
        pthread_spin_init(orch_ktp->task_app_lock + i, NULL);
        
        orch_ktp->task_list_head[i] = malloc(sizeof(thd_task_t));
		orch_ktp->task_list_head[i]->next = NULL;
		orch_ktp->task_list_end[i] = orch_ktp->task_list_head[i];
    }
	int op_base_tid = OP_START_TID;
    for(int i = 0; i < OP_THREADS; i++)
	{
		int real_tid = op_base_tid + i;
        frames[real_tid].tid = real_tid;
		int ret = pthread_create(orch_ktp->op_thd_info + i, NULL, do_operation_in_kernel, &frames[real_tid]);
		if(ret != 0)
            goto create_thd_error;
	}
    
	return 0;
alloc_error:
    perror("malloc memery error");
	return -1;
create_thd_error:
    perror("create thread error");
	return -1;
}

void destroy_kernel_thd_pool()
{
    kth_pool_pt orch_ktp = &orch_kth_pool;
	orch_ktp->shutdown = 1;

	// Notify other threads to end execution
	int recv_base_tid = 0;
    for(int i = 0; i < RECV_THREADS; i++)
	{
		int real_tid = recv_base_tid + i;
        pthread_join(orch_ktp->recv_thd_info[i],NULL);
	}

	for(int i = 0; i < OP_THREADS; i++)
	{
		sem_post(&(orch_ktp->task_queue_sem[i]));
	}

    // close threads
    int op_base_tid = 4;
    for(int i = 0; i < OP_THREADS; i++)
	{
        pthread_join(orch_ktp->op_thd_info[i],NULL);
	}

    // free
	for(int i = 0; i < OP_THREADS; i++)
	{
		if(orch_ktp->task_list_head[i] != NULL)
			free(orch_ktp->task_list_head[i]);
		orch_ktp->task_list_head[i] = NULL;
		orch_ktp->task_list_end[i] = NULL;
	}
    free(orch_ktp->recv_thd_info);
    free(orch_ktp->op_thd_info);
}

void add_task_to_queue(int queue_id, thd_task_pt add_info_pt)
{
	kth_pool_pt orch_ktp = &orch_kth_pool;
	int qid = queue_id;
	pthread_spin_lock(orch_ktp->task_app_lock + qid);
	memcpy(orch_ktp->task_list_end[qid], add_info_pt, sizeof(thd_task_t));
	orch_ktp->task_list_end[qid]->next = malloc(sizeof(thd_task_t)); 
	orch_ktp->task_list_end[qid] = orch_ktp->task_list_end[qid]->next;
	orch_ktp->task_list_end[qid]->next = NULL;

	__sync_add_and_fetch(orch_ktp->task_num_inq + qid, 1);
	sem_post(&(orch_ktp->task_queue_sem[qid]));
	pthread_spin_unlock(orch_ktp->task_app_lock + qid);
}

#ifdef __cplusplus
}
#endif