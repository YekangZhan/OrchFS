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
#include <stdlib.h>
#include <unistd.h>


long calc_diff(struct timespec start, struct timespec end){
	return (end.tv_sec * (long)(1000000000) + end.tv_nsec) -
	(start.tv_sec * (long)(1000000000) + start.tv_nsec);
}

int main()
{
    struct timespec stopwatch_start;
	struct timespec stopwatch_stop;
    int fd=open("/mnt/f2fstest/test1",O_WRONLY | __O_DIRECT | O_SYNC );//    | __O_DIRECT| O_SYNC  S_IRWXU
    int blocksize=4*1024;
    char* buf=(char*)valloc(blocksize);
    size_t* ind=malloc(sizeof(size_t)*1000*500);
    for(int i=0;i<blocksize;i++)
        buf[i]=i%256;
    for(size_t i=0;i<500000;i++)
        ind[i]=rand()%(4*1024)+i*4*1024;
        //ind[i]=i*(4*1024*2);
        //ind[i]=i*(4*1024);
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
    int num=200000;
    for(uint64_t i = 0; i < num; i++)
    {
        pread(fd,buf,blocksize,ind[i]);//
        //fsync(fd);
    }
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
    double time_diff = calc_diff(stopwatch_start, stopwatch_stop)*1.0/(long)(1000000000);
    printf("%fMB/s %fs\n",1.0*blocksize/1000*num/1000/time_diff,time_diff);
    printf("lantency=%.1fus\n",time_diff/num*1000000);
    close(fd);
    return 0;

    
    // struct timespec stopwatch_start;
	// struct timespec stopwatch_stop;
    // FILE* fp = fopen("/mnt/ext4test/checkpointfile", "r");   //打开文件
    // fseek(fp, 0, SEEK_END);      //将文件指针指向该文件的最后
    // long long file_size = ftell(fp); 
    // char* buf=(char*)valloc(file_size);
    // fseek(fp, 0, SEEK_SET); 
    // long long len=fread(buf, sizeof(char), file_size, fp); 
    // //printf("len=%lld file_size=%lld\n",len,file_size);
    // clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
    // int fd=open("\\/file",O_RDWR | O_CREAT, S_IRWXU);
    // len=pwrite(fd,buf,file_size,0);
    // close(fd);
    // clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
    // double time_diff = calc_diff(stopwatch_start, stopwatch_stop)*1.0/(long)(1000000000);
    // printf("len=%lld time=%fs\n",len,time_diff);
    // return 0;
}
//4K 860 1120