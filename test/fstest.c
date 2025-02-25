#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
//#include "../src/device.h"

#define TEST_BLOCKSIZE 2*1024*1024
#define DEFAULT_PATH "/mnt/pmem_emul/testfile"
#define DEFAULT_SIZE (2*1024*1024*(unsigned long)1024) //(128*1024*(unsigned long)1024) //
#define _GB (1*1024*1024*(unsigned long)1024)
void sequential_rdwr(const char * path, unsigned long);
void small_chunck_rdwr(const char * path, unsigned long);
void sim_log_write(const char * path, unsigned long size, unsigned long num);
long calc_diff(struct timespec start, struct timespec end);
// #define comfs
#ifdef comfs
#include "../comfs.h"
#define OPEN        comfs_open
#define PWRITE      comfs_pwrite
#define PWRITEA		comfs_pwrite_atomic
#define PREAD       comfs_pread
#define CLOSE		close
#define INIT        comfs_init(0)
#else
#include <unistd.h>
#define OPEN 		open 
#define PWRITE 		pwrite64
#define PWRITEA		pwrite64
#define PREAD 		pread64
#define CLOSE		close 
#define FOPEN		fopen 
#define FCLOSE		fclose 
#define FWRITE		fwrite 
#define FREAD		fread 
#define FFLUSH		fflush 
#define INIT    	
#define print_debug(a) 
#endif


int _rd = 0;
int _wr = 0;
int _ap = 0;
int _rad = 0;
int _fs = 0;
int _stream = 0;
int _op=0;
int _flag=0;
size_t _BLOCKSIZE=0;
unsigned long total_writen = 0;
unsigned long total_read = 0;
unsigned long total_nsec = 0;
unsigned long total_nsec_writen = 0;
unsigned long total_nsec_read = 0;

long calc_diff(struct timespec start, struct timespec end){
	return (end.tv_sec * (long)(1000000000) + end.tv_nsec) -
	(start.tv_sec * (long)(1000000000) + start.tv_nsec);
}

int main(int argc, char * argv[]){
	INIT;
	int opt;
	opterr = 0;
	
	unsigned long size = DEFAULT_SIZE;
	unsigned num = 1;
	char * dest = DEFAULT_PATH;
	char path_buf[100];

	while((opt = getopt(argc, argv, "wrRfaSs:n:G:M:K:p:o:F")) != -1){
		switch(opt){
			case 'w':{
				_wr = 1;
				break;
			}
			case 'r':{
				_rd = 1;
				break;
			}
			case 'R':{
				_rad = 1;
				break;
			}
			case 'f':{
				_flag = 1;
				break;
			}
			case 'a':{
				_ap = 1;
				break;
			}
			case 'S':{
				_fs = 1;
				break;
			}
			case 's':{
				size = atoi(optarg);
				break;
			}
			case 'n':{
				num = atoi(optarg);
				break;
			}
			case 'G':{
				size = atoi(optarg) * 1024 * 1024 * (unsigned long)1024;
				break;
			}
			case 'M':{
				size = atoi(optarg) * 1024 * (unsigned long)1024;
				break;
			}
			case 'K':{
				size = atoi(optarg) * (unsigned long)1024;
				break;
			}
			case 'p':{
				dest = optarg;
				break;
			}
			case 'o':{
				_op = optarg[0]-'0';
				if(_op==0) 
					_BLOCKSIZE=256;
				else if(_op==1)
					_BLOCKSIZE=16*1024;
				else _BLOCKSIZE=2*1024*1024;
				break;
			}
			case 'F':{
				_stream = 1;
				break;
			}
			default: {
				fprintf(stderr, "Usage: %s: \n\t[-w write] \n\t[-r read] \n\t[-R random] \n\t[-a append] \n\t[-S fsync] \n\t[-s size] \n\t[-n number of files] \n\t[-G size in GB] \n\t[-M size in MB] \n\t[-K size in KB] \n\t[-p path] \n\t[-F use file stream]\n",
				argv[0]);
				exit(0);
			}
		}
	}
	if(_ap){
		printf("********%s\n",dest);
		sprintf(path_buf, "%s_0", dest);
		if(size==DEFAULT_SIZE){
			size = 256;
			num = 8388608;
		}
		else{
			num = size / (256);
			size = 256;
		}
		sim_log_write(path_buf, size, num);
	}
	else if(_rad){
		for (int i=0; i<num; i++){
			sprintf(path_buf, "%s_%d", dest, i);
			small_chunck_rdwr(path_buf, size);
		}
	}
	else{
		for (int i=0; i<num; i++){
			sprintf(path_buf, "%s_%d", dest, i);
			sequential_rdwr(path_buf, size);
		}
	}
	printf("==================END OF TESTS=====================\n");
	printf("time used: %ld ns\nTotal Written: %ld\nTotal Read: %ld\nWrite speed: %f GB/s\nRead speed: %f GB/s\n", 
	total_nsec, 
	total_writen,
	total_read,
	(double)total_writen / (double)total_nsec_writen, 
	(double)total_read / (double)total_nsec_read);
	//printf("numread=%d numwrite=%d\n",numread,numwrite);
	long write_block_num = total_writen/_BLOCKSIZE,read_block_num = total_read/_BLOCKSIZE;
	printf("writeIOPS(k)=%f\n",(double)write_block_num / ((double)total_nsec_writen/1e6));
	printf("readIOPS(k)=%f\n",(double)read_block_num / ((double)total_nsec_read/1e6));
	//printf("\tNomalized 2GB time(W/R): %ld ns / %ld ns\n", (unsigned long)((double)total_nsec / (double)total_writen * (double)DEFAULT_SIZE), (unsigned long)((double)total_nsec / (double)total_read * (double)DEFAULT_SIZE));
}

void sequential_rdwr(const char * path, unsigned long size){
	struct timespec stopwatch_start;
	struct timespec stopwatch_stop;
	int fd_wr;
	FILE * f = NULL;
	long time_diff;
	long res, retry = 0;
	// const char * path = "/home/siwei/research/test/testfile";

	printf("Generating %ld bytes of buffer data...\n", size);
	clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
	if(_stream){
		f = FOPEN(path, "w+");
	}
	else{
		if(_flag)
			fd_wr = OPEN(path, O_RDWR, S_IRWXU);
		else fd_wr = OPEN(path, O_RDWR | O_CREAT, S_IRWXU);
	}
	printf("fd=%d\n",fd_wr);
	clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
	time_diff = calc_diff(stopwatch_start, stopwatch_stop);
	printf("Open Took %ld ns\n", time_diff);
	int* buffer; 
	if(size > 1 * _GB){
		buffer = valloc(1 * _GB);
		for(long i = 0; i < 1 * _GB / sizeof(int); i++){
			buffer[i] = rand();
		}
	}
	else{
		buffer = valloc(size);
		for(long i = 0; i < size / sizeof(int); i++){
			buffer[i] = rand();
		}
	}
	unsigned long written = 0;
	
	// sleep(1);

	if(_wr){
		while (1)
		{
			unsigned long needed;
			if(size - written > 1 * _GB){
				needed = 1 * _GB;
			}
			else{
				needed = size - written;
			}
			res = 0;
			retry = 0;
			printf("Writing %ldB to %s start...\n", needed, path);
			// comfs_pwrite(fd_wr, (void*)buffer, buf_size, 0);
			print_debug(fd_wr);
			clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
			if(_stream){
				res = FWRITE(buffer, 1, needed, f);
			}
			else{
				// res = PWRITEA(fd_wr, (void*)buffer, (needed << 2)+_op, written);
				res = PWRITEA(fd_wr, (void*)buffer, needed, written);
				// printf("ret: %ld\n",res);
			}
			while(res < needed && retry < 10){
				if(_stream){
					res += FWRITE(buffer, 1, needed - res, f);
				}
				else{
					res += PWRITEA(fd_wr, (void*)buffer, ((needed - res) << 2)+_op, written + res);
				}
				retry ++;
			}
			if(res < needed){
				printf("Writing failed. Returned :%lu\n", res);
				exit(1);
			}
			if(_fs){
				if(_stream){
					FFLUSH(f);
				}
				else{
					fsync(fd_wr);
				}
			}
			clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
			retry = 0;
			printf("Writing buffer to disk end: %ld\n", res);
			time_diff = calc_diff(stopwatch_start, stopwatch_stop);
			printf("time used: %ld ns\nspeed: %f GB/s\nWritten so far: %ldB\n", time_diff, (double)res / (double)time_diff, written);
			// printf("\n******************************************\n\n");
			// printf("\tNomalized 2GB time: %ld ns\n", (unsigned long)((double)time_diff / (double)res * (double)DEFAULT_SIZE));
			print_debug(fd_wr);
			total_writen += res;
			total_nsec_writen += time_diff;
			written += needed;
			if (size - written ==0){
				break;
			}
		}
		
		
	}
	written = 0;
	if(_rd){
		while(1){
			unsigned long needed;
			if(size - written > 1 * _GB){
				needed = 1 * _GB;
			}
			else{
				needed = size - written;
			}
			res = 0;
			retry = 0;
			printf("\nReading content from %s start...\n",path);
			//res = comfs_pread(fd_rd, buffer_rd, buf_size, 0);
			clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
			while(res < needed && retry < 10){
				//printf("read:%lu %lu\n",(needed - res),written + res);
				res += PREAD(fd_wr, (void*)buffer, ((needed - res) << 2)+_op, written + res);
				retry ++;
			}
			if(res < needed){
				printf("Reading failed. Returned :%lu\n", res);
				exit(1);
			}
			if(_fs){
				fsync(fd_wr);
			}
			// fsync(fd_rd);
			clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
			printf("Reading content from disk end: %lu\n",res);
			time_diff = calc_diff(stopwatch_start, stopwatch_stop);
			printf("time used: %ld ns\nspeed: %f GB/s\nRead so far: %ldB\n", time_diff, (double)res / (double)time_diff, written);
			// printf("\n******************************************\n\n");
			// printf("\tNomalized 2GB time: %ld ns\n", (unsigned long)((double)time_diff / (double)res * (double)DEFAULT_SIZE));
			total_read += res;
			total_nsec_read += time_diff;
			written += needed;
			if (size - written ==0){
				break;
			}
		}
		
	}
	if(_stream){
		FCLOSE(f);
	}
	else{
		CLOSE(fd_wr);
	}
	//free(buffer);
	print_debug(fd_wr);
	// close(fd_rd);
	// free(buffer_rd);
	// print_time();
}

void small_chunck_rdwr(const char * path, unsigned long size){
	struct timespec stopwatch_start;
	struct timespec stopwatch_stop;
	
	// const char * path = "/home/siwei/research/test/testfile";

	printf("Generating buffer data...\n");

	int* buffer = malloc(_GB);
	unsigned long res = 0;
	long time_diff;
	unsigned long pos;
	for(long i = 0; i < 1 * _GB / sizeof(int); i++){
		buffer[i] = rand();
	}
	uint64_t total = 0;
	uint64_t * ind = malloc(size/_BLOCKSIZE*sizeof(uint64_t));
	for(uint64_t i = 0; i < size/_BLOCKSIZE; i++){
		ind[i] = (rand() % (_GB - _BLOCKSIZE - 1)) & ~((uint64_t)_BLOCKSIZE-1);
	}
	uint64_t * ind2 = malloc(size/_BLOCKSIZE*sizeof(uint64_t));
	for(uint64_t i = 0; i < size/_BLOCKSIZE; i++){
		ind2[i] = (rand() % (size - _BLOCKSIZE - 1)) & ~((uint64_t)_BLOCKSIZE-1);
	}

	
	int fd_wr;
	if(_flag)
		fd_wr=OPEN(path, O_RDWR, S_IRWXU);
	else fd_wr=OPEN(path, O_RDWR | O_CREAT, S_IRWXU);
	printf("fd=%d _flag=%d\n",fd_wr,_flag);
	if(_wr){
		res = 0;
		printf("Writing buffer to %s start...\n", path);
		// res = PWRITE(fd_wr, ((void*)buffer), buf_size, 0 );
		print_debug(fd_wr);
		
		clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
		int num=0;
		for(uint64_t i = 0; i < size/_BLOCKSIZE; i++){
			res += PWRITEA(fd_wr, ((void*)buffer + ind[i]), (_BLOCKSIZE<<2)+_op,ind2[i]);//ind2[i]
			//num+=256;
		}
		if(_fs){
			fsync(fd_wr);
		}
		clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
		time_diff = calc_diff(stopwatch_start, stopwatch_stop);
		printf("Writing buffer to disk end: %ld\n", res);
		printf("time used: %ld ns\nspeed: %f GB/s\n", time_diff, (double)res / (double)time_diff);
		//printf("\tNomalized 2GB time: %ld ns\n", (unsigned long)((double)time_diff / (double)res * (double)DEFAULT_SIZE));
		print_debug(fd_wr);
		total_writen += res;
		total_nsec_writen += time_diff;
	}
	//sleep(10);
	total = 0;
	//freopen("./readoffset","w",stdout);
	if(_rd){
		res = 0;
		printf("\nReading content from %s start...\n", path);	
		int num=0;
		clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
		for(uint64_t i = 0; i < size/_BLOCKSIZE; i++){ //size/_BLOCKSIZE
			res += PREAD(fd_wr, ((void*)buffer + ind[i]), (_BLOCKSIZE<<2)+_op, ind2[i]);//ind2[i]
		}
		if(_fs){
			fsync(fd_wr);
		}
		clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
		time_diff = calc_diff(stopwatch_start, stopwatch_stop);
		printf("Reading content from disk end: %ld\n", res);
		printf("time used: %ld ns\nspeed: %f GB/s\n", time_diff, (double)res / (double)time_diff);
		//printf("\tNomalized 2GB time: %ld ns\n", (unsigned long)((double)time_diff / (double)res * (double)DEFAULT_SIZE));
		print_debug(fd_wr);
		total_read += res;
		total_nsec_read += time_diff;
	}
	
	//free(buffer);
	//free(ind);
	CLOSE(fd_wr);
	//print_time();
	//printf("alloc256=%d\n",alloc256);
}

void sim_log_write(const char * path, unsigned long size, unsigned long num){
	struct timespec stopwatch_start;
	struct timespec stopwatch_stop;
	unsigned long res = 0;
	long time_diff;
	char* buffer_rd;
	int* buffer;
	unsigned long num_ = num;
	printf("Preparing for appending %luB ...\n", size * num);
	if(size*num > 1 * _GB){
		buffer_rd = (char*)(malloc(1 * _GB));
		if(buffer_rd == NULL){
			printf("malloc failed!\n");
			exit(1);
		}
		buffer = (int*)buffer_rd;
		for(long i = 0; i < 1 * _GB / sizeof(int); i++){
			buffer[i] = rand();
		}
	}
	else{
		buffer_rd = (char*)(malloc(size*num));
		if(buffer_rd == NULL){
			printf("malloc failed!\n");
			exit(1);
		}
		buffer = (int*)buffer_rd;
		for(long i = 0; i < size*num / sizeof(int); i++){
			buffer[i] = rand();
		}
	}
	
	
	printf("Writing log-like input to %s start...\n", path);
	int fd_rd = OPEN(path, O_RDWR | O_CREAT, S_IRWXU);
	printf("fd_rd=%d\n",fd_rd);
	
	
	unsigned long needed = num > (1*_GB)/size ? (1*_GB)/size : num;
	//printf("needed=%lu\n",needed);
	unsigned long add=0;
	clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
	while(1){
		for(long i = 0; i < needed; i++){
			//printf("i=%ld\n",i);
			//sleep(5);
			res += PWRITE(fd_rd, buffer_rd + add , (size<<2)+_op, res);
			add+=size;
			// res += PWRITE(fd_rd, buffer_rd  , size, i * size);
			if(_fs){
				fsync(fd_rd);
			}
		}
		num -= needed;
		if(num == 0)
			break;
	}
	// print_debug(fd_rd);
	// 
	clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
	printf("Writing content to disk end: %ld\n", res);
	free(buffer_rd);

	time_diff = calc_diff(stopwatch_start, stopwatch_stop);
	printf("Write %lu files of %lu B.\ntime used: %ld ns\nspeed: %f GB/s\n\n", num_, size, time_diff, res / (double)time_diff);
	printf("\tNomalized 2GB time: %ld ns\n", (unsigned long)((double)time_diff / (double)res * (double)DEFAULT_SIZE));
	total_writen += res;
	total_nsec += time_diff;
	CLOSE(fd_rd);
}

// long calc_diff(struct timespec start, struct timespec end){
// 	return (end.tv_sec * (long)(1000000000) + end.tv_nsec) -
// 	(start.tv_sec * (long)(1000000000) + start.tv_nsec);
// }