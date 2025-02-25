#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <sys/resource.h>
#include "../src/comfs.h"
#include "../src/comfs_runtime.h"
#include "../src/comfs_config.h"
#include "../src/device.h"
#include "../src/radix.h"

//操作模式
enum test_mode{
	SEQREAD=0,
    SEQWRITE,
    RANDREAD,
    RANDWRITE,
    RANDRW11,
    RANDRW12,
    RANDRW21,
    SEQRW11,
    SEQRW12,
    SEQRW21
};

//每一个线程需要写入的大小
#define FACTOR  (2*2*2*2*3*3*5*7*1024)       //整除大约5M
#define ARR_SIZE (8LL*1024*1024)

//读写代号
#define ROP 0
#define WOP 1

uint64_t myrand()
{
    uint64_t a = rand(),b = rand(),c = rand(),ret = rand();
    ret += (a<<15)+(b<<30)+(c<<45);
    return ret;
}

struct test_frame{
	char* filename;
    int64_t op_size;
    int64_t tid; 
    int64_t mode;
    int64_t thd_op_blks;
    int64_t rwthread_time;
    int64_t rwbytes;
    int64_t rwblk;
    int64_t rthread_time;
    int64_t rbytes;
    int64_t rblk;
    int64_t wthread_time;
    int64_t wbytes;
    int64_t wblk;
};
typedef struct test_frame test_frame_t;
int64_t blk_sum;      ///总共要写的块数
int64_t now_blk_id;     ///当前获取的块编号
int latancy_per_rw[25001000],latancy_per_r[15001000],latancy_per_w[15001000];

// long calc_diff(struct timespec start, struct timespec end){
// 	return (end.tv_sec * (long)(1000000000) + end.tv_nsec) -
// 	(start.tv_sec * (long)(1000000000) + start.tv_nsec);
// }

int cmp ( const void *a , const void *b )
{
    return *(int *)b - *(int *)a;
}

void init_pos_and_type(int64_t* startpos, int64_t* op_type_arr, int op_times, int op_mode, int op_size)
{
    for(long long i = 0; i < op_times; i++)
        startpos[i] = myrand()%(blk_sum*op_size-op_size);
    if(op_mode==RANDRW11||op_mode==SEQRW11)
    {
        for(long long i = 0; i < op_times; i++)
        {
            if(i%2==0) op_type_arr[i] = ROP;
            else if(i%2==1) op_type_arr[i] = WOP;
        }
    }
    else if(op_mode==RANDRW12||op_mode==SEQRW12)
    {
        for(long long i = 0; i < op_times; i++)
        {
            if(i%3==0) op_type_arr[i] = ROP;
            else if(i%3==1) op_type_arr[i] = WOP;
            else if(i%3==2) op_type_arr[i] = WOP;
        }
    }
    else if(op_mode==RANDRW21||op_mode==SEQRW21)
    {
        for(long long i = 0; i < op_times; i++)
        {
            if(i%3==0) op_type_arr[i] = ROP;
            else if(i%3==1) op_type_arr[i] = ROP;
            else if(i%3==2) op_type_arr[i] = WOP;
        }
    }
}

uint64_t run_test(test_frame_t * frame){
    struct timespec stopwatch_start;
	struct timespec stopwatch_stop;
    frame->rwthread_time = frame->rthread_time = frame->wthread_time = 0;
    frame->rwbytes = frame->rbytes = frame->wbytes = 0;
    frame->rblk = frame->wblk = frame->rwblk = 0;
    //读写缓冲区
    char* writebuf = valloc(frame->op_size);
    char* readbuf = valloc(frame->op_size);
    uint64_t opsize = frame->op_size;
    int64_t *startpos=malloc(1ll*sizeof(uint64_t)*ARR_SIZE);
    int64_t *op_type_arr=malloc(1ll*sizeof(uint64_t)*ARR_SIZE);
    init_pos_and_type(startpos, op_type_arr, frame->thd_op_blks, frame->mode, opsize);
    //开始计时
    int fd = open(frame->filename ,O_RDWR  | __O_DIRECT );//| O_SYNC __O_DIRECT
    // int fddev = open("/dev/md1",  O_RDWR | __O_DIRECT ); //
    // printf("optimes:%lld\n",frame->thd_op_blks);
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
    if(frame->mode==SEQREAD)
    {
        for(long long i = 0; i < frame->thd_op_blks; i++)
        {
            int64_t blk_id = __sync_fetch_and_add(&now_blk_id, 1);
            //clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
            pread(fd, readbuf, opsize, opsize*blk_id);
            //fsync(fd);
            //clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
            //long time0 = calc_diff(stopwatch_start, stopwatch_stop);
            // long time0 = 1;
            //latancy_per_rw[blk_id] = time0;frame->rthread_time += time0;
            frame->rbytes += opsize;  frame->rblk++;
        }
    }
    else if(frame->mode==SEQWRITE)
    {
        for(long long i = 0; i < frame->thd_op_blks; i++)
        {
            int64_t blk_id = __sync_fetch_and_add(&now_blk_id, 1);
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
            pwrite(fd, writebuf, opsize, opsize*blk_id);
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
            long time0 = calc_diff(stopwatch_start, stopwatch_stop);
            frame->wbytes += opsize; latancy_per_rw[blk_id] = time0; frame->wthread_time += time0; frame->wblk++;
        }
    }
    else if(frame->mode==RANDREAD)
    {
        for(long long i = 0; i < frame->thd_op_blks; i++)
        {
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
            pread(fd, readbuf, opsize, startpos[i]);
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
            long time0 = calc_diff(stopwatch_start, stopwatch_stop);
            frame->rbytes += opsize; latancy_per_rw[i] = time0; frame->rthread_time += time0; frame->rblk++;
        }
    }
    else if(frame->mode==RANDWRITE)
    {
        for(long long i = 0; i < frame->thd_op_blks; i++)
        {
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
            pwrite(fd, writebuf, opsize, startpos[i]);
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
            long time0 = calc_diff(stopwatch_start, stopwatch_stop);
            frame->wbytes += opsize; latancy_per_rw[i] = time0; frame->wthread_time += time0; frame->wblk++;
        }
    }
    else if(frame->mode==RANDRW11||frame->mode==RANDRW12||frame->mode==RANDRW21)
    {
        for(long long i = 0; i < frame->thd_op_blks; i++)
        {
            int64_t blk_id = __sync_fetch_and_add(&now_blk_id, 1);
            if(op_type_arr[i]==WOP)
            {
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
                pwrite(fd, writebuf, opsize, startpos[i]);
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
                long time0 = calc_diff(stopwatch_start, stopwatch_stop);
                frame->wbytes += opsize; latancy_per_w[i] = time0; frame->wthread_time += time0; 
                frame->wblk++; latancy_per_rw[blk_id] = time0;
            }
            else if(op_type_arr[i]==ROP)
            {
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
                pread(fd, readbuf, opsize, startpos[i]);
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
                long time0 = calc_diff(stopwatch_start, stopwatch_stop);
                frame->rbytes += opsize; latancy_per_r[i] = time0; frame->rthread_time += time0; 
                frame->rblk++; latancy_per_rw[blk_id] = time0;
            }
        }
    }
    else if(frame->mode==SEQRW11||frame->mode==SEQRW12||frame->mode==SEQRW21)
    {
        for(long long i = 0; i < frame->thd_op_blks; i++)
        {
            int64_t blk_id = __sync_fetch_and_add(&now_blk_id, 1);
            if(op_type_arr[i]==WOP)
            {
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
                pwrite(fd, writebuf, opsize, opsize*blk_id);
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
                long time0 = calc_diff(stopwatch_start, stopwatch_stop);
                frame->wbytes += opsize; latancy_per_w[blk_id] = time0; frame->wthread_time += time0; 
                frame->wblk++; latancy_per_rw[blk_id] = time0;

            }
            else if(op_type_arr[i]==ROP)
            {
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
                pread(fd, readbuf, opsize, opsize*blk_id);
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
                long time0 = calc_diff(stopwatch_start, stopwatch_stop);
                frame->rbytes += opsize; latancy_per_r[blk_id] = time0; frame->rthread_time += time0; 
                frame->rblk++; latancy_per_rw[blk_id] = time0;
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
    long time0 = calc_diff(stopwatch_start, stopwatch_stop);
    close(fd);
    // 整理测试时间
    frame->rthread_time += time0;
    frame->rwthread_time = frame->rthread_time+frame->wthread_time;
    //frame->rwthread_time = time0;
    frame->rwbytes = frame->rbytes+frame->wbytes;
    frame->rwblk = frame->rblk+frame->wblk;
    //释放空间，结束线程
    free(writebuf);
    free(readbuf);
    free(startpos);
    free(op_type_arr);
}

int create_file(char* filename,int64_t blk_size)
{
    char* buf = valloc(2*blk_size);
    int fd = open(filename ,O_RDWR | O_CREAT | O_SYNC | O_APPEND, S_IRWXU );
    for(long long i = 0; i < blk_sum; i++)
    {
        if(i%4096==0)
            printf("%lld\n",i);
        //pwrite(fd,buf,blk_size,i*blk_size);
        write(fd,buf,blk_size);
    }
    close(fd);
}

int64_t get_file_size(char* file_size_str)
{
    int fs_len = strlen(file_size_str);
    int64_t unit_size = 1;
    if(file_size_str[fs_len-1]=="k"||file_size_str[fs_len-1]=="K")
        unit_size = 1024;
    else if(file_size_str[fs_len-1]=="m"||file_size_str[fs_len-1]=="M")
        unit_size = 1024*1024;
    else if(file_size_str[fs_len-1]=="g"||file_size_str[fs_len-1]=="G")
        unit_size = 1LL*1024*1024*1024;
    else
        return -1;
    file_size_str[fs_len-1] = 0;
    int64_t units = atoi(file_size_str);
    int64_t ret_size = unit_size*units;
    return ret_size;
}

int work_type_process(char* work_type_str)
{
    int type_kinds = 10;
    char* all_work_type[10] = {"SEQREAD","SEQWRITE","RANDREAD","RANDWRITE",
                                "RANDRW11","RANDRW12","RANDRW21","SEQRW11","SEQRW12","SEQRW21"};
    for(int i = 0; i < type_kinds; i++)
    {
        if(strcmp(work_type_str,all_work_type[i])==0)
            return i;
    }
    return -1;
}

int file_sys_process(char* file_sys_type)
{
    int fs_kinds = 4;
    char* all_fs_kinds[10] = {"xxfs","comfs","ext4dax","f2fs","novarelaxed"};
    for(int i = 0; i < fs_kinds; i++)
    {
        if(strcmp(file_sys_type,all_fs_kinds[i])==0)
            return i;
    }
    return -1;
}

int64_t get_thd_op_size(int work_threads, char* fs_name)
{
    int64_t ret_size = 0,muti = 90;
    if(work_threads <= 4)
        ret_size = 1LL*12*FACTOR*muti;
    else if(work_threads >= 5&&work_threads <= 8)
        ret_size = 1LL*10*FACTOR*muti;
    else if(work_threads >= 9&&work_threads <= 12)
        ret_size = 1LL*8*FACTOR*muti;
    else if(work_threads >= 13&&work_threads <= 16)
        ret_size = 1LL*6*FACTOR*muti;
    return ret_size;
}

///需要传入块粒度，单位为k
///需要传入线程数
///需要传入工作种类
///SEQREAD, SEQWRITE, RANDREAD, RANDWRITE
///RANDRWab，a:b代表读写比，例如RANDRW12表示读写比1:2
///RANDRW11，RANDRW12，RANDRW21，RANDRW13，RANDRW31
///argv第一个参数是工作种类，第二个参数是块粒度，第三个参数是线程数
int main(int argc, const char* argv[]){
    int blocksize=256*1024;
    char* buf=(char*)valloc(blocksize);
    // size_t* ind=malloc(sizeof(size_t)*1000*500);
    for(int i=0;i<blocksize;i++)
        buf[i]=i%256;
    // for(size_t i=0;i<500000;i++)
    //     ind[i]=rand()%(4*1024)+i*4*1024;
        //ind[i]=i*(4*1024*2);
        //ind[i]=i*(4*1024);
    struct timespec stopwatch_start;
	struct timespec stopwatch_stop;
    double time_diff=0;
    int num=20000;
    uint64_t offset=0;
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
    for(uint64_t i = 0; i < num; i++)
    {
        memcpy(CT_REL2ABS(offset), buf, blocksize);//
        cache_wb(CT_REL2ABS(offset),blocksize);
        offset+=blocksize;
        //fsync(fd);
    }
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
    time_diff+=calc_diff(stopwatch_start, stopwatch_stop)*1.0/(long)(1000000000);
    printf("%fMB/s %fs\n",1.0*blocksize/1000*num/1000/time_diff,time_diff);
    printf("lantency=%.1fus\n",time_diff/num*1000000);
    /*
    ///传入的参数处理
    printf("par:%s %s %s %s %s\n",argv[1],argv[2],argv[3],argv[4],argv[5]);
    srand(time(NULL));
    char* fs_name = argv[1];                         //文件系统的名称
    int fs_id = file_sys_process(fs_name);           //文件系统的代号
    int blk_size = atoi(argv[2])*1024;               //块大小
    int work_threads = atoi(argv[3]);                //工作的线程数
    int work_type = work_type_process(argv[4]);      //工作负载类型
    char file_path[300] = {0};
    if(strlen(argv[5]) > 256)
    {
        printf("mount path too long!\n");
        exit(0);
    }
    strcpy(file_path, argv[5]);
    char* filename = "/test1";
    strcat(file_path,filename);                       //文件系统挂载点
    if(work_type < 0)
    {
        printf("operation type error!\n");
        exit(0);
    }
    int64_t THD_OP_SIZE = 0;
    if(work_type==SEQWRITE||work_type==SEQREAD)
        THD_OP_SIZE = get_thd_op_size(work_threads, fs_name)/2;     //每一个线程需要操作的数量
    else
        THD_OP_SIZE = get_thd_op_size(work_threads, fs_name);     //每一个线程需要操作的数量
    int64_t THD_OP_BLKS = THD_OP_SIZE/blk_size;              //每一个线程操作的大小
    blk_sum = THD_OP_BLKS*work_threads;
    printf("info:%s %llu %d %d %d %s\n",fs_name, blk_sum, blk_size, work_threads, work_type, file_path);
    // char* filename_list[5] = {"/xx/test","\\/test","/mnt/ext4test/test","/mnt/f2fstest/test","/mnt/novatest/test"};
    //创建或打开文件
    //create_file(file_path, blk_size);
    //printf("create end!\n");
    // getchar();
    //创建工作线程的相关信息
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    test_frame_t* work_frame = malloc(work_threads * sizeof(test_frame_t));
    pthread_t* work_thd_meta = malloc(work_threads * sizeof(pthread_t));
    for(int j = 0; j < work_threads; j++){
        work_frame[j] = (test_frame_t){
            .filename = file_path,
            .op_size = blk_size,
            .tid = j,
            .thd_op_blks = THD_OP_BLKS,
            .mode = work_type};
        int pret = pthread_create(&work_thd_meta[j], &attr, run_test, &work_frame[j]);
    }

    ///释放线程并关闭文件
    double total_rwtime = 0,total_rtime = 0,total_wtime = 0;
    double total_rwbyte = 0,total_rbyte = 0,total_wbyte = 0;
    double total_rwblk = 0,total_rblk = 0,total_wblk = 0;
    for(int j = 0; j < work_threads; j++){
        uint64_t ret;
        pthread_join(work_thd_meta[j], (void**)&ret);
        total_rwtime += work_frame[j].rwthread_time;
        total_rtime += work_frame[j].rthread_time;
        total_wtime += work_frame[j].wthread_time;
        total_rwbyte += work_frame[j].rwbytes;
        total_rbyte += work_frame[j].rbytes;
        total_wbyte += work_frame[j].wbytes;
        total_rwblk += work_frame[j].rwblk;
        total_rblk += work_frame[j].rblk;
        total_wblk += work_frame[j].wblk;
    }
    ///输出信息
    qsort(latancy_per_rw, total_rwblk, sizeof(latancy_per_rw[0]), cmp);
    int rwindex = total_rwblk/100, rindex = total_rblk/100, windex = total_wblk/100;
    double rwbandwidth = (total_rwbyte/total_rwtime)*1000.0*work_threads, rwlatancy = (total_rwtime/total_rwblk)/1000.0, tail_rwlatancy = latancy_per_rw[rwindex]/1000.0;
    if(work_type==RANDRW11||work_type==RANDRW12||work_type==RANDRW21||work_type==SEQRW11||work_type==SEQRW12||work_type==SEQRW21)
    {
        qsort(latancy_per_r, total_rblk, sizeof(latancy_per_r[0]), cmp);
        double rbandwidth = (total_rbyte/total_rtime)*1000.0*work_threads, rlatancy = (total_rtime/total_rblk)/1000.0, tail_rlatancy = latancy_per_r[rindex]/1000.0;
        qsort(latancy_per_w, total_wblk, sizeof(latancy_per_w[0]), cmp);
        double wbandwidth = (total_wbyte/total_wtime)*1000.0*work_threads, wlatancy = (total_wtime/total_wblk)/1000.0, tail_wlatancy = latancy_per_w[windex]/1000.0;
        printf("%s\t%d\t%dk\t%s\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\n",fs_name, work_threads, blk_size/1024, argv[4], 
                rwbandwidth, rwlatancy, tail_rwlatancy, rbandwidth, rlatancy, tail_rlatancy, wbandwidth, wlatancy, tail_wlatancy);
    }
    else
    {
        printf("%s\t%d\t%dk\t%s\t%.3f\t%.3f\t%.3f\n",fs_name, work_threads, blk_size/1024, argv[4], rwbandwidth, rwlatancy, tail_rwlatancy);
    }
	return 0;
    */
}

