#include "kernel_socket.h"
#include "thdpool.h"
#include "../config/socket_config.h"
#include "../config/protocol.h"

#ifdef __cplusplus       
extern "C"{
#endif

// #define KERNEL_RECV_DEBUG


void init_recv_port_basic_info(int64_t now_port)
{
     for(int i = 0; i < MAX_PORT_NUM; i++)
     {
          if(krecv_port[i].used_flag == KWORK_USED)
               continue;
          krecv_port[i].used_flag = KWORK_USED;
          krecv_port[i].port_type = KERNEL_LISTEN_PORT;
          krecv_port[i].port_num = now_port;
          krecv_port[i].socket_fd = socket(AF_INET, SOCK_STREAM, 0);
          if(krecv_port[i].socket_fd == -1)
          {
               printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
               exit(0);
          }

          int opt = 1, new_buf_size = 192*1024;  
          setsockopt(krecv_port[i].socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
          setsockopt(krecv_port[i].socket_fd, SOL_SOCKET, SO_RCVBUF, &new_buf_size, sizeof(new_buf_size)); 

          krecv_port[i].kip_port.sin_family = AF_INET; 
          krecv_port[i].kip_port.sin_addr.s_addr = inet_addr(LOCAL_IP);
          krecv_port[i].kip_port.sin_port = htons(now_port);
          // printf("port1: %lld\n",now_port);
    
          struct sockaddr_in* kip_addr = &(krecv_port[i].kip_port);
          int bind_ret = bind(krecv_port[i].socket_fd, (struct sockaddr*)kip_addr, sizeof(struct sockaddr_in));
          if(bind_ret < 0)
          {
               printf("bind socket error2: %s(errno: %d)\n",strerror(errno),errno);
               exit(0);
          }
          krecv_port[i].recv_buf = malloc(MAX_BUFFER_LEN);
          krecv_port[i].work_flag = CAN_WORK;
          break;
     }
}

void init_kernel_recv_info()
{
     for(int i = 0; i < MAX_PORT_NUM; i++)
     {
          krecv_port[i].used_flag = KWORK_NOT_USED;
          krecv_port[i].work_flag = CAN_NOT_WORK;
     }
     init_recv_port_basic_info(ALLOC_OP_PORT);
     init_recv_port_basic_info(DEALLOC_OP_PORT);
     init_recv_port_basic_info(LOG_OP_PORT);
     init_recv_port_basic_info(CACHE_OP_PORT);
     init_recv_port_basic_info(REGISTER_PORT);

     // init_recv_port_basic_info(CLOSE_FS_PORT);
}


void* get_end_message()
{
     void* data0 = malloc(sizeof(int32_t) * END_MESSAGE_SEG_NUM);
     if(data0 == NULL)
          goto alloc_error;
     int32_t* data0_pt = (int32_t*)data0; 
     for(int i = 0; i < END_MESSAGE_SEG_NUM; i++)
          data0_pt[i] = END_MESSAGE;
     return data0;
alloc_error:
     printf("alloc memort error!\n");
     exit(1);
}

void free_kernel_recv_info()
{
     void* end_meg_pt = get_end_message();
     send_close_message(end_meg_pt, END_MESSAGE_LEN, ALLOC_OP_PORT);
     send_close_message(end_meg_pt, END_MESSAGE_LEN, DEALLOC_OP_PORT);
     send_close_message(end_meg_pt, END_MESSAGE_LEN, LOG_OP_PORT);
     send_close_message(end_meg_pt, END_MESSAGE_LEN, CACHE_OP_PORT);
     send_close_message(end_meg_pt, END_MESSAGE_LEN, REGISTER_PORT);
     free(end_meg_pt);
     for(int i = 0; i < KER_LISTEN_PORT_NUM; i++)
          sem_wait(recv_end_sure + i);
     for(int i = 0; i < MAX_PORT_NUM; i++)
     {
          if(krecv_port[i].used_flag == KWORK_USED)
          {
               close(krecv_port[i].socket_fd);
          }
          free(krecv_port[i].recv_buf);
          krecv_port[i].work_flag = CAN_NOT_WORK;
          krecv_port[i].used_flag = KWORK_NOT_USED;
     }
}


int check_end(void* message_pt)
{
     int32_t* message_int32_pt = (int32_t*)message_pt; 
     for(int i = 0; i < END_MESSAGE_SEG_NUM; i++)
     {
          if(message_int32_pt[i] != END_MESSAGE)
               return 0;
     }
     return 1;
}

void listen_operation(int pos)
{

     int check_len = 0, check_opt_len = sizeof(int);
     getsockopt(krecv_port[pos].socket_fd , SOL_SOCKET , SO_RCVBUF , &check_len , &check_opt_len);
     if(check_len < 192*1024)
     {
          printf("kernel recv buffer is too small! %d\n",check_len);
          exit(0);
     }

     int lis_ret = listen(krecv_port[pos].socket_fd, 10);
     if(lis_ret < 0)
     {
          printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
          exit(0);
     }

     int connect_fd = accept(krecv_port[pos].socket_fd, (struct sockaddr*)NULL, NULL);
     if(connect_fd < 0)
     {
          printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
          exit(1);
     }

     // int recv_off = 0;
     // while (1) 
     // {
     //      int bytes_read = recv(connect_fd, krecv_port[pos].recv_buf + recv_off, MAX_BUFFER_LEN, 0);
     //      fprintf(stderr,"bytes_read: %d\n",bytes_read);
     //      if (bytes_read == -1) 
     //      {
     //           if (errno == EAGAIN || errno == EWOULDBLOCK) 
     //           {
     //                break; //表示没有数据了
     //           }
   	// 	     printf("kernel socket unkonwn error!\n");
     //           exit(0);
     //      } 
     //      else if (bytes_read == 0) 
     //      {
     //           break; 
     //      }
     //      recv_off += bytes_read;     
     // }
     // return recv_off;
     int recv_len = recv(connect_fd, krecv_port[pos].recv_buf, MAX_BUFFER_LEN, 0);
     close(connect_fd); //*****************
}

void listen_alloc_op()
{
     int pos = -1;
     for(int i = 0; i < MAX_PORT_NUM; i++)
     {
          if(krecv_port[i].used_flag == KWORK_USED && krecv_port[i].port_num == ALLOC_OP_PORT)
          {
               pos = i; break;
          }
     }
     // printf("alloc pos: %d\n",pos);
     if(pos == -1)
          goto init_error;
     while(1)
     {
          listen_operation(pos);

          int end_flag = check_end(krecv_port[pos].recv_buf);
          if(end_flag == 1)
          {
               sem_post(recv_end_sure + pos);
               break;
          }

          thd_task_pt task0 = malloc(sizeof(thd_task_t));
          int64_t* recv_int64_pt = (int64_t*)krecv_port[pos].recv_buf;
          task0->func_type = recv_int64_pt[0];
          if(task0->func_type >= MIN_ALLOC_FUNC_CODE && task0->func_type <= MAX_ALLOC_FUNC_CODE)
          {
               task0->error_flag = TYPE_NOT_ERROR;
               int64_t para_len = sizeof(int64_t) * 3;
               task0->para_space = malloc(para_len);
               void* para_start_pt = (void*)(recv_int64_pt + 1);
               memcpy(task0->para_space, para_start_pt, para_len);

               // debug
               int64_t* debug = (int64_t*)task0->para_space;
               // printf("info 0: %lld %lld %lld\n",debug[0],debug[1],task0->func_type);
               add_task_to_queue(ALLOC_TID- OP_START_TID, task0);
          }
          else
          {
               task0->error_flag = TYPE_ERROR;
               task0->para_space = NULL;
               add_task_to_queue(ALLOC_TID- OP_START_TID, task0);
          }
          free(task0);
     }
     return;
init_error:
     printf("The process of init may be error!\n");
     exit(1);
}

void listen_dealloc_op()
{
     int pos = -1;
     for(int i = 0; i < MAX_PORT_NUM; i++)
     {
          if(krecv_port[i].used_flag == KWORK_USED && krecv_port[i].port_num == DEALLOC_OP_PORT)
          {
               pos = i; break;
          }
     }
     // printf("dealloc pos: %d\n",pos);
     if(pos == -1)
          goto init_error;
     while(1)
     {
          // printf("listen begin! %lld\n", krecv_port[pos].port_num);
          listen_operation(pos);
          // printf("listen end!\n");

          int end_flag = check_end(krecv_port[pos].recv_buf);
          if(end_flag == 1)
          {
               sem_post(recv_end_sure + pos);
               break;
          }

          thd_task_pt task0 = malloc(sizeof(thd_task_t));
          int64_t* recv_int64_pt = (int64_t*)krecv_port[pos].recv_buf;
          task0->func_type = recv_int64_pt[0];
          if(task0->func_type >= MIN_DEALLOC_FUNC_CODE && task0->func_type <= MAX_DEALLOC_FUNC_CODE)
          {
               int64_t para_len = recv_int64_pt[1]*sizeof(int64_t) + sizeof(int64_t)*3;

#ifdef KERNEL_RECV_DEBUG
               printf("recv\n");
               for(int i = 0; i < recv_int64_pt[1]+3; i++)
                    printf("%" PRId64" ",recv_int64_pt[i]);
               printf("\n");
#endif

               void* para_start_pt = (void*)(recv_int64_pt + 1);
               task0->para_space = malloc(para_len);
               memcpy(task0->para_space, para_start_pt, para_len);

               add_task_to_queue(DEALLOC_TID - OP_START_TID,  task0);
               // free(task0->para_space);
          }
          else
          {
               task0->error_flag = TYPE_ERROR;
               task0->para_space = NULL;
               add_task_to_queue(DEALLOC_TID - OP_START_TID, task0);
          }
          free(task0);
     }
     return;
init_error:
     printf("The process of init may be error!\n");
     exit(1);
}

void listen_log_op()
{
     int pos = -1;
     for(int i = 0; i < MAX_PORT_NUM; i++)
     {
          if(krecv_port[i].used_flag == KWORK_USED && krecv_port[i].port_num == LOG_OP_PORT)
          {
               pos = i; break;
          }
     }
     // printf("log pos: %d\n",pos);
     if(pos == -1)
          goto init_error;
     while(1)
     {
          listen_operation(pos);

          int end_flag = check_end(krecv_port[pos].recv_buf);
          if(end_flag == 1)
          {
               sem_post(recv_end_sure + pos);
               break;
          }

          thd_task_pt task0 = malloc(sizeof(thd_task_t));
          int64_t* recv_int64_pt = (int64_t*)krecv_port[pos].recv_buf;
          task0->func_type = recv_int64_pt[0];
          if(task0->func_type >= MIN_LOG_FUNC_CODE && task0->func_type <= MAX_LOG_FUNC_CODE)
          {
               task0->error_flag = TYPE_NOT_ERROR;
               int64_t para_len = 8;
               task0->para_space = malloc(para_len);
               void* para_start_pt = (void*)(recv_int64_pt + 1);
               memcpy(task0->para_space, para_start_pt, para_len);

               add_task_to_queue(LOG_TID- OP_START_TID, task0);
          }
          else
          {
               task0->error_flag = TYPE_ERROR;
               task0->para_space = NULL;
               add_task_to_queue(LOG_TID- OP_START_TID, task0);
          }
          free(task0);

     }
     return;
init_error:
     printf("The process of init may be error!\n");
     exit(1);
}

void listen_register_op()
{
     int pos = -1;
     for(int i = 0; i < MAX_PORT_NUM; i++)
     {
          if(krecv_port[i].used_flag == KWORK_USED && krecv_port[i].port_num == REGISTER_PORT)
          {
               pos = i; break;
          }
     }
     fflush(stdout);
     if(pos == -1)
          goto init_error;
     while(1)
     {
          listen_operation(pos);

          int end_flag = check_end(krecv_port[pos].recv_buf);
          if(end_flag == 1)
          {
               sem_post(recv_end_sure + pos);
               break;
          }

          thd_task_pt task0 = malloc(sizeof(thd_task_t));
          int64_t* recv_int64_pt = (int64_t*)krecv_port[pos].recv_buf;
          task0->func_type = recv_int64_pt[0];
          if(task0->func_type >= MIN_RIG_FUNC_CODE && task0->func_type <= MAX_RIG_FUNC_CODE)
          {
               task0->error_flag = TYPE_NOT_ERROR;
               task0->para_space = NULL;
               add_task_to_queue(RIGESTER_TID- OP_START_TID, task0);
          }
          else
          {
               task0->error_flag = TYPE_ERROR;
               task0->para_space = NULL;
               add_task_to_queue(RIGESTER_TID- OP_START_TID, task0);
          }
          free(task0);

     }
     return;
init_error:
     printf("The process of init may be error!\n");
     exit(1);
}

void listen_cache_op()
{
     int pos = -1;
     for(int i = 0; i < MAX_PORT_NUM; i++)
     {
          if(krecv_port[i].used_flag == KWORK_USED && krecv_port[i].port_num == CACHE_OP_PORT)
          {
               pos = i; break;
          }
     }
     // printf("cache pos: %d\n",pos);
     if(pos == -1)
          goto init_error;
     while(1)
     {
          listen_operation(pos);

          int end_flag = check_end(krecv_port[pos].recv_buf);
          if(end_flag == 1)
          {
               sem_post(recv_end_sure + pos);
               break;
          }

          // thd_task_pt task0 = malloc(sizeof(thd_task_t));
     }
     return;
init_error:
     printf("The process of init may be error!\n");
     exit(1);
}

#ifdef __cplusplus
}
#endif
