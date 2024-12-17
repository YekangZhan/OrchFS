#ifndef LIB_FUNC_H
#define LIB_FUNC_H

#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/statfs.h>
#include <sys/vfs.h>
#include <inttypes.h>
#include <linux/magic.h>
#include <linux/falloc.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <x86intrin.h>
#include <stdarg.h> 
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <inttypes.h>


void init_libfs();

void close_libfs();

void read_from_file(int fd, int64_t file_start_byte, int64_t read_len, void* read_buf);

void write_into_file(int fd, int64_t file_start_byte, int64_t write_len, void* write_buf);

int orchfs_open (const char *pathname, int oflag, ...);

int orchfs_openat (int dirfd, const char *pathname, int mode, ...);

int orchfs_close(int fd);

int64_t orchfs_pwrite(int fd, const void *buf, int64_t write_len, int64_t offset);

int64_t orchfs_pread(int fd, void *buf, int64_t read_len, int64_t byte_offset);

int64_t orchfs_write(int fd, const void *buf, size_t write_len);

int64_t orchfs_read(int fd, void *buf, int64_t read_len);

int orchfs_mkdir(const char *pathname, uint16_t mode);

int orchfs_rmdir(const char *pathname);

int orchfs_unlink (const char *pathname);

int orchfs_fstatfs(int fd, struct statfs *buf);

int orchfs_lstat (const char *pathname, struct stat *buf);

int orchfs_stat(const char *pathname, struct stat *buf);

int orchfs_fstat(int fd, struct stat *buf);

int orchfs_lseek(int fd, int offset, int whence);

struct dirent * orchfs_readdir(DIR *dirp);

DIR* orchfs_opendir(const char *_pathname);

int orchfs_closedir(DIR *dirp);

int orchfs_rename(const char *oldpath, const char *newpath);

int orchfs_fcntl(int fd, int cmd, ...);


#endif