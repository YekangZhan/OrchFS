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
	const char* fs_name = argv[1];                           //文件系统的名称
    const char* runid = argv[2];                             //文件系统的名称
    const char* work_type = argv[3];                         //负载类型
	const char* path = argv[4];                              //文件系统的名称

	const char* cmd = "sudo ./run_fs.sh";
	char new_cmd[1024] = {0};

	get_new_cmd(new_cmd, cmd, argv, 4);

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
		printf("i am the parent process, my process id is %d\n",getpid()); 
		const char* fcmd1 = "sudo ../../build/mkfs";
		system(fcmd1);
        sleep(1);
		const char* fcmd = "sudo ../../build/kfs_main";
		system(fcmd);
	}
	return 0;
}