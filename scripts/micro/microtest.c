#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

void get_new_cmd(char new_cmd[], char* cmd, const char* argv[], int argv_num)
{
	int cmdlen = strlen(cmd);
	// for(int i = 0; i < 1024; i++)
	// {
	// 	if(cmd[i] != 0 && cmd[i] != '\n')
	// 		cmdlen++;
	// 	else
	// 		break;
	// }
	// printf("cmdlen: %d\n",cmdlen);
	int cur = 0;
	for(int i = 0; i < cmdlen; i++)
		new_cmd[i] = cmd[i];
	cur = cmdlen;
	for(int i = 1; i <= argv_num; i++)
	{
		int len = strlen(argv[i]);
		fprintf(stderr,"argv[%d]: %s\n",i,argv[i]);
		new_cmd[cur++] = ' ';
		for(int j = 0; j < len; j++)
		{
			new_cmd[cur] = argv[i][j];
			cur += 1;
		}
	}
	new_cmd[cur++] = 0;
	assert(strlen(new_cmd)==cur-1);
}

int main(int argc, const char* argv[]) 
{ 
	const char* block_size = argv[1];                        //块大小
	const char* fs_name = argv[2];                           //文件系统的名称
	const char* runid = argv[3];                             //文件系统的名称
	const char* path = argv[4];                             //文件系统的名称
	const char* work_type = argv[5];                         //负载类型
	const char* threads = argv[6];                           //线程数

	// const char* cmd = "sudo taskset -c 0-27,56-83 ./run_fs2.sh";
	const char* cmd = "sudo sh run_fs2.sh";
	char new_cmd[1024] = {0};

	get_new_cmd(new_cmd, cmd, argv, 6);

	printf("cmd: %s\n",new_cmd);

	pid_t fpid; //fpid表示fork函数返回的值
	fpid=fork(); 
	if (fpid < 0) 
		printf("error in fork!"); 
	else if (fpid == 0) 
    {
		// 儿子进程，lib
		sleep(8);
        printf("i am the child process, my process id is %d\n",getpid()); 
		system(new_cmd);
		sleep(1);
		printf("will close kernelFS\n");
		const char* close_cmd = "sudo ../../build/close_kfs";
		system(close_cmd);
	}
	else 
    {
		// 父亲进程，kernel
		fprintf(stderr,"i am the parent process, my process id is %d\n",getpid()); 
		const char* fcmd1 = "sudo ../../build/mkfs";
		system(fcmd1);
        sleep(1);
		const char* fcmd = "sudo ../../build/kfs_main";
		system(fcmd);
	}
	return 0;
}