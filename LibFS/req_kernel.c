#include "../config/socket_config.h"
#include "../config/protocol.h"
#include "lib_socket.h"
#include "req_kernel.h"
#include "runtime.h"

#ifdef __cplusplus       
extern "C"{
#endif

void reqlock_unlock()
{
    pthread_spin_unlock(&(orch_rt.req_kernel_lock));
    // printf("unlock\n");
}
void reqlock_lock()
{
    pthread_spin_lock(&(orch_rt.req_kernel_lock));
    // printf("lock\n");
}

void orch_time_stamp(struct timespec * time)
{
    clock_gettime(CLOCK_REALTIME, time);
}


void debug(void* out)
{
    int64_t* out_pt = (int64_t*)out;
    for(int i = 0; i <= 60; i++)
        printf("%" PRId64" ",out_pt[i]);
    printf("\n");
}

/*-------------------------alloc----------------------------*/

void request_inode_id_arr(void* ret_info_buf, int64_t req_blk_num, int64_t ret_type)
{
    int64_t send_buf[5];
    send_buf[0] = ALLOC_INODE_FUNC;
    send_buf[1] = req_blk_num; send_buf[2] = ret_type;
    send_buf[3] = lib_register_pid;
    void* send_data = (void*)send_buf;
    
    reqlock_lock();
    lib_send_message(send_data, sizeof(int64_t)*4, ALLOC_OP_PORT);
    int recv_len = lib_recv_message(RECV_BLK_PORT + lib_register_pid, ret_info_buf, sizeof(int64_t)*(req_blk_num+2));
    // fprintf(stderr, "recv len: %d 0 \n",recv_len);
    reqlock_unlock();
}

void request_idxnd_id_arr(void* ret_info_buf, int64_t req_blk_num, int64_t ret_type)
{
    int64_t send_buf[5];
    send_buf[0] = ALLOC_INXND_FUNC;
    send_buf[1] = req_blk_num; send_buf[2] = ret_type;
    send_buf[3] = lib_register_pid;
    void* send_data = (void*)send_buf;

    reqlock_lock();
    lib_send_message(send_data, sizeof(int64_t)*4, ALLOC_OP_PORT);
    int recv_len = lib_recv_message(RECV_BLK_PORT + lib_register_pid, ret_info_buf, sizeof(int64_t)*(req_blk_num+2));
    // fprintf(stderr, "recv len: %d 1 \n",recv_len);
    reqlock_unlock();
}

void request_virnd_id_arr(void* ret_info_buf, int64_t req_blk_num, int64_t ret_type)
{
    int64_t send_buf[5];
    send_buf[0] = ALLOC_VIRND_FUNC;
    send_buf[1] = req_blk_num; send_buf[2] = ret_type;
    send_buf[3] = lib_register_pid;
    void* send_data = (void*)send_buf;
    
    reqlock_lock();
    lib_send_message(send_data, sizeof(int64_t)*4, ALLOC_OP_PORT);
    int recv_len = lib_recv_message(RECV_BLK_PORT + lib_register_pid, ret_info_buf, sizeof(int64_t)*(req_blk_num+2));
    // fprintf(stderr, "recv len: %d 2 \n",recv_len);
    reqlock_unlock();
}

void request_bufmeta_id_arr(void* ret_info_buf, int64_t req_blk_num, int64_t ret_type)
{
    int64_t send_buf[5];
    send_buf[0] = ALLOC_BUFMETA_FUNC;
    send_buf[1] = req_blk_num; send_buf[2] = ret_type;
    send_buf[3] = lib_register_pid;
    void* send_data = (void*)send_buf;
    
    reqlock_lock();
    // fprintf(stderr, "send len: 3 \n");
    lib_send_message(send_data, sizeof(int64_t)*4, ALLOC_OP_PORT);
    int recv_len = lib_recv_message(RECV_BLK_PORT + lib_register_pid, ret_info_buf, sizeof(int64_t)*(req_blk_num+2));
    // fprintf(stderr, "recv len: %d 3 \n",recv_len);
    reqlock_unlock();
}

void request_page_id_arr(void* ret_info_buf, int64_t req_blk_num, int64_t ret_type)
{
    int64_t send_buf[5];
    send_buf[0] = ALLOC_PAGE_FUNC;
    send_buf[1] = req_blk_num; send_buf[2] = ret_type;
    send_buf[3] = lib_register_pid;
    void* send_data = (void*)send_buf;
    
    reqlock_lock();
    // fprintf(stderr, "send len: 4\n");
    lib_send_message(send_data, sizeof(int64_t)*4, ALLOC_OP_PORT);
    int recv_len = lib_recv_message(RECV_BLK_PORT + lib_register_pid, ret_info_buf, sizeof(int64_t)*(req_blk_num+2));
    // fprintf(stderr, "recv len: %d 4 \n",recv_len);
    reqlock_unlock();
}

void request_block_id_arr(void* ret_info_buf, int64_t req_blk_num, int64_t ret_type)
{
    int64_t send_buf[5];
    send_buf[0] = ALLOC_BLOCK_FUNC;
    send_buf[1] = req_blk_num; 
    send_buf[2] = ret_type;
    send_buf[3] = lib_register_pid;
    void* send_data = (void*)send_buf;
    
    reqlock_lock();
    lib_send_message(send_data, sizeof(int64_t)*4, ALLOC_OP_PORT);
    int recv_len = lib_recv_message(RECV_BLK_PORT + lib_register_pid, ret_info_buf, sizeof(int64_t)*(req_blk_num+2));
    // fprintf(stderr, "recv len: %d 5 \n",recv_len);
    reqlock_unlock();
}


/*-------------------------dealloc--------------------------*/

void send_dealloc_inode_req(void* send_buf, int dealloc_blk_num, int64_t ret_type)
{
    int64_t message_len = (dealloc_blk_num+4) * sizeof(int64_t);
    void* send_data = malloc(message_len);

    int64_t* sd_int64_pt = (int64_t*)send_data;
    sd_int64_pt[0] = DEALLOC_INODE_FUNC;
    sd_int64_pt[1] = dealloc_blk_num;
    sd_int64_pt[2] = ret_type;
    sd_int64_pt[3] = lib_register_pid;

    void* dealloc_blkid_pt = (void*)(sd_int64_pt + 4);
    memcpy(dealloc_blkid_pt, send_buf, dealloc_blk_num * sizeof(int64_t));
    lib_send_message(send_data, message_len, DEALLOC_OP_PORT);
    free(send_data);
}

void send_dealloc_idxnd_req(void* send_buf, int dealloc_blk_num, int64_t ret_type)
{
    int64_t message_len = (dealloc_blk_num+4) * sizeof(int64_t);
    void* send_data = malloc(message_len);

    int64_t* sd_int64_pt = (int64_t*)send_data;
    sd_int64_pt[0] = DEALLOC_INXND_FUNC;
    sd_int64_pt[1] = dealloc_blk_num;
    sd_int64_pt[2] = ret_type;
    sd_int64_pt[3] = lib_register_pid;

    void* dealloc_blkid_pt = (void*)(sd_int64_pt + 4);
    memcpy(dealloc_blkid_pt, send_buf, dealloc_blk_num * sizeof(int64_t));
    lib_send_message(send_data, message_len, DEALLOC_OP_PORT);
    free(send_data);
}

void send_dealloc_virnd_req(void* send_buf, int dealloc_blk_num, int64_t ret_type)
{
    int64_t message_len = (dealloc_blk_num+4) * sizeof(int64_t);
    void* send_data = malloc(message_len);

    int64_t* sd_int64_pt = (int64_t*)send_data;
    sd_int64_pt[0] = DEALLOC_VIRND_FUNC;
    sd_int64_pt[1] = dealloc_blk_num;
    sd_int64_pt[2] = ret_type;
    sd_int64_pt[3] = lib_register_pid;

    void* dealloc_blkid_pt = (void*)(sd_int64_pt + 4);
    memcpy(dealloc_blkid_pt, send_buf, dealloc_blk_num * sizeof(int64_t));
    lib_send_message(send_data, message_len, DEALLOC_OP_PORT);
    free(send_data);
}

void send_dealloc_bufmeta_req(void* send_buf, int dealloc_blk_num, int64_t ret_type)
{
    int64_t message_len = (dealloc_blk_num+4) * sizeof(int64_t);
    void* send_data = malloc(message_len);

    int64_t* sd_int64_pt = (int64_t*)send_data;
    sd_int64_pt[0] = DEALLOC_BUFMETA_FUNC;
    sd_int64_pt[1] = dealloc_blk_num;
    sd_int64_pt[2] = ret_type;
    sd_int64_pt[3] = lib_register_pid;

    void* dealloc_blkid_pt = (void*)(sd_int64_pt + 4);
    memcpy(dealloc_blkid_pt, send_buf, dealloc_blk_num * sizeof(int64_t));
    lib_send_message(send_data, message_len, DEALLOC_OP_PORT);
    free(send_data);
}

void send_dealloc_page_req(void* send_buf, int dealloc_blk_num, int64_t ret_type)
{
    int64_t message_len = (dealloc_blk_num+4) * sizeof(int64_t);
    void* send_data = malloc(message_len);

    int64_t* sd_int64_pt = (int64_t*)send_data;
    sd_int64_pt[0] = DEALLOC_PAGE_FUNC;
    sd_int64_pt[1] = dealloc_blk_num;
    sd_int64_pt[2] = ret_type;
    sd_int64_pt[3] = lib_register_pid;

    void* dealloc_blkid_pt = (void*)(sd_int64_pt + 4);
    memcpy(dealloc_blkid_pt, send_buf, dealloc_blk_num * sizeof(int64_t));
    lib_send_message(send_data, message_len, DEALLOC_OP_PORT);
    free(send_data);
}

void send_dealloc_block_req(void* send_buf, int dealloc_blk_num, int64_t ret_type)
{
    int64_t message_len = (dealloc_blk_num+4) * sizeof(int64_t);
    void* send_data = malloc(message_len);

    int64_t* sd_int64_pt = (int64_t*)send_data;
    sd_int64_pt[0] = DEALLOC_BLOCK_FUNC;
    sd_int64_pt[1] = dealloc_blk_num;
    sd_int64_pt[2] = ret_type;
    sd_int64_pt[3] = lib_register_pid;

    void* dealloc_blkid_pt = (void*)(sd_int64_pt + 4);
    memcpy(dealloc_blkid_pt, send_buf, dealloc_blk_num * sizeof(int64_t));
    lib_send_message(send_data, message_len, DEALLOC_OP_PORT);
    free(send_data);
}

/*-------------------------log--------------------------*/
int64_t request_log_seg()
{
    int64_t send_buf[5];
    send_buf[0] = ALLOC_LOG_SEG_CODE;
    send_buf[1] = lib_register_pid;
    void* send_data = (void*)send_buf;

    reqlock_lock();
    lib_send_message(send_data, sizeof(int64_t)*2, LOG_OP_PORT);
    void* ret_info_buf = malloc(4096);
    lib_recv_message(RECV_LOG_SP_PORT + lib_register_pid, ret_info_buf, sizeof(int64_t)*2);
    reqlock_unlock();

    int64_t* ret_info_int64 = (int64_t*)ret_info_buf;
    int64_t ret_seg_id = ret_info_int64[1];
    free(ret_info_buf);
    return ret_seg_id;
}

/*-------------------------req process id--------------------------*/
int64_t request_process_id()
{

    int64_t send_buf[5];
    send_buf[0] = REGISTER_CODE;
    void* send_data = (void*)send_buf;

    lib_send_message(send_data, sizeof(int64_t), REGISTER_PORT);
    void* ret_info_buf = malloc(4096);
    lib_recv_message(RECV_PID_PORT, ret_info_buf, sizeof(int64_t)*2);

    int64_t* ret_info_int64 = (int64_t*)ret_info_buf;
    int64_t ret_pid = ret_info_int64[1];
    free(ret_info_buf);
    return ret_pid;
}

#ifdef __cplusplus
}
#endif