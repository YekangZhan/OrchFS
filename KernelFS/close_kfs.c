#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <pthread.h>

#include "../KernelFS/device.h"
#include "../KernelFS/type.h"
#include "../config/config.h"
#include "../config/socket_config.h"

void req_kernelfs_close()
{
    int64_t send_buf[5];
    for(int i = 0; i < 5; i++)
        send_buf[i] = CLOSE_FS_MESSAGE;
    void* send_data = (void*)send_buf;

    struct sockaddr_in lip_port;
    lip_port.sin_family = AF_INET; 
    lip_port.sin_addr.s_addr = inet_addr(LOCAL_IP);
    lip_port.sin_port = htons(CLOSE_FS_PORT);
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd == -1)
    {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    struct sockaddr_in* lip_addr = &lip_port;
    int con_ret = connect(sock_fd, (struct sockaddr*)lip_addr, sizeof(struct sockaddr_in));
    if(con_ret < 0)
    {
        perror("connected failed");
        exit(0);
    }

    int send_ret = send(sock_fd, send_data, sizeof(int64_t)*3, 0);
    if(send_ret < 0)
    {
        printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    close(sock_fd);
    fprintf(stderr,"close message sended!\n");
    return;
}

int main()
{
    fprintf(stderr,"will close kernelfs!\n");
	req_kernelfs_close();
    return 0;
}