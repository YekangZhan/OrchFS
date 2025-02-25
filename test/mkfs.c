#include "../src/comfs.h"
int main(int argc, char ** argv){
    // fflush(stdout);
    int flag = 0;
    if(argc > 1){
        if(atoi(argv[1])){
            flag |= COMFS_MKFS_FLAG_RESET_DAX;
            printf("Set to reset dax\n");
        }
    }
    //printf("pre\n");
    int ret = comfs_mkfs(flag);
    //printf("next res=%d\n",ret);
    if(ret){
        printf("Failed to format!!\n");
        return -1;
    }
    // printf("ctFS formated!\n");
    return 0;
    // int fd = open("/home/robin/d", O_RDWR);
    // printf("open return %d\n", fd);
    // int fd2 = open("/home/robin/d", O_RDWR);
    // printf("open return %d\n", fd2);
}
