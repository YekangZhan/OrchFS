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

// long calc_diff(struct timespec start, struct timespec end){
// 	return (end.tv_sec * (long)(1000000000) + end.tv_nsec) -
// 	(start.tv_sec * (long)(1000000000) + start.tv_nsec);
// }

void sync_test(char* filename)
{
    int fd = open(filename ,O_RDWR | O_CREAT, S_IRWXU );
    int allsize = 16*1024*1024;
    char* write_buf = malloc(allsize+512);
    for(int i = 0; i < allsize; i++)
        *(write_buf+i) = (char)(i%128);
    int wblksize[105] = {0},max_opsize = 256*1024;
    for(int i = 0; i <= 100; i++)
        wblksize[i] = ((3452*i)%max_opsize*635)%max_opsize;
    int add = 0;
    for(int i = 1; i <= 100; i++)
    {
        pwrite(fd,write_buf+add,wblksize[i],add);
        add += wblksize[i];
    }
    free(write_buf);
    close(fd);
}

void test(char* filename)
{
    int fd = open(filename ,O_RDWR | O_CREAT, S_IRWXU );
    int allsize = 16*1024*1024;
    char* read_buf = malloc(allsize+512);
    int wblksize[105] = {0},max_opsize = 256*1024;
    for(int i = 0; i <= 100; i++)
        wblksize[i] = ((3452*i)%max_opsize*635)%max_opsize;
    int add0 = 0;
    for(int i = 1; i <= 100; i++)
    {
        pread(fd,read_buf+add0,wblksize[i],add0);
        add0 += wblksize[i];
    }
    for(int i = 0; i < allsize; i++)
        assert((int)read_buf[i]==i%128);
    free(read_buf);
    close(fd);
}

// int main(int argc, const char* argv[])
// {
//     srand(time(NULL));
//     char filename[200] = "/xx/test";
//     if(argv[1]=='0')
//         sync_test(filename);
//     else if(argv[1]=='1')
//         test(filename);
//     printf("end!\n");
// }

uint64_t myrand()
{
    uint64_t a = rand(),b = rand(),ret = rand();
    ret += (a<<15)+(b<<30);
    return ret;
}

void strata_test()
{
    //用于盲写测试
    //首先创建一个文件和对应的读写缓冲区，里面的内容按照某种规则初始化
    char filename[200] = "/xx/test1";
    int fd = open(filename ,O_RDWR | O_CREAT, S_IRWXU );
    int allsize = 128*1024*1024;
    int size1m = 1024*1024;
    char* write_buf = malloc(allsize+512);
    char* read_buf = malloc(allsize+512);
    for(int i = 0; i < allsize; i++)
    {
        write_buf[i] = (char)(i/size1m);
        // read_buf[i] = (char)(i%125)+2;
    }
    //向里面写入若干SSD的块
    int blksize = 16*1024,wtime = 1024,add = 0;
    for(int i = 0; i < wtime; i++)
    {
        int muti = rand()%8+1;
        // printf("%d %llu %llu\n",i,add,blksize*muti);
        write(fd,write_buf+add,blksize*muti);
        add += blksize*muti;
    }
    printf("create end!\n");
    //先块内写，然后读出来检测
    int statime = 10000;
    for(int i = 0; i < statime; i++)
    {
        uint64_t startpos = myrand()%(add/blksize-3)*blksize;
        uint64_t thiswsize = myrand()%(12*1024)+1,offset0 = myrand()%(3*1024+512);
        // printf("%d %llu %llu\n",i,startpos,thiswsize);
        pwrite(fd, write_buf+startpos+offset0, thiswsize, startpos+offset0);
    }
    // printf("add:%llu\n",add);
    pread(fd, read_buf, add, 0);
    for(int i = 0; i < add; i++)
    {
        if(write_buf[i]!=read_buf[i])
        {
            for(int j = i-5; j <= i+10; j++)
                printf("%d %d %d\n",j,write_buf[j],read_buf[j]);
            assert(write_buf[i]==read_buf[i]);
        }
    }
    printf("check 1 end!\n");
    // 再加上块间写，然后读出来检测
    for(int i = 0; i < statime; i++)
    {
        uint64_t startpos = myrand()%(add-16*blksize);
        uint64_t thiswsize = myrand()%(12*1024)+1;
        pwrite(fd, write_buf+startpos, thiswsize, startpos);
    }
    pread(fd, read_buf, add, 0);
    for(int i = 0; i < add; i++)
        assert(write_buf[i]==read_buf[i]);
    printf("check 2 end!\n");
    //关闭文件，清理缓存
    close(fd);
    free(write_buf);
    free(read_buf);
}

int main()
{
   strata_test();
}
