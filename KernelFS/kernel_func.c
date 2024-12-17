#include "kernel_func.h"
#include "balloc.h"
#include "device.h"
#include "log.h"
#include "type.h"
#include "thdpool.h"
#include "addr_util.h"
#include "kernel_socket.h"
#include "ker_shm.h"
#include "execio_thdpool.h"
#include "../config/protocol.h"
#include "../config/socket_config.h"
#include "../config/config.h"

#ifdef __cplusplus       
extern "C"{
#endif

// #define KERNEL_FUNC_DEBUG

int now_registered_pid;


void init_kernelFS()
{

	device_init();
	// printf("dev init!\n");
	// fflush(stdout);

	init_kernel_recv_info();
	init_kernel_send_info();
	// printf("socket init!\n");
	// fflush(stdout);

	init_kernel_thd_pool();
	// printf("thread pool init!\n");
	// fflush(stdout);


	init_kernel_shm(KET_MAGIC_OFFSET);


	init_ker_io_thread_pool();


	init_mem_bmp();
	// printf("bitmap init!\n");
	// fflush(stdout);

	init_kernel_log();
	// printf("log init!\n");
	// fflush(stdout);
}


void close_kernelFS()
{

	fprintf(stderr,"close begin!\n");
	free_kernel_recv_info();
	free_kernel_send_info();
	fprintf(stderr,"socket close!\n");

	destroy_kernel_thd_pool();
	fprintf(stderr,"thdpool close!\n");


	destroy_ker_io_thread_pool();
	close_kernel_shm();
	

	close_kernel_log();
	fprintf(stderr,"log close!\n");

	sync_all_mem_bmp();
	delete_mem_bmp();
	fprintf(stderr,"bmp close!\n");

	device_close();
	fprintf(stderr,"dev close!\n");
}

int call_alloc_func(int64_t func_type, void* para_space)
{
	int64_t* para_int64_pt = (int64_t*)para_space;
    int64_t alloc_blk_num = para_int64_pt[0];
    int64_t return_type = para_int64_pt[1];
	int64_t reg_pid = para_int64_pt[2];
	// printf("func_type: %lld %lld %lld\n",func_type,alloc_blk_num,return_type);

	int64_t mesg_headsize = sizeof(int64_t) * 2;
    void* ret_info_pt = malloc(sizeof(int64_t) * alloc_blk_num + mesg_headsize);
	void* blk_start_pt = (void*)((int64_t)ret_info_pt + mesg_headsize);
	if(func_type == ALLOC_INODE_FUNC)
	{
		alloc_inodes(alloc_blk_num, blk_start_pt, return_type);
	}
	else if(func_type == ALLOC_INXND_FUNC)
	{
		alloc_idx_nodes(alloc_blk_num, blk_start_pt, return_type);
	}
	else if(func_type == ALLOC_VIRND_FUNC)
	{
		alloc_viridx_nodes(alloc_blk_num, blk_start_pt, return_type);
	}
	else if(func_type == ALLOC_BUFMETA_FUNC)
	{
		alloc_bufmeta_nodes(alloc_blk_num, blk_start_pt, return_type);
	}
	else if(func_type == ALLOC_PAGE_FUNC)
	{
		alloc_nvm_pages(alloc_blk_num, blk_start_pt, return_type);
	}
	else if(func_type == ALLOC_BLOCK_FUNC)
	{
		alloc_ssd_blocks(alloc_blk_num, blk_start_pt, return_type);
	}
	else
		goto func_type_error;

	int64_t* ret_info_int64_pt = (void*)ret_info_pt;
	ret_info_int64_pt[0] = func_type;
	ret_info_int64_pt[1] = alloc_blk_num;


	// for(int i = 1; i < alloc_blk_num; i++)
	// {
	// 	if(ret_info_int64_pt[i+2] != ret_info_int64_pt[i+1] + 1)
	// 	{
	// 		printf("alloc blk id error!\n");
	// 		exit(1);
	// 	}
	// }


	int64_t send_len = sizeof(int64_t) * alloc_blk_num + mesg_headsize;
	if(send_len > 192*1024)
	{
		printf("send len is too long!\n");
		exit(1);
	}
	// for(int i = 0; i < alloc_blk_num; i++)
	// 	printf("%" PRId64 " ",ret_info_int64_pt[2+i]);
	// printf("\n");


    kernel_send_message(ret_info_pt, send_len, RECV_BLK_PORT+reg_pid);
    free(ret_info_pt);
	return 0;
func_type_error:
	printf("The func type is error!\n");
	return -1;
}

int call_dealloc_func(int64_t func_type, void* para_space)
{
    int64_t* para_int64_pt = (int64_t*)para_space;
    int64_t dealloc_blk_num = para_int64_pt[0];
    int64_t opblk_type = para_int64_pt[1];
	int64_t reg_pid = para_int64_pt[2];
	int64_t* blk_start_pt = (int64_t*)((int64_t)para_space + sizeof(int64_t)*3);
	// printf("blk_type: %" PRId64" %" PRId64"  %" PRId64"\n",func_type, dealloc_blk_num, reg_pid);
	// for(int i = 0; i < dealloc_blk_num; i++)
	// 	printf("%" PRId64" ",blk_start_pt[i]);
	// printf("\n");

	if(func_type == DEALLOC_INODE_FUNC)
	{
		for(int64_t i = 0; i < dealloc_blk_num; i++)
			dealloc_inode(blk_start_pt[i], opblk_type);
	}
	else if(func_type == DEALLOC_INXND_FUNC)
	{
		for(int64_t i = 0; i < dealloc_blk_num; i++)
			dealloc_idx_node(blk_start_pt[i], opblk_type);
	}
	else if(func_type == DEALLOC_VIRND_FUNC)
	{
		for(int64_t i = 0; i < dealloc_blk_num; i++)
			dealloc_viridx_node(blk_start_pt[i], opblk_type);
	}
	else if(func_type == DEALLOC_BUFMETA_FUNC)
	{
		for(int64_t i = 0; i < dealloc_blk_num; i++)
			dealloc_bufmeta_node(blk_start_pt[i], opblk_type);
	}
	else if(func_type == DEALLOC_PAGE_FUNC)
	{
		for(int64_t i = 0; i < dealloc_blk_num; i++)
			dealloc_nvm_page(blk_start_pt[i], opblk_type);
	}
	else if(func_type == DEALLOC_BLOCK_FUNC)
	{
		for(int64_t i = 0; i < dealloc_blk_num; i++)
			dealloc_ssd_block(blk_start_pt[i], opblk_type);
	}
	else
		goto func_type_error;
	return 0;
func_type_error:
	printf("The func type is error!\n");
	return -1;
}

int call_log_func(int64_t func_type, void* para_space)
{
	if(func_type == ALLOC_LOG_SEG_CODE)
	{
		int64_t* para_int64_pt = (int64_t*)para_space;
		int64_t reg_pid = para_int64_pt[0];

		int64_t ret_seg_id = alloc_log_segment_from_dev();
		int64_t ret_info_arr[2] = {0};
		ret_info_arr[0] = func_type;
		ret_info_arr[1] = ret_seg_id;

		kernel_send_message(ret_info_arr, 16, RECV_LOG_SP_PORT+reg_pid);
	}
	else
		goto func_type_error;
	return 0;
func_type_error:
	printf("The func type is error!\n");
	return -1;
message_error:
	printf("The message structure is error!\n");
	return -1;
}

int call_register_func(int64_t func_type, void* para_space)
{
	if(func_type == REGISTER_CODE)
	{
		if(para_space != NULL)
			goto message_error;
		
		int64_t ret_pid = now_registered_pid++;      

		int64_t ret_info_arr[2] = {0};
		ret_info_arr[0] = func_type;
		ret_info_arr[1] = ret_pid;

		kernel_send_message(ret_info_arr, 16, RECV_PID_PORT);
	}
	else
		goto func_type_error;
	return 0;
func_type_error:
	printf("The func type is error!\n");
	return -1;
message_error:
	printf("The message structure is error!\n");
	return -1;
}

#ifdef __cplusplus
}
#endif