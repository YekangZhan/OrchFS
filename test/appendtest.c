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
// #include "../src/radix.h"

//操作模式
enum test_mode{
	XXFS=0,
    CTFS,
    EXT4,
    NOVA,
    F2FS
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
    int64_t fstype;
    int64_t thd_op_blks;
    int64_t apthread_time;
    int64_t apbytes;
    int64_t apblk;
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

uint64_t run_test(test_frame_t * frame){
    struct timespec stopwatch_start;
	struct timespec stopwatch_stop;
    frame->apthread_time = frame->apbytes = frame->apblk = 0;
    //追加写缓冲区
    char* appendbuf = valloc(frame->op_size*3);
    uint64_t opsize = frame->op_size;
    int fd = 0;
    if(frame->fstype==XXFS||frame->fstype==CTFS)
    {
        // fprintf(stderr,"%s %d\n", frame->filename, frame->fstype);
        fd = open(frame->filename ,O_RDWR | O_CREAT , S_IRWXU );
    }
    else if(frame->fstype==F2FS||frame->fstype==EXT4)
    {
        fprintf(stderr,"%s %d\n", frame->filename, frame->fstype);
        // fd = open(frame->filename ,O_RDWR | O_SYNC | O_CREAT, S_IRWXU);
        fd = open(frame->filename ,O_RDWR | O_CREAT , S_IRWXU);
    }
    else if(frame->fstype==NOVA)
    {
        fprintf(stderr,"%s %d\n", frame->filename, frame->fstype);
        fd = open(frame->filename ,O_RDWR | O_CREAT , S_IRWXU );
    }
    int64_t add_size = 0;
    // fprintf(stderr,"%lld %d\n", frame->thd_op_blks,fd);
    for(long long i = 0; i < frame->thd_op_blks; i++)
    {
        int64_t blk_id = __sync_fetch_and_add(&now_blk_id, 1);
        clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
        write(fd, appendbuf, rw_size_eachtime[frame->tid][i]);
        fsync(fd);
        add_size += rw_size_eachtime[frame->tid][i];
        // pwrite(fd, appendbuf, frame->op_size, add_size);
        // add_size += frame->op_size;
        clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
        long time0 = calc_diff(stopwatch_start, stopwatch_stop);
        frame->apbytes += opsize; latancy_per_rw[blk_id] = time0; frame->apthread_time += time0; frame->apblk++;
    }
    close(fd);
    //释放空间，结束线程
    free(appendbuf);
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
    char* all_work_type[10] = {"APPEND"};
    if(strcmp(work_type_str,all_work_type[0])==0)
        return 1;
    else
        return 0;
}

int file_sys_process(char* file_sys_type)
{
    int fs_kinds = 4;
    char* all_fs_kinds[10] = {"xxfs","comfs","ext4dax","novarelaxed","f2fs"};
    for(int i = 0; i <= fs_kinds; i++)
    {
        if(strcmp(file_sys_type,all_fs_kinds[i])==0)
            return i;
    }
    return -1;
}

int64_t get_thd_op_size(int work_threads, char* fs_name, int blk_size)
{
    int64_t ret_size = 0, muti = 50;
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

void get_rw_size_eachtime(int64_t avg_blk_size, int64_t RW_BLKS,int threads)
{
    int64_t size4k = 4*1024, size16k = 16*1024, size64k = 64*1024, size128k = 128*1024;
    int64_t size1k = 1024, size8k = 8*1024, size32k = 32*1024, size256k = 256*1024;
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
                rw_size_eachtime[i][j] = randsize;
                rw_size_eachtime[i][j+1] = size16k*2-randsize;
            }
            else if(avg_blk_size==size64k)
            {
                int64_t randsize = myrand()%(size64k)+size32k;
                rw_size_eachtime[i][j] = randsize;
                rw_size_eachtime[i][j+1] = size64k*2-randsize;
            }
            else if(avg_blk_size==size256k)
            {
                int64_t randsize = myrand()%(size256k)+size1k;
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
    // fprintf(stderr,"par:%s %s %s %s %s\n",argv[1],argv[2],argv[3],argv[4],argv[5]);
    srand(time(NULL));
    char* fs_name = argv[1];                         //文件系统的名称
    int fs_id = file_sys_process(fs_name);           //文件系统的代号
    int avg_blk_size = atoi(argv[2])*1024;               //块大小均值
    int work_threads = atoi(argv[3]);                //工作的线程数
    int work_type = work_type_process(argv[4]);      //工作负载类型
    if(work_type!=1)
    {
        printf("operation type error!\n");
        exit(0);
    }
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
    int64_t THD_OP_SIZE = 0;
    THD_OP_SIZE = get_thd_op_size(work_threads, fs_name, avg_blk_size);     //每一个线程需要操作的数量
    int64_t THD_OP_BLKS = THD_OP_SIZE/avg_blk_size;              //每一个线程操作的大小
    // fprintf(stderr,"info:%d %llu %d %d %d %s\n",fs_id, THD_OP_BLKS, avg_blk_size, work_threads, work_type, file_path[0]);
    get_rw_size_eachtime(avg_blk_size, THD_OP_BLKS, work_threads);
    //创建工作线程的相关信息
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    test_frame_t* work_frame = malloc(work_threads * sizeof(test_frame_t));
    pthread_t* work_thd_meta = malloc(work_threads * sizeof(pthread_t));
    for(int j = 0; j < work_threads; j++){
        work_frame[j] = (test_frame_t){
            .filename = file_path[j+1],
            .op_size = avg_blk_size,
            .tid = j,
            .thd_op_blks = THD_OP_BLKS,
            .fstype = fs_id};
        int pret = pthread_create(&work_thd_meta[j], &attr, run_test, &work_frame[j]);
    }

    ///释放线程并关闭文件
    double total_aptime = 1,total_apbyte = 1,total_apblk = 1;
    for(int j = 0; j < work_threads; j++){
        uint64_t ret;
        pthread_join(work_thd_meta[j], (void**)&ret);
        total_aptime += work_frame[j].apthread_time;
        total_apbyte += work_frame[j].apbytes;
        total_apblk += work_frame[j].apblk;
    }
    ///输出信息
    qsort(latancy_per_rw, total_apblk, sizeof(latancy_per_rw[0]), cmp);
    int apindex = total_apblk/100;
    double apbandwidth = (total_apbyte/total_aptime)*1000.0*work_threads, aplatancy = (total_aptime/total_apblk)/1000.0, tail_aplatancy = latancy_per_rw[apindex]/1000.0;
    printf("%s\t%d\t%dk\t%s\t%.3f\t%.3f\t%.3f\n",fs_name, work_threads, avg_blk_size/1024, argv[4], apbandwidth, aplatancy, tail_aplatancy);
	return 0;
}

