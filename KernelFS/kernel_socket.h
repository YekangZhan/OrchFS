#ifndef KERNEL_SOCKET_H
#define KERNEL_SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <inttypes.h>
#include <fcntl.h>
#include <semaphore.h>


#define MAX_PORT_NUM                   8
#define KER_LISTEN_PORT_NUM            5

#define KWORK_USED                     1
#define KWORK_NOT_USED                 0

struct port_basic_info
{
    struct sockaddr_in kip_port;
    int64_t socket_fd;
    int32_t port_type;
    int32_t work_flag;
    int32_t port_num;
    int32_t used_flag;
    void* recv_buf;
};
typedef struct port_basic_info port_binfo_t;
typedef port_binfo_t* port_binfo_pt;

port_binfo_t krecv_port[MAX_PORT_NUM];
port_binfo_t ksend_port[MAX_PORT_NUM];
sem_t recv_end_sure[KER_LISTEN_PORT_NUM];


void init_kernel_recv_info();


void init_kernel_send_info();


void free_kernel_recv_info();


void free_kernel_send_info();


void listen_alloc_op();


void listen_dealloc_op();


void listen_log_op();


void listen_closefs_op();


void listen_cache_op();


void listen_register_op();


void kernel_send_message(void* send_data, int data_len, int64_t target_port);


void send_close_message(void* send_data, int data_len, int64_t target_port);

#endif