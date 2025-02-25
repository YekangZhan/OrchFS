#include "lib_socket.h"
#include "req_kernel.h"
#include "../config/socket_config.h"
#include "../config/protocol.h"

#ifdef __cplusplus       
extern "C"{
#endif

void init_send_port_basic_info(int64_t now_port)
{
    for(int i = 0; i < LIB_MAX_PORT_NUM; i++)
    {
        if(lsend_port[i].used_flag == LWORK_USED)
            continue;
        lsend_port[i].used_flag = LWORK_USED;
        lsend_port[i].port_type = LIB_SEND_PORT;
        lsend_port[i].port_num = now_port;
        lsend_port[i].socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(lsend_port[i].socket_fd == -1)
        {
            printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
            exit(0);
        }
        lsend_port[i].lip_port.sin_family = AF_INET; 
        lsend_port[i].lip_port.sin_addr.s_addr = inet_addr(LOCAL_IP);
        lsend_port[i].lip_port.sin_port = htons(now_port);
        lsend_port[i].work_flag = CAN_WORK;
        break;
    }
}


void init_recv_port_basic_info(int64_t now_port)
{
    for(int i = 0; i < LIB_MAX_PORT_NUM; i++)
    {
        if(lrecv_port[i].used_flag == LWORK_USED)
            continue;
        lrecv_port[i].used_flag = LWORK_USED;
        lrecv_port[i].port_type = LIB_LISTEN_PORT;
        lrecv_port[i].port_num = now_port;
        lrecv_port[i].socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(lrecv_port[i].socket_fd == -1)
        {
            printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
            exit(0);
        }

        int opt = 1, new_buf_size = 192*1024;    /* The size of the sending buffer */
        setsockopt(lrecv_port[i].socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(lrecv_port[i].socket_fd, SOL_SOCKET, SO_RCVBUF, &new_buf_size, sizeof(new_buf_size)); 

        lrecv_port[i].lip_port.sin_family = AF_INET; 
        lrecv_port[i].lip_port.sin_addr.s_addr = inet_addr(LOCAL_IP);
        lrecv_port[i].lip_port.sin_port = htons(now_port);

        // Call the bind function to bind
        struct sockaddr_in* lip_addr = &(lrecv_port[i].lip_port);
        int bind_ret = bind(lrecv_port[i].socket_fd, (struct sockaddr*)lip_addr, sizeof(struct sockaddr_in));
        if(bind_ret < 0)
        {
            printf("bind socket error1: %s(errno: %d)\n",strerror(errno),errno);
            exit(0);
        }
        lrecv_port[i].work_flag = CAN_WORK;
        break;
    }
}

void init_lib_socket_info()
{
    for(int i = 0; i < LIB_MAX_PORT_NUM; i++)
    {
        lsend_port[i].used_flag = LWORK_NOT_USED;
        lsend_port[i].work_flag = CAN_NOT_WORK;
    }
    for(int i = 0; i < LIB_MAX_PORT_NUM; i++)
    {
        lrecv_port[i].used_flag = LWORK_NOT_USED;
        lrecv_port[i].work_flag = CAN_NOT_WORK;
    }
    init_send_port_basic_info(ALLOC_OP_PORT);
    init_send_port_basic_info(DEALLOC_OP_PORT);
    init_send_port_basic_info(LOG_OP_PORT);
    init_send_port_basic_info(CACHE_OP_PORT);
    init_send_port_basic_info(CLOSE_FS_PORT);
    init_send_port_basic_info(REGISTER_PORT);


    init_recv_port_basic_info(RECV_PID_PORT);
    sleep(1);
    fprintf(stderr,"lib_register_pid begin\n");
    lib_register_pid = request_process_id();
    fprintf(stderr,"lib_register_pid: %lld\n",lib_register_pid);
    for(int i = 0; i < LIB_MAX_PORT_NUM; i++)
    {
        if(lrecv_port[i].used_flag == LWORK_USED)
        {
            close(lrecv_port[i].socket_fd);
        }
        lrecv_port[i].work_flag = CAN_NOT_WORK;
        lrecv_port[i].used_flag = LWORK_NOT_USED;
    }

    init_recv_port_basic_info(RECV_BLK_PORT + lib_register_pid);
    init_recv_port_basic_info(RECV_LOG_SP_PORT + lib_register_pid);
    init_recv_port_basic_info(RECV_CACHE_PORT + lib_register_pid);
}

void free_lib_socket_info()
{
    for(int i = 0; i < LIB_MAX_PORT_NUM; i++)
    {
        if(lsend_port[i].used_flag == LWORK_USED)
        {
            close(lsend_port[i].socket_fd);
        }
        lsend_port[i].work_flag = CAN_NOT_WORK;
        lsend_port[i].used_flag = LWORK_NOT_USED;
    }
    for(int i = 0; i < LIB_MAX_PORT_NUM; i++)
    {
        if(lrecv_port[i].used_flag == LWORK_USED)
        {
            close(lrecv_port[i].socket_fd);
        }
        lrecv_port[i].work_flag = CAN_NOT_WORK;
        lrecv_port[i].used_flag = LWORK_NOT_USED;
    }
}

int lib_recv_message(int64_t port, void* recv_buf, int recv_info_len)
{
    int pos = -1;
    for(int i = 0; i < LIB_MAX_PORT_NUM; i++)
    {
        if(lrecv_port[i].used_flag == LWORK_USED && lrecv_port[i].port_num == port)
        {
            pos = i; break;
        }
    }
    if(pos == -1)
    {
        fprintf(stderr, "error port: %lld\n",port);
        fflush(stdout);
        goto port_error;
    }

    // Check the size of the buffer zone
    int check_len = 0, check_opt_len = sizeof(int);
    getsockopt(lrecv_port[pos].socket_fd , SOL_SOCKET , SO_RCVBUF , &check_len , &check_opt_len);
    if(check_len < 192*1024)
    {
        printf("lib recv buffer is too small!\n");
        exit(0);
    }

    int lis_ret = listen(lrecv_port[pos].socket_fd, 10);
    if(lis_ret < 0)
    {
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
     
    int connect_fd = accept(lrecv_port[pos].socket_fd, (struct sockaddr*)NULL, NULL);
    if(connect_fd < 0)
    {
        printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
        exit(1);
    }
    // Accept data transmitted from the client
    int recv_off = 0;
    while (1) 
    {
        int bytes_read = recv(connect_fd, recv_buf + recv_off, MAX_BUFFER_LEN, 0);
        // fprintf(stderr,"bytes_read: %d\n",bytes_read);
        if(recv_off + bytes_read >= recv_info_len)
        {
            recv_off += bytes_read; 
            break;
        }
        if(bytes_read == -1) 
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) 
            {
                break;
            }
   		    printf("kernel socket unkonwn error!\n");
            exit(0);
        } 
        else if(bytes_read == 0) 
        {
            break; 
        }
        recv_off += bytes_read;     
    }
    close(connect_fd);
    return recv_off;
    // int recv_len = recv(connect_fd, recv_buf, MAX_BUFFER_LEN, 0);
    // return recv_len;
init_error:
    printf("The process of init may be error!\n");
    exit(1);
port_error:
    printf("The port is error!\n");
    exit(1);
}

void lib_send_message(void* send_data, int data_len, int64_t target_port)
{   
    int port_info_pos = -1;
    // if(target_port == CLOSE_FS_PORT)
    //     fprintf("target_pos: %lld\n",target_port);
    for(int i = 0; i < LIB_MAX_PORT_NUM; i++)
    {
        // fprintf(stderr,"port: %d %lld\n",lsend_port[i].used_flag,lsend_port[i].port_num);
        if(lsend_port[i].used_flag == LWORK_USED && lsend_port[i].port_num == target_port)
        {
            port_info_pos = i; break;
        }
    }
    if(port_info_pos == -1)
        goto port_error;
    

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd == -1)
    {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }

    int new_buf_size = 192*1024; 
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &new_buf_size, sizeof(new_buf_size)); 

    // printf("socketed\n");
    struct sockaddr_in* lip_addr = &(lsend_port[port_info_pos].lip_port);
    int con_ret = connect(sock_fd, (struct sockaddr*)lip_addr, sizeof(struct sockaddr_in));
    if(con_ret < 0)
    {
        perror("connected failed");
        exit(0);
    }
    // printf("connected\n");

    int check_len = 0, check_opt_len = sizeof(int);
    getsockopt(sock_fd , SOL_SOCKET , SO_SNDBUF , &check_len , &check_opt_len);
    if(check_len < 192*1024)
    {
        printf("lib send buffer is too small!\n");
        exit(0);
    }

    int send_ret = send(sock_fd, send_data, data_len, 0);
    if(send_ret < 0)
    {
        printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    close(sock_fd);
    return;
port_error:
    printf("The port is error! -- lib_send_message\n");
    exit(1);
}

#ifdef __cplusplus
}
#endif