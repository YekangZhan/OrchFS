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
// #include "../src/comfs.h"
// #include "../src/comfs_runtime.h"
// #include "../src/comfs_config.h"
// #include "../src/device.h"
// #include "../LibFS/orchfs.h"


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
    ret += (a<<15)+(b<<30);
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
int64_t now_blk_id;     ///当前获取的块编号
int latancy_per_rw[25001000],latancy_per_r[15001000],latancy_per_w[15001000];
int rw_size_eachtime[18][2000000];

long calc_diff(struct timespec start, struct timespec end){
	return (end.tv_sec * (long)(1000000000) + end.tv_nsec) -
	(start.tv_sec * (long)(1000000000) + start.tv_nsec);
}

int cmp ( const void *a , const void *b )
{
    return *(int *)b - *(int *)a;
}

void init_pos_and_type(int64_t* startpos, int64_t* op_type_arr, int op_times, int op_mode, int op_size)
{
    for(long long i = 0; i < op_times; i++)
    {
        startpos[i] = myrand()%(op_times*1LL*op_size-op_size*6);
    }
    if(op_mode==RANDRW11||op_mode==SEQRW11)
    {
        for(long long i = 0; i < op_times; i++)
        {
            if(rand()%2==0) op_type_arr[i] = ROP;
            else if(rand()%2==1) op_type_arr[i] = WOP;
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
    char* writebuf = valloc(frame->op_size*6);
    char* readbuf = valloc(frame->op_size*6);
    uint64_t opsize = frame->op_size;
    int64_t *startpos=malloc(1ll*sizeof(uint64_t)*ARR_SIZE);
    int64_t *op_type_arr=malloc(1ll*sizeof(uint64_t)*ARR_SIZE);
    init_pos_and_type(startpos, op_type_arr, frame->thd_op_blks, frame->mode, opsize);
    //开始计时
    // int fd = open(frame->filename ,O_RDWR | O_CREAT | __O_DIRECT, S_IRWXU );
    int fd = open(frame->filename ,O_RDWR | O_SYNC);
    fprintf(stderr,"open end!: %d %s\n",fd,frame->filename);
    // int fddev = open("/dev/nvme3n1",  O_RDWR | __O_DIRECT ); //
    int64_t add_size = 0, now_arr_cur = rand()%frame->thd_op_blks;
    // clock_gettime(CLOCK_MONOTONIC, &stopwatch_start); 
    if(frame->mode==SEQREAD)
    {
        for(long long i = 0; i < frame->thd_op_blks; i++)
        {
            int64_t blk_id = __sync_fetch_and_add(&now_blk_id, 1);
            // printf("%lld %d %lld %lld\n",frame->tid,i,blk_id,rw_size_eachtime[frame->tid][i]);
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
            // pread(fddev, readbuf, opsize, opsize*(blk_id+off));
            pread(fd, readbuf, rw_size_eachtime[frame->tid][i], add_size);
            add_size += rw_size_eachtime[frame->tid][i];
            // now_arr_cur = (now_arr_cur+1)%frame->thd_op_blks;
            // pread(fd, readbuf, opsize, add_size);
            // add_size += opsize;
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
            long time0 = calc_diff(stopwatch_start, stopwatch_stop);
            frame->rbytes += rw_size_eachtime[frame->tid][i]; 
            frame->rblk++; latancy_per_rw[blk_id] = time0; frame->rthread_time += time0; 
        }
    }
    else if(frame->mode==SEQWRITE)
    {
        for(long long i = 0; i < frame->thd_op_blks; i++)
        {
            int64_t blk_id = __sync_fetch_and_add(&now_blk_id, 1);
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
            // if(i%4096==0)
            //     printf("%lld %d %lld %lld\n",frame->tid,i,add_size,rw_size_eachtime[frame->tid][i]);
            // pwrite(fd, writebuf, opsize, add_size);
            pwrite(fd, writebuf, rw_size_eachtime[frame->tid][i], add_size);
            // add_size += opsize;
            add_size += rw_size_eachtime[frame->tid][i];
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
            long time0 = calc_diff(stopwatch_start, stopwatch_stop);
            frame->wbytes += rw_size_eachtime[frame->tid][i]; 
            frame->wblk++; latancy_per_rw[blk_id] = time0; frame->wthread_time += time0; 
        }
    }
    else if(frame->mode==RANDREAD)
    {
        for(long long i = 0; i < frame->thd_op_blks; i++)
        {
            int64_t blk_id = __sync_fetch_and_add(&now_blk_id, 1);
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
            pread(fd, readbuf, rw_size_eachtime[frame->tid][i], startpos[i]);
            // pread(fd, readbuf, opsize, startpos[i]);
            // ssd_read(readbuf, opsize, ct_rt.ssd_16KBspace+startpos[i]);
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
            long time0 = calc_diff(stopwatch_start, stopwatch_stop);
            frame->rbytes += rw_size_eachtime[frame->tid][i]; 
            frame->rblk++; latancy_per_rw[i] = time0; frame->rthread_time += time0; 
        }
    }
    else if(frame->mode==RANDWRITE)
    {
        for(long long i = 0; i < frame->thd_op_blks; i++)
        {
            int64_t blk_id = __sync_fetch_and_add(&now_blk_id, 1);
            // if(i%4096==0)
            // fprintf(stderr, "%lld %d %lld %lld\n",frame->tid,i,blk_id,rw_size_eachtime[frame->tid][i]);
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
            // pwrite(fd, writebuf, opsize, startpos[i]);
            pwrite(fd, writebuf, rw_size_eachtime[frame->tid][i], startpos[i]);
            clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
            long time0 = calc_diff(stopwatch_start, stopwatch_stop);
            frame->wbytes += rw_size_eachtime[frame->tid][i]; 
            frame->wblk++; latancy_per_rw[i] = time0; frame->wthread_time += time0; 
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
                // pwrite(fd, writebuf, opsize, startpos[i]);
                pwrite(fd, writebuf, rw_size_eachtime[frame->tid][i], startpos[i]);
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
                long time0 = calc_diff(stopwatch_start, stopwatch_stop);
                frame->wbytes += rw_size_eachtime[frame->tid][i]; 
                frame->wblk++; latancy_per_w[i] = time0; frame->wthread_time += time0; 
                latancy_per_rw[blk_id] = time0;
            }
            else if(op_type_arr[i]==ROP)
            {
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
                // pread(fd, readbuf, opsize, startpos[i]);
                pread(fd, readbuf, rw_size_eachtime[frame->tid][i], startpos[i]);
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
                long time0 = calc_diff(stopwatch_start, stopwatch_stop);
                frame->rbytes += rw_size_eachtime[frame->tid][i]; 
                frame->rblk++; latancy_per_r[i] = time0; frame->rthread_time += time0; 
                latancy_per_rw[blk_id] = time0;
            }
        }
    }
    else if(frame->mode==SEQRW11||frame->mode==SEQRW12||frame->mode==SEQRW21)
    {
        for(long long i = 0; i < frame->thd_op_blks; i++)
        {
            int64_t blk_id = __sync_fetch_and_add(&now_blk_id, 1);
            // printf("blkid:%lld %llu\n",blk_id,rw_size_eachtime[frame->tid][i]);
            if(op_type_arr[i]==WOP)
            {
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
                // pwrite(fd, writebuf, opsize, add_size);
                // add_size += opsize;
                pwrite(fd, writebuf, rw_size_eachtime[frame->tid][i], add_size);
                add_size += rw_size_eachtime[frame->tid][i];
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
                long time0 = calc_diff(stopwatch_start, stopwatch_stop);
                frame->wbytes += rw_size_eachtime[frame->tid][i]; frame->wblk++;
                latancy_per_w[blk_id] = time0; frame->wthread_time += time0; 
                latancy_per_rw[blk_id] = time0;
            }
            else if(op_type_arr[i]==ROP)
            {
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
                // pread(fd, writebuf, opsize, add_size);
                // add_size += opsize;
                pread(fd, readbuf, rw_size_eachtime[frame->tid][i], add_size);
                add_size += rw_size_eachtime[frame->tid][i];
                clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
                long time0 = calc_diff(stopwatch_start, stopwatch_stop);
                frame->rbytes += rw_size_eachtime[frame->tid][i]; frame->rblk++;
                latancy_per_r[blk_id] = time0; frame->rthread_time += time0; 
                latancy_per_rw[blk_id] = time0;
            }
        }
    }
    // clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
    // long time0 = calc_diff(stopwatch_start, stopwatch_stop);
    // close(fddev);
    // fprintf(stderr, "thdend! begin! %d\n",frame->tid);
    close(fd);
    // 整理测试时间
    frame->rwthread_time = frame->rthread_time+frame->wthread_time;
    frame->rwbytes = frame->rbytes+frame->wbytes;
    frame->rwblk = frame->rblk+frame->wblk;
    //释放空间，结束线程
    free(writebuf);
    free(readbuf);
    free(startpos);
    free(op_type_arr);
    fprintf(stderr, "thdend!\n");
}

int create_file(char* filename,int64_t blk_size,int64_t this_file_blk_sum,int tid0)
{
    char* buf = valloc(6*blk_size);
    long long add0 = 0;
    int fd = open(filename ,O_RDWR | O_CREAT | O_SYNC , S_IRWXU );
    fprintf(stderr,"%lld %d\n",this_file_blk_sum,fd);
    for(long long i = 0; i < this_file_blk_sum; i++)
    {
        // write(fd,buf,blk_size);
        pwrite(fd,buf,rw_size_eachtime[tid0][i],add0);
        // printf("rw_size:%llu\n",rw_size_eachtime[tid0][i]);
        // pwrite(fd,buf,blk_size,add0);
        add0 += rw_size_eachtime[tid0][i];
        // add0 += blk_size;
    }
    free(buf);
    close(fd);
}

// int64_t get_file_size(char* file_size_str)
// {
//     int fs_len = strlen(file_size_str);
//     int64_t unit_size = 1;
//     if(file_size_str[fs_len-1]=='k'||file_size_str[fs_len-1]=='K')
//         unit_size = 1024;
//     else if(file_size_str[fs_len-1]=='m'||file_size_str[fs_len-1]=='M')
//         unit_size = 1024*1024;
//     else if(file_size_str[fs_len-1]=='g'||file_size_str[fs_len-1]=='G')
//         unit_size = 1LL*1024*1024*1024;
//     else
//         return -1;
//     file_size_str[fs_len-1] = 0;
//     int64_t units = atoi(file_size_str);
//     int64_t ret_size = unit_size*units;
//     return ret_size;
// }

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
    int fs_kinds = 9;
    char* all_fs_kinds[10] = {"xxfs","comfs","ext4dax","f2fs","ext4","novarelaxed","spfs_f2fs","spfs_ext4","madfs"};
    for(int i = 0; i < fs_kinds; i++)
    {
        if(strcmp(file_sys_type,all_fs_kinds[i])==0)
            return i;
    }
    return -1;
}

int64_t get_thd_op_size(int work_threads, char* fs_name, int blk_size)
{
    int64_t ret_size = 0, muti = 1; //1==60M
    if(work_threads <= 4)
        ret_size = 1LL*12*FACTOR*muti;
    else if(work_threads > 4&&work_threads <= 12)
        ret_size = 1LL*10*FACTOR*muti;
    else if(work_threads >= 13&&work_threads <= 16)
        ret_size = 1LL*6*FACTOR*muti;
    return ret_size;
}

void get_rw_size_eachtime(int64_t avg_blk_size, int64_t RW_BLKS,int threads)
{
    int64_t size4k = 4*1024, size16k = 16*1024, size64k = 64*1024, size256k = 256*1024;
    int64_t size1k = 1024, size8k = 8*1024, size32k = 32*1024, size128k = 128*1024;
    for(int i = 0; i <= threads; i++)
    {
        for(int64_t j = 0; j <= RW_BLKS+10; j+=2)
        {
            if(avg_blk_size==size4k)
            {
                int64_t randsize = myrand()%(size4k*2-size1k)+size1k;
                rw_size_eachtime[i][j] = randsize;
                rw_size_eachtime[i][j+1] = size4k*2-randsize;
            }
            else if(avg_blk_size==size16k)
            {
                int64_t randsize = myrand()%(size16k)+size8k;
                // int64_t randsize = (size16k);
                rw_size_eachtime[i][j] = randsize;
                rw_size_eachtime[i][j+1] = size16k*2-randsize;
            }
            else if(avg_blk_size==size64k)
            {
                // int64_t randsize = myrand()%(size64k)+size32k;
                int64_t randsize = (size64k);
                rw_size_eachtime[i][j] = randsize;
                rw_size_eachtime[i][j+1] = size64k*2-randsize;
            }
            else if(avg_blk_size==size128k)
            {
                int64_t randsize = myrand()%(size256k)+size128k;
                rw_size_eachtime[i][j] = randsize;
                rw_size_eachtime[i][j+1] = size256k*2-randsize;
            }
            else if(avg_blk_size==size256k)
            {
                int64_t randsize = myrand()%(size256k)+size1k;
                // int64_t randsize = (size256k);
                rw_size_eachtime[i][j] = randsize;
                rw_size_eachtime[i][j+1] = size256k*2-randsize;
            }
            else
            {
                printf("avg block size error!\n");
            }
        }
        for(int64_t j = 0; j <= RW_BLKS/2; j++)
        {
            int64_t pos0 = myrand()%RW_BLKS, pos1 = myrand()%RW_BLKS;
            int trans = rw_size_eachtime[i][pos0];
            rw_size_eachtime[i][pos0] = rw_size_eachtime[i][pos1];
            rw_size_eachtime[i][pos1] = trans;
        }
    }
}

///需要传入块粒度，单位为k
///需要传入线程数
///需要传入工作种类
///SEQREAD, SEQWRITE, RANDREAD, RANDWRITE
///RANDRWab，a:b代表读写比，例如RANDRW12表示读写比1:2
///RANDRW11，RANDRW12，RANDRW21，RANDRW13，RANDRW31
///argv第一个参数是工作种类，第二个参数是块粒度，第三个参数是线程数
int main(int argc, const char* argv[]){

    ///传入的参数处理
    // init_libfs();
    // srand(time(NULL));
    fprintf(stderr ,"par:%s %s %s %s %s\n",argv[1],argv[2],argv[3],argv[4],argv[5]);
    char* fs_name = argv[1];                         //文件系统的名称
    int fs_id = file_sys_process(fs_name);           //文件系统的代号
    int avg_blk_size = atoi(argv[2])*1024;               //块大小均值
    // printf("avg_blk_size:%d\n",avg_blk_size);
    int work_threads = atoi(argv[3]);                //工作的线程数
    int work_type = work_type_process(argv[4]);      //工作负载类型
    if(work_type==SEQWRITE||work_type==RANDWRITE)
        srand(time(NULL));
    char file_path[20][300] = {0};
    if(strlen(argv[5]) > 256)
    {
        printf("mount path too long!\n");
        exit(0);
    }
    for(int i = 0; i <= 16; i++)
        strcpy(file_path[i], argv[5]);
    char* filename[20] = {"/test0","/test1","/test2","/test3","/test4","/test5","/test6","/test7","/test8",\
    "/test9","/test10","/test11","/test12","/test13","/test14","/test15","/test16","/test17"};
    for(int i = 0; i <= 16; i++)
        strcat(file_path[i],filename[i]);                       //文件系统挂载点
    if(work_type < 0)
    {
        printf("operation type error!\n");
        exit(0);
    }
    int64_t THD_OP_SIZE = 0;
    if(work_type==SEQWRITE||work_type==SEQREAD)
        THD_OP_SIZE = get_thd_op_size(work_threads, fs_name, avg_blk_size);     //每一个线程需要操作的数量
    else
        THD_OP_SIZE = get_thd_op_size(work_threads, fs_name, avg_blk_size);     //每一个线程需要操作的数量
    int64_t THD_OP_BLKS = 0;
    THD_OP_BLKS = THD_OP_SIZE/(avg_blk_size);              //每一个线程操作的大小
    fprintf(stderr,"info:%s %llu %d %d %d %s\n",fs_name, THD_OP_BLKS, avg_blk_size, work_threads, work_type, file_path[0]);
    get_rw_size_eachtime(avg_blk_size, THD_OP_BLKS, work_threads);
    //创建或打开文件
    if(argv[6][0]=='1')
    {
        struct timespec app_watch_start;
	    struct timespec app_watch_end;
        clock_gettime(CLOCK_MONOTONIC, &app_watch_start);
        for(int i = 0; i < work_threads; i++)
        {
            // fprintf(stderr,"%d %s\n",i,file_path[i]);
            create_file(file_path[i], avg_blk_size, THD_OP_BLKS, i);
        }
        fprintf(stderr,"create end!\n");
        clock_gettime(CLOCK_MONOTONIC, &app_watch_end);
        long time0 = calc_diff(app_watch_start, app_watch_end);
        fprintf(stderr,"apptime: %ld\n",time0);
        void* space = malloc(4096);
        int stat_ret = stat("/xx/test0",space);
        printf("stat_ret: %d\n",stat_ret);
        return 0;
    }
    //创建工作线程的相关信息
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    test_frame_t* work_frame = malloc(work_threads * sizeof(test_frame_t));
    pthread_t* work_thd_meta = malloc(work_threads * sizeof(pthread_t));
    for(int j = 0; j < work_threads; j++){
        work_frame[j] = (test_frame_t){
            .filename = file_path[j],
            .op_size = avg_blk_size,
            .tid = j,
            .thd_op_blks = THD_OP_BLKS,
            .mode = work_type};
        int pret = pthread_create(&work_thd_meta[j], &attr, run_test, &work_frame[j]);
    }
    ///释放线程并关闭文件
    double total_rwtime = 1,total_rtime = 1,total_wtime = 1;
    double total_rwbyte = 1,total_rbyte = 1,total_wbyte = 1;
    double total_rwblk = 1,total_rblk = 1,total_wblk = 1;
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
    fprintf(stderr,"end!\n");
    ///输出信息
    qsort(latancy_per_rw, total_rwblk, sizeof(latancy_per_rw[0]), cmp);
    int rwindex = total_rwblk/100, rindex = total_rblk/100, windex = total_wblk/100;
    double rwbandwidth = (total_rwbyte/total_rwtime)*1000.0*work_threads, rwlatancy = (total_rwtime/total_rwblk)/1000.0, tail_rwlatancy = latancy_per_rw[rwindex]/1000.0;
    // printf("%s\t%d\t%dk\t%s\t%.3f\t%.3f\t%.3f\n",fs_name, work_threads, avg_blk_size/1024, argv[4], rwbandwidth, rwlatancy, tail_rwlatancy);
    if(work_type==RANDRW11||work_type==RANDRW12||work_type==RANDRW21||work_type==SEQRW11||work_type==SEQRW12||work_type==SEQRW21)
    {
        qsort(latancy_per_r, total_rblk, sizeof(latancy_per_r[0]), cmp);
        double rbandwidth = (total_rbyte/total_rtime)*1000.0*work_threads, rlatancy = (total_rtime/total_rblk)/1000.0, tail_rlatancy = latancy_per_r[rindex]/1000.0;
        qsort(latancy_per_w, total_wblk, sizeof(latancy_per_w[0]), cmp);
        double wbandwidth = (total_wbyte/total_wtime)*1000.0*work_threads, wlatancy = (total_wtime/total_wblk)/1000.0, tail_wlatancy = latancy_per_w[windex]/1000.0;
        printf("%s\t%d\t%dk\t%s\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\n",fs_name, work_threads, avg_blk_size/1024, argv[4], 
                rwbandwidth, rwlatancy, tail_rwlatancy, rbandwidth, rlatancy, tail_rlatancy, wbandwidth, wlatancy, tail_wlatancy);
    }
    else
    {
        // printf("%.3f\t%.3f\n",total_rwtime,total_rwbyte);

        printf("%s\t%d\t%dk\t%s\t%.3f\t%.3f\t%.3f\n",fs_name, work_threads, avg_blk_size/1024, argv[4], rwbandwidth, rwlatancy, tail_rwlatancy);
    }
    fprintf(stderr,"test end!\n");
	return 0;
}

