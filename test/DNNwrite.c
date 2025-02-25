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

double time_diff[6]={0,0,0,0,0,0};
int main()
{
    struct timespec stopwatch_start;
	struct timespec stopwatch_stop;
    //FILE* fp = fopen("/mnt/EXT4TEST/resnet50.0.0", "r");   //打开文件
    //FILE* fp = fopen("/mnt/EXT4TEST/resnet152.0.0", "r"); 
    //FILE* fp = fopen("/mnt/EXT4TEST/resnext101.0.0", "r"); 
    FILE* fp = fopen("/mnt/EXT4TEST/vgg16.0.0", "r"); 
    fseek(fp, 0, SEEK_END);      //将文件指针指向该文件的最后
    long long file_size = ftell(fp); 
    char* buf=(char*)valloc(file_size);
    fseek(fp, 0, SEEK_SET); 
    long long len=fread(buf, sizeof(char), file_size, fp); 
    assert(len==file_size);
    int num=86; //156 131 86
    for(int i=1;i<=num;i++)
    {
        for(int j=4;j<=4;j++)
        {
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
            int fd=-1;
            if(j==1)
                fd=open("/mnt/ext4test/file",O_RDWR | O_CREAT, S_IRWXU);
            else if(j==2)
                fd=open("/mnt/f2fstest/file",O_RDWR | O_CREAT, S_IRWXU);
            else if(j==3)
                fd=open("/mnt/novatest/file",O_RDWR | O_CREAT, S_IRWXU);
            else if(j==4)
                fd=open("/xx/file",O_RDWR | O_CREAT, S_IRWXU);
            else if(j==5)
                fd=open("\\/file",O_RDWR | O_CREAT, S_IRWXU);
            assert(fd>=0);
            len=pwrite(fd,buf,file_size,0);
            assert(len==file_size);
            fsync(fd);
            close(fd);
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
            time_diff[j]+=calc_diff(stopwatch_start, stopwatch_stop)*1.0/(long)(1000000000);
        }
    }
    //printf("ext4dax:%f f2fs:%f nova:%f\n",time_diff[1]/num,time_diff[2]/num,time_diff[3]/num);
    //printf("f2fs:%f\n",time_diff[2]/num);
    printf("xxfs:%f\n",time_diff[4]/num);
    return 0;
}
//4K 860 1120