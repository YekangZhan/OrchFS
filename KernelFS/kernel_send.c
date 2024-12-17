#include "../config/socket_config.h"
#include "kernel_socket.h"

#ifdef __cplusplus       
extern "C"{
#endif


void init_send_port_basic_info(int64_t now_port)
{
    for(int i = 0; i < MAX_PORT_NUM; i++)
    {
        if(ksend_port[i].used_flag == KWORK_USED)
            continue;
        ksend_port[i].used_flag = KWORK_USED;
        ksend_port[i].port_type = KERNEL_SEND_PORT;
        ksend_port[i].port_num = now_port;
        ksend_port[i].socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        int new_buf_size = 192*1024; 
        setsockopt(ksend_port[i].socket_fd, SOL_SOCKET, SO_RCVBUF, &new_buf_size, sizeof(new_buf_size)); 
        if(ksend_port[i].socket_fd == -1)
        {
            printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
            exit(0);
        }
        ksend_port[i].kip_port.sin_family = AF_INET; 
        ksend_port[i].kip_port.sin_addr.s_addr = inet_addr(LOCAL_IP);
        ksend_port[i].kip_port.sin_port = htons(now_port);
        // printf("port: %lld\n",now_port);

        ksend_port[i].recv_buf = NULL;
        ksend_port[i].work_flag = CAN_WORK;
        break;
    }
}

void init_kernel_send_info()
{
    for(int i = 0; i < MAX_PORT_NUM; i++)
    {
        ksend_port[i].used_flag = KWORK_NOT_USED;
        ksend_port[i].work_flag = CAN_NOT_WORK;
    }
    init_send_port_basic_info(RECV_PID_PORT);
    for(int i = 0; i < 4; i++)
    {
        init_send_port_basic_info(RECV_BLK_PORT + i);
        init_send_port_basic_info(RECV_LOG_SP_PORT + i);
        init_send_port_basic_info(RECV_CACHE_PORT + i);
    }
}

void free_kernel_send_info()
{
    for(int i = 0; i < MAX_PORT_NUM; i++)
    {
        if(ksend_port[i].used_flag == KWORK_USED)
        {
            close(ksend_port[i].socket_fd);
        }
        ksend_port[i].work_flag = CAN_NOT_WORK;
        ksend_port[i].used_flag = KWORK_NOT_USED;
    }
}

int find_port_pos(int64_t port)
{
    for(int i = 0; i < MAX_PORT_NUM; i++)
    {
        if(ksend_port[i].used_flag == KWORK_USED && ksend_port[i].port_num == port)
            return i;
    }
    printf("The port is error!\n");
    exit(1);
}

void kernel_send_message(void* send_data, int data_len, int64_t target_port)
{   
    int port_info_pos = find_port_pos(target_port);

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd == -1)
    {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    int new_scobuf_size = 1024*1024; 
    int optlen = sizeof(int); 
    int err = setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &new_scobuf_size, optlen); 
    if(err < 0)
    { 
        printf("send buffer size set error!\n"); 
    } 
    struct sockaddr_in* kip_addr = &(ksend_port[port_info_pos].kip_port);
    // printf("target_port: %lld\n",target_port);
    int con_ret = connect(sock_fd, (struct sockaddr*)kip_addr, sizeof(struct sockaddr_in));
    if(con_ret < 0)
    {
        perror("connected failed");
        exit(0);
    }
    

    int check_len = 0, check_opt_len = sizeof(int);
    getsockopt(sock_fd , SOL_SOCKET , SO_SNDBUF , &check_len , &check_opt_len);
    if(check_len < 192*1024 || data_len > check_len)
    {
        printf("kernel send buffer is too small!\n");
        exit(0);
    }


    int send_ret = send(sock_fd, send_data, data_len, 0);
    if(send_ret < 0)
    {
        printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    close(sock_fd);
}

void send_close_message(void* send_data, int data_len, int64_t target_port)
{   
    int port_info_pos = -1;
    for(int i = 0; i < MAX_PORT_NUM; i++)
    {
        if(krecv_port[i].used_flag == KWORK_USED && krecv_port[i].port_num == target_port)
        {
            port_info_pos = i; break;
        }
    }
    if(port_info_pos == -1)
        goto port_error;

    // printf("port_info_pos: %d\n",port_info_pos);
    int close_soc_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(close_soc_fd == -1)
    {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    struct sockaddr_in* kip_addr = &(krecv_port[port_info_pos].kip_port);
    int con_ret = connect(close_soc_fd, (struct sockaddr*)kip_addr, sizeof(struct sockaddr_in));
    if(con_ret < 0)
    {
        perror("connected failed");
        exit(0);
    }

 
    int send_ret = send(close_soc_fd, send_data, data_len, 0);
    if(send_ret < 0)
    {
        printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    close(close_soc_fd);
    return;
port_error:
    printf("The port is error!\n");
    exit(1);
}

#ifdef __cplusplus
}
#endif