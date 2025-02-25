#include <assert.h>
#include <emmintrin.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include <libpmem.h>
//#include <libpmem2.h>
#include <sys/time.h>
#include <time.h>

#include "../src/comfs.h"
#include "../src/comfs_runtime.h"
#include "../src/comfs_config.h"
#include "../src/device.h"
#include "../src/radix.h"

#define SINGLE_READ_MAX_SIZE 1ULL*1024*1024*1024*1
#define SINGLE_WRITE_MAX_SIZE 1ULL*1024*1024*1024*1
#define MAX_QUERY_READ_BLOCK 1024*256
#define MAX_QUERY_WRITE_BLOCK 1024*256
#define SLOT_NUM (SIZE_256B/8)

void Sequential_writing(uint64_t block_type,uint64_t write_byte,uint64_t* offset_start,
                        uint64_t* write_time,radix_node* tree0)
{
    struct timespec stopwatch_start;
	struct timespec stopwatch_stop;
    long time_diff;
    if(block_type==BLOCK_4KB)
    {
        char* buf = malloc(sizeof(char)*write_byte);
        uint64_t* buf_store = (uint64_t*)buf;
        for(uint64_t i = 0; i < write_byte/SIZE_1KB; i++)
        {
            for(int j = 0; j < SIZE_1KB/8; j++)
                buf_store[i*SIZE_1KB/8+j] = *offset_start+i;
        }
        uint64_t write_block = write_byte/SIZE_1KB;
        if(write_byte%SIZE_1KB!=0)
            write_block++;
        clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
        //printf("%llu\n",(*offset_start)*256);
        nvm_write(buf, write_byte, (*offset_start)*256);
        clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
        time_diff = calc_diff(stopwatch_start, stopwatch_stop);
        *offset_start += write_block;
        *write_time += time_diff;
        free(buf);
    }
    // printf("write:%ldns\n",alltime);
}

void Sequential_reading(uint64_t block_type,uint64_t read_byte,uint64_t* offset_start,
                        uint64_t* read_time,radix_node* tree0)
{
    struct timespec stopwatch_start;
	struct timespec stopwatch_stop;
    long time_diff;
    if(block_type==BLOCK_4KB)
    {
        char* buf = malloc(sizeof(char)*read_byte);
        // uint64_t* buf_store = (uint64_t*)buf;
        uint64_t read_block = read_byte/SIZE_1KB;
        if(read_byte%SIZE_1KB!=0)
            read_block++;
        clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
        nvm_read(buf, read_byte, (*offset_start)*256);
        clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
        time_diff = calc_diff(stopwatch_start, stopwatch_stop);
        *read_time += time_diff;
        // for(uint64_t i = 0; i < read_byte/SIZE_256B; i++)
        // {
        //     for(int j = 0; j < SIZE_256B/8; j++)
        //         assert(buf_store[i*SIZE_256B/8+j]==*offset_start+i);
        // }
        *offset_start += read_block;
        free(buf);
    }
    // printf("read:%ldns\n",alltime);
}


int main()
{
    // void* tree0=create_tree_root();
    // long readtime = 0,writetime = 0;
    // uint64_t read_off_start = 0,write_off_start = 0;
    // /*顺序读写*/
    // Sequential_writing(BLOCK_1KB,SINGLE_WRITE_MAX_SIZE,&write_off_start,&writetime,tree0);
    // printf("write time:%ldns\n",writetime);
    // Sequential_reading(BLOCK_1KB,SINGLE_READ_MAX_SIZE,&read_off_start,&readtime,tree0);
    // printf("read time:%ldns\n",readtime);
    /*顺序块读写*/
    // for(uint64_t i = 0; i < SINGLE_WRITE_MAX_SIZE/SIZE_256B; i++)
    // {
    //     Sequential_writing(BLOCK_256B,SIZE_256B,&write_off_start,&writetime,tree0);
    // }
    // printf("write time:%ldns\n",writetime);
    // for(uint64_t i = 0; i < SINGLE_READ_MAX_SIZE/SIZE_256B; i++)
    // {
    //     Sequential_reading(BLOCK_256B,SIZE_256B,&read_off_start,&readtime,tree0);
    // }
    // printf("read time:%ldns\n",readtime);
}