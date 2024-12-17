#include "../config/protocol.h"
#include "../config/config.h"
#include "../config/socket_config.h"

#include "kernel_func.h"
#include "kernel_socket.h"
#include "addr_util.h"
#include "balloc.h"
#include "device.h"
#include "type.h"
#include "log.h"
#include "thdpool.h"

#ifdef __cplusplus       
extern "C"{
#endif

void listen_close_message()
{
    while(1)
    {
        int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(socket_fd == -1)
        {
            printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
            exit(0);
        }
        struct sockaddr_in ip_port;
        ip_port.sin_family = AF_INET; 
        ip_port.sin_addr.s_addr = inet_addr(LOCAL_IP);
        ip_port.sin_port = htons(CLOSE_FS_PORT);

        int opt = 1;   
        setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        struct sockaddr_in* ip_addr = &ip_port;
        int bind_ret = bind(socket_fd, (struct sockaddr*)ip_addr, sizeof(struct sockaddr_in));
        if(bind_ret < 0)
        {
            printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
            exit(0);
        }
        int lis_ret = listen(socket_fd, 10);
        if(lis_ret < 0)
        {
            printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
            exit(0);
        }

        int connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL);
        if(connect_fd < 0)
        {
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
            exit(1);
        }

        void* buffer_sp = malloc(1024*1024);
        // printf("buffer recv!\n");
        int recv_len = recv(connect_fd, buffer_sp, 1024, 0);
        int64_t* buffer_int64_pt = (int64_t*)buffer_sp;
        if(buffer_int64_pt[0] == CLOSE_FS_MESSAGE)
        {
            close(socket_fd);
            close(connect_fd); //*****************
            free(buffer_sp);
            break;
        }
        free(buffer_sp);
        close(socket_fd);
        close(connect_fd); //*****************
    }
    return;
}

int main()
{
    init_kernelFS();
    fprintf(stderr,"init fs end!\n");
    listen_close_message();
    fprintf(stderr,"kernel fs is working!\n");
    close_kernelFS();
    fprintf(stderr,"fs closed!\n");
    return 0;
}

#ifdef __cplusplus
}
#endif