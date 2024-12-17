#ifndef LIB_SOCKET_H
#define LIB_SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>


#define LIB_MAX_PORT_NUM                   32

#define LWORK_USED                         1
#define LWORK_NOT_USED                     0

struct port_basic_info
{
    struct sockaddr_in lip_port;
    int64_t socket_fd;
    int32_t port_type;
    int32_t work_flag;
    int32_t port_num;
    int32_t used_flag;
};
typedef struct port_basic_info port_binfo_t;
typedef port_binfo_t* port_binfo_pt;

port_binfo_t lrecv_port[LIB_MAX_PORT_NUM];
port_binfo_t lsend_port[LIB_MAX_PORT_NUM];
int64_t lib_register_pid;


void init_lib_socket_info();


void free_lib_socket_info();

/* The function of receiving information in user space
 */
int lib_recv_message(int64_t port, void* recv_buf, int recv_info_len);

/* Function for sending information in user space
 */
void lib_send_message(void* send_data, int data_len, int64_t target_port);

#endif