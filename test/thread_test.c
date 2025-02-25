#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>

#include "../src/comfs.h"
#include "../src/comfs_runtime.h"
#include "../src/comfs_config.h"
#include "../src/device.h"
#include "../src/radix.h"

#define BLK_MAX 1ULL*1024*1024
#define GIGABYTE 1ULL*1024*1024*1024*1
// #define SIZE_4KB 4096

uint64_t ssd_blk_addr[BLK_MAX], nvm_blk_addr[BLK_MAX];
uint64_t nvm_flag[BLK_MAX];
int64_t pos_list[BLK_MAX];

uint64_t myrand()
{
    uint64_t a = rand(),b = rand(),c = rand(),ret = rand();
    ret += (a<<15)+(b<<30)+(c<<45);
    return ret;
}

void buf_test()
{
    srand(time(NULL));
    char* writebuf = malloc(GIGABYTE*sizeof(char));
    char* readbuf = malloc(GIGABYTE*sizeof(char));
    printf("memstart:%"PRIu64" %"PRIu64"\n",writebuf,readbuf);
    int64_t filesize = 32*1024*1024;
    for(int i = 0; i <= filesize+16*1024; i++)
        writebuf[i] = (char)(rand()%128);
    void* tree0 = create_tree_root();
    tree_insert_and_write(tree0, 0, filesize+16*1024, 0, writebuf);
    // tree_search_and_read(tree0, 0, SIZE_4KB*1028, 0, readbuf);
    // for(int j = 0; j < SIZE_4KB*1028; j++)
    // {
    //     if(readbuf[j]!=writebuf[j])
    //     {
    //         printf("%d\n",j);
    //         assert(1==0);
    //     }
    // }
    int64_t wop_time = 1024*128;
    for(int64_t i = 0; i < wop_time; i++)
        pos_list[i] = myrand()%filesize;
    struct timespec stopwatch_start;
    struct timespec stopwatch_stop;
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
    for(int64_t i = 0; i < wop_time; i++)
    {
        // printf("wposinfo:%d %d %d\n",i,pos,op_size);
        tree_insert_and_write(tree0, pos_list[i], 1024, 0, writebuf+pos_list[i]);
        // tree_search_and_read(tree0, 0, SIZE_4KB*1028, 0, readbuf);
        // for(int j = 0; j < SIZE_4KB*1028; j++)
        // {
        //     if(readbuf[j]!=writebuf[j])
        //     {
        //         printf("%d %d\n",i,j);
        //         assert(1==0);
        //     }
        // }
        // getchar();
    }
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
    long alltime = calc_diff(stopwatch_start, stopwatch_stop);
    printf("avg latancy:%"PRIu64" ns\n",alltime/wop_time);
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
    tree_insert_and_write(tree0, 0, filesize, 0, writebuf);
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
    alltime = calc_diff(stopwatch_start, stopwatch_stop);
    printf("time:%"PRIu64" ns\n",alltime);
    printf("write success!\n");
    // int64_t rop_time = 1024;
    // for(int64_t i = 0; i < rop_time; i++)
    // {
    //     int pos = rand()%(SIZE_4KB*(1024-60)),op_size = rand()%(1024*64);
    //     printf("rposinfo:%d %d\n",pos,op_size);
    //     tree_search_and_read(tree0, pos, op_size, 0, readbuf);
    //     for(int j = 0; j < op_size; j++)
    //     {
    //         if(readbuf[j]!=writebuf[pos+j])
    //         {
    //             printf("%d %d\n",i,j);
    //             assert(1==0);
    //         }
    //     }
    //     // getchar();
    // }
    // printf("read success!\n");
}

int main()
{
    buf_test();
    // char* writebuf = valloc(16*GIGABYTE*sizeof(char));
    // for(uint64_t i = 0; i <= GIGABYTE*8; i+=32)
    // {
    //     writebuf[i] = rand()%128;
    // }
    // pth_pool * pool = malloc(sizeof(pth_pool));
    // init_pool(pool);
    // //测试1：每一个写4次256B的NVM，总共写2G，看不同线程数量下的性能
    // uint64_t test_blk = 1ULL*1024*256;
    // for(int i = 0; i < test_blk; i++)
    // {
    //     blk_addr[i] = block_16KB_alloc();
    // }
    // printf("block addr init!\n");
    // struct task * tsk_head = NULL;
    // int task_sum = 0;
    // for(int i = 0; i < test_blk; i+=1)
    // {
    //     struct task * tsk_node = malloc(sizeof(struct task));
    //     tsk_node->task_blk_type = BLOCK_16KB;
    //     tsk_node->task_opblk_sum = 1;
    //     tsk_node->task_id = 0;
    //     for(int j = 0; j < 1; j++)
    //     {
    //         tsk_node->task_blk_addr[j] = blk_addr[i+j];
    //         tsk_node->task_oplen[j] = SIZE_16KB;
    //         tsk_node->task_opbuf[j] = writebuf+(i*1LL)*SIZE_256B;
    //     }
    //     tsk_node->next = tsk_head;
    //     tsk_head = tsk_node;
    //     task_sum++;
    // }
    // // printf("task_sum:%d\n",task_sum);
    // printf("task init!\n");
    // struct timespec stopwatch_start;
	// struct timespec stopwatch_stop;
    // clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
    // int tsk_id = add_task(pool, tsk_head, task_sum);
    // // printf("task_id%d\n",tsk_id);
    // pthread_mutex_lock(&(pool->op_cond_lock)[tsk_id]);
    // while(pool->done[tsk_id]==0){
    //     pthread_cond_wait(&(pool->op_cond)[tsk_id],&(pool->op_cond_lock)[tsk_id]);
    // }
	// pthread_mutex_unlock(&(pool->op_cond_lock)[tsk_id]);
    // clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
    // long running_time = calc_diff(stopwatch_start, stopwatch_stop);
    // printf("time:%ld\n",running_time);
    // destroy_pool(pool);
    // free(writebuf);
    // return 0;
}