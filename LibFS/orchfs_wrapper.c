#define _GNU_SOURCE
#include <dlfcn.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>

#include "lib_dir.h"
#include "orchfs.h"
#include "runtime.h"
#include <boost/preprocessor/seq/for_each.hpp>
#include "../config/config.h"
#include "ffile.h"
// #define WRAPPER_DEBUG

// #ifdef __cplusplus       
// extern "C"{
// #endif

#ifdef WRAPPER_DEBUG
#define PRINT_FUNC	printf("\tcalled ctFS func: %s\n", __func__)
#else
#define PRINT_FUNC	;
#endif

#define ORCH_FD_OFFSET 1048576
// #define ORCH_FD_OFFSET 10000000

// int64_t all_read_size;

# define EMPTY(...)
# define DEFER(...) __VA_ARGS__ EMPTY()
# define OBSTRUCT(...) __VA_ARGS__ DEFER(EMPTY)()
# define EXPAND(...) __VA_ARGS_

#define MK_STR(arg) #arg
#define MK_STR2(x) MK_STR(x)
#define MK_STR3(x) MK_STR2(x)
// Information about the functions which are wrapped by EVERY module
// Alias: the standard function which most users will call
#define ALIAS_OPEN   open 
#define ALIAS_CREAT  creat 
#define ALIAS_EXECVE execve 
#define ALIAS_EXECVP execvp 
#define ALIAS_EXECV execv 
#define ALIAS_MKNOD __xmknod 
#define ALIAS_MKNODAT __xmknodat 

#define ALIAS_FOPEN  	fopen 
#define ALIAS_FOPEN64  	fopen64 
#define ALIAS_FREAD  	fread 
#define ALIAS_FEOF 	 	feof 
#define ALIAS_FERROR 	ferror 
#define ALIAS_CLEARERR 	clearerr 
#define ALIAS_FWRITE 	fwrite 
#define ALIAS_FSEEK  	fseek 
#define ALIAS_FTELL  	ftell 
#define ALIAS_FTELLO 	ftello 
#define ALIAS_FCLOSE 	fclose 
#define ALIAS_FPUTS		fputs 
#define ALIAS_FGETS		fgets 
#define ALIAS_FFLUSH	fflush 

#define ALIAS_FSTATFS	fstatfs 
#define ALIAS_FDATASYNC	fdatasync 
#define ALIAS_FCNTL		fcntl 
#define ALIAS_FCNTL2	__fcntl64_nocancel_adjusted 
#define ALIAS_OPENDIR		opendir 
#define ALIAS_READDIR	readdir 
#define ALIAS_READDIR64	readdir64 
#define ALIAS_CLOSEDIR	closedir 
#define ALIAS_ERROR		__errno_location 
#define ALIAS_SYNC_FILE_RANGE	sync_file_range 

#define ALIAS_ACCESS access 
#define ALIAS_READ   read 
#define ALIAS_READ2		__libc_read 
#define ALIAS_WRITE  write 
#define ALIAS_SEEK   lseek 
#define ALIAS_CLOSE  close 
#define ALIAS_FTRUNC ftruncate 
#define ALIAS_TRUNC  truncate 
#define ALIAS_DUP    dup 
#define ALIAS_DUP2   dup2 
#define ALIAS_FORK   fork 
#define ALIAS_VFORK  vfork 
#define ALIAS_MMAP   mmap 
#define ALIAS_READV  readv 
#define ALIAS_WRITEV writev 
#define ALIAS_PIPE   pipe 
#define ALIAS_SOCKETPAIR   socketpair 
#define ALIAS_IOCTL  ioctl 
#define ALIAS_MUNMAP munmap 
#define ALIAS_MSYNC  msync 
#define ALIAS_CLONE  __clone 
#define ALIAS_PREAD  pread 
#define ALIAS_PREAD64  pread64 
#define ALIAS_PWRITE64 pwrite64 
#define ALIAS_PWRITE pwrite 
//#define ALIAS_PWRITESYNC pwrite64_sync
#define ALIAS_FSYNC  fsync 
#define ALIAS_FDSYNC fdatasync 
#define ALIAS_FTRUNC64 ftruncate64 
#define ALIAS_OPEN64  open64 
#define ALIAS_LIBC_OPEN64 __libc_open64 
#define ALIAS_SEEK64  lseek64 
#define ALIAS_MMAP64  mmap64 
#define ALIAS_MKSTEMP mkstemp 
#define ALIAS_MKSTEMP64 mkstemp64 
#define ALIAS_ACCEPT  accept 
#define ALIAS_SOCKET  socket 
#define ALIAS_UNLINK  unlink 
#define ALIAS_POSIX_FALLOCATE posix_fallocate 
#define ALIAS_POSIX_FALLOCATE64 posix_fallocate64 
#define ALIAS_FALLOCATE fallocate 

#ifdef _STAT_VER
#define ALIAS_XSTAT __xstat
#else 
#define ALIAS_STAT stat 
#endif
#ifdef _STAT_VER
#define ALIAS_FXSTAT __fxstat
#else 
#define ALIAS_STAT fstat 
#endif

#define ALIAS_STAT64 stat64 
#define ALIAS_FSTAT64 fstat64 
#define ALIAS_LSTAT lstat 
#define ALIAS_LSTAT64 lstat64 
/* Now all the metadata operations */
#define ALIAS_MKDIR mkdir 
#define ALIAS_RENAME rename 
#define ALIAS_LINK link 
#define ALIAS_SYMLINK symlink 
#define ALIAS_RMDIR rmdir 
/* All the *at operations */
#define ALIAS_OPENAT openat 
#define ALIAS_SYMLINKAT symlinkat 
#define ALIAS_MKDIRAT mkdirat 
#define ALIAS_UNLINKAT  unlinkat 


// The function return type
#define RETT_OPEN   int
#define RETT_LIBC_OPEN64 int
#define RETT_CREAT  int
#define RETT_EXECVE int
#define RETT_EXECVP int
#define RETT_EXECV int
#define RETT_SHM_COPY void
#define RETT_MKNOD int
#define RETT_MKNODAT int

// #ifdef TRACE_FP_CALLS
#define RETT_FOPEN  FILE*
#define RETT_FOPEN64  FILE*
#define RETT_FWRITE size_t
#define RETT_FSEEK  int
#define RETT_FTELL  long int
#define RETT_FTELLO off_t
#define RETT_FCLOSE int
#define RETT_FPUTS	int
#define RETT_FGETS	char*
#define RETT_FFLUSH	int
// #endif

#define RETT_FSTATFS	int
#define RETT_FDATASYNC	int
#define RETT_FCNTL		int
#define RETT_FCNTL2		int
#define RETT_OPENDIR	DIR *
#define RETT_READDIR	struct dirent *
#define RETT_READDIR64	struct dirent64 *
#define RETT_CLOSEDIR	int
#define RETT_ERROR		int *
#define RETT_SYNC_FILE_RANGE int

#define RETT_ACCESS int
#define RETT_READ   ssize_t
#define RETT_READ2   ssize_t
#define RETT_FREAD  size_t
#define RETT_FEOF   int
#define RETT_FERROR int
#define RETT_CLEARERR void
#define RETT_WRITE  ssize_t
#define RETT_SEEK   off_t
#define RETT_CLOSE  int
#define RETT_FTRUNC int
#define RETT_TRUNC  int
#define RETT_DUP    int
#define RETT_DUP2   int
#define RETT_FORK   pid_t
#define RETT_VFORK  pid_t
#define RETT_MMAP   void*
#define RETT_READV  ssize_t
#define RETT_WRITEV ssize_t
#define RETT_PIPE   int
#define RETT_SOCKETPAIR   int
#define RETT_IOCTL  int
#define RETT_MUNMAP int
#define RETT_MSYNC  int
#define RETT_CLONE  int
#define RETT_PREAD  ssize_t
#define RETT_PREAD64  ssize_t
#define RETT_PWRITE ssize_t
#define RETT_PWRITE64 ssize_t
//#define RETT_PWRITESYNC ssize_t
#define RETT_FSYNC  int
#define RETT_FDSYNC int
#define RETT_FTRUNC64 int
#define RETT_OPEN64  int
#define RETT_SEEK64  off64_t
#define RETT_MMAP64  void*
#define RETT_MKSTEMP int
#define RETT_MKSTEMP64 int
#define RETT_ACCEPT  int
#define RETT_SOCKET  int
#define RETT_UNLINK  int
#define RETT_POSIX_FALLOCATE int
#define RETT_POSIX_FALLOCATE64 int
#define RETT_FALLOCATE int

#ifdef _STAT_VER
#define RETT_XSTAT int
#else 
#define RETT_STAT int
#endif
#ifdef _STAT_VER
#define RETT_FXSTAT int
#else 
#define RETT_FSTAT int
#endif

#define RETT_STAT64 int
#define RETT_FSTAT64 int
#define RETT_LSTAT int
#define RETT_LSTAT64 int
/* Now all the metadata operations */
#define RETT_MKDIR int
#define RETT_RENAME int
#define RETT_LINK int
#define RETT_SYMLINK int
#define RETT_RMDIR int
/* All the *at operations */
#define RETT_OPENAT int
#define RETT_SYMLINKAT int
#define RETT_MKDIRAT int
#define RETT_UNLINKAT int


// The function interface
#define INTF_OPEN const char *path, int oflag, ...
#define INTF_LIBC_OPEN64 const char *path, int oflag, ...

#define INTF_CREAT const char *path, mode_t mode
#define INTF_EXECVE const char *filename, char *const argv[], char *const envp[]
#define INTF_EXECVP const char *file, char *const argv[]
#define INTF_EXECV const char *path, char *const argv[]
#define INTF_SHM_COPY void
#define INTF_MKNOD int ver, const char* path, mode_t mode, dev_t* dev
#define INTF_MKNODAT int ver, int dirfd, const char* path, mode_t mode, dev_t* dev

// #ifdef TRACE_FP_CALLS
#define INTF_FOPEN  const char* __restrict path, const char* __restrict mode
#define INTF_FOPEN64  const char* __restrict path, const char* __restrict mode
#define INTF_FREAD  void* __restrict buf, size_t length, size_t nmemb, FILE* __restrict fp
#define INTF_CLEARERR FILE* fp
#define INTF_FEOF   FILE* fp
#define INTF_FERROR FILE* fp
#define INTF_FWRITE const void* __restrict buf, size_t length, size_t nmemb, FILE* __restrict fp
#define INTF_FSEEK  FILE* fp, long int offset, int whence
#define INTF_FTELL  FILE* fp
#define INTF_FTELLO FILE* fp
#define INTF_FCLOSE FILE* fp
#define INTF_FPUTS	const char *str, FILE *stream
#define INTF_FGETS	char *str, int n, FILE *stream
#define INTF_FFLUSH	FILE* fp
// #endif

#define INTF_FSTATFS	int fd, struct statfs *buf
#define INTF_FDATASYNC	int fd
#define INTF_FCNTL		int fd, int cmd, ...
#define INTF_FCNTL2		int fd, int cmd, void *arg
#define INTF_OPENDIR	const char *path
#define INTF_READDIR	DIR *dirp
#define INTF_READDIR64	DIR *dirp
#define INTF_CLOSEDIR	DIR *dirp
#define INTF_ERROR		void
#define INTF_SYNC_FILE_RANGE int fd, off64_t offset, off64_t nbytes, unsigned int flags


#define INTF_ACCESS const char *pathname, int mode
#define INTF_READ   int file, void* buf, size_t length
#define INTF_READ2   int file, void* buf, size_t length
#define INTF_WRITE  int file, const void* buf, size_t length
#define INTF_SEEK   int file, off_t offset, int whence
#define INTF_CLOSE  int file
#define INTF_FTRUNC int file, off_t length
#define INTF_TRUNC  const char* path, off_t length
#define INTF_DUP    int file
#define INTF_DUP2   int file, int fd2
#define INTF_FORK   void
#define INTF_VFORK  void
#define INTF_MMAP   void *addr, size_t len, int prot, int flags, int file, off_t off
#define INTF_READV  int file, const struct iovec *iov, int iovcnt
#define INTF_WRITEV int file, const struct iovec *iov, int iovcnt
#define INTF_PIPE   int file[2]
#define INTF_SOCKETPAIR   int domain, int type, int protocol, int sv[2]
#define INTF_IOCTL  int file, unsigned long int request, ...
#define INTF_MUNMAP void *addr, size_t len
#define INTF_MSYNC  void *addr, size_t len, int flags
#define INTF_CLONE  int (*fn)(void *a), void *child_stack, int flags, void *arg
#define INTF_PREAD  int file,       void *buf, size_t count, off_t offset
#define INTF_PREAD64  int file,       void *buf, size_t count, off_t offset
#define INTF_PWRITE int file, const void *buf, size_t count, off_t offset
#define INTF_PWRITE64 int file, const void *buf, size_t count, off_t offset
//#define INTF_PWRITESYNC int file, const void *buf, size_t count, off_t offset
#define INTF_FSYNC  int file
#define INTF_FDSYNC int file
#define INTF_FTRUNC64 int file, off64_t length
#define INTF_OPEN64  const char* path, int oflag, ...
#define INTF_SEEK64  int file, off64_t offset, int whence
#define INTF_MMAP64  void *addr, size_t len, int prot, int flags, int file, off64_t off
#define INTF_MKSTEMP char* file
#define INTF_MKSTEMP64 char* file
#define INTF_ACCEPT  int file, struct sockaddr *addr, socklen_t *addrlen
#define INTF_SOCKET  int domain, int type, int protocol
#define INTF_UNLINK  const char* path
#define INTF_POSIX_FALLOCATE int file, off_t offset, off_t len
#define INTF_POSIX_FALLOCATE64 int file, off_t offset, off_t len
#define INTF_FALLOCATE int file, int mode, off_t offset, off_t len

#ifdef _STAT_VER
#define INTF_XSTAT int ver,const char *path, struct stat *buf
#else 
#define INTF_STAT const char *path, struct stat *buf
#endif
#ifdef _STAT_VER
#define INTF_FXSTAT int ver, int file, struct stat *buf
#else 
#define INTF_FSTAT int file, struct stat *buf
#endif


#define INTF_STAT64 const char *path, struct stat64 *buf
#define INTF_FSTAT64 int file, struct stat64 *buf
#define INTF_LSTAT const char *path, struct stat *buf
#define INTF_LSTAT64 const char *path, struct stat64 *buf
/* Now all the metadata operations */
#define INTF_MKDIR const char *path, uint32_t mode
#define INTF_RENAME const char *old, const char *new
#define INTF_LINK const char *path1, const char *path2
#define INTF_SYMLINK const char *path1, const char *path2
#define INTF_RMDIR const char *path
/* All the *at operations */
#define INTF_OPENAT int dirfd, const char* path, int oflag, ...
#define INTF_UNLINKAT  int dirfd, const char* path, int flags
#define INTF_SYMLINKAT const char* old_path, int newdirfd, const char* new_path
#define INTF_MKDIRAT int dirfd, const char* path, mode_t mode





// The function interface
#define M_INTF_OPEN const char *path, int oflag, ...
#define M_INTF_LIBC_OPEN64 const char *path, int oflag, ...

#define M_INTF_CREAT const char *path, mode_t mode
#define M_INTF_EXECVE const char *filename, char *const argv[], char *const envp[]
#define M_INTF_EXECVP const char *file, char *const argv[]
#define M_INTF_EXECV const char *path, char *const argv[]
#define M_INTF_SHM_COPY void
#define M_INTF_MKNOD int ver, const char* path, mode_t mode, dev_t* dev
#define M_INTF_MKNODAT int ver, int dirfd, const char* path, mode_t mode, dev_t* dev

// #ifdef TRACE_FP_CALLS
#define M_INTF_FOPEN  const char* __restrict path, const char* __restrict mode
#define M_INTF_FOPEN64  const char* __restrict path, const char* __restrict mode
#define M_INTF_FREAD  void* __restrict buf, size_t length, size_t nmemb, FILE* __restrict fp, int op
#define M_INTF_CLEARERR FILE* fp
#define M_INTF_FEOF   FILE* fp
#define M_INTF_FERROR FILE* fp
#define M_INTF_FWRITE const void* __restrict buf, size_t length, size_t nmemb, FILE* __restrict fp, int op
#define M_INTF_FSEEK  FILE* fp, long int offset, int whence, int op
#define M_INTF_FTELL  FILE* fp
#define M_INTF_FTELLO FILE* fp
#define M_INTF_FCLOSE FILE* fp
#define M_INTF_FPUTS	const char *str, FILE *stream, int op
#define M_INTF_FGETS	char *str, int n, FILE *stream, int op
#define M_INTF_FFLUSH	FILE* fp
// #endif

#define M_INTF_FSTATFS	int fd, struct statfs *buf
#define M_INTF_FDATASYNC	int fd
#define M_INTF_FCNTL		int fd, int cmd, ...
#define M_INTF_FCNTL2		int fd, int cmd, void *arg
#define M_INTF_OPENDIR	const char *path
#define M_INTF_READDIR	DIR *dirp
#define M_INTF_READDIR64	DIR *dirp
#define M_INTF_CLOSEDIR	DIR *dirp
#define M_INTF_ERROR		void
#define M_INTF_SYNC_FILE_RANGE int fd, off64_t offset, off64_t nbytes, unsigned int flags

#define M_INTF_ACCESS const char *pathname, int mode
#define M_INTF_READ   int file, void* buf, size_t length, int op
#define M_INTF_READ2   int file, void* buf, size_t length, int op
#define M_INTF_WRITE  int file, const void* buf, size_t length,int op
#define M_INTF_SEEK   int file, size_t offset, int whence, int op
#define M_INTF_CLOSE  int file
#define M_INTF_FTRUNC int file, size_t length, int op
#define M_INTF_TRUNC  const char* path, size_t length, int op
#define M_INTF_DUP    int file
#define M_INTF_DUP2   int file, int fd2
#define M_INTF_FORK   void
#define M_INTF_VFORK  void
#define M_INTF_MMAP   void *addr, size_t len, int prot, int flags, int file, off_t off
#define M_INTF_READV  int file, const struct iovec *iov, int iovcnt
#define M_INTF_WRITEV int file, const struct iovec *iov, int iovcnt
#define M_INTF_PIPE   int file[2]
#define M_INTF_SOCKETPAIR   int domain, int type, int protocol, int sv[2]
#define M_INTF_IOCTL  int file, unsigned long int request, ...
#define M_INTF_MUNMAP void *addr, size_t len
#define M_INTF_MSYNC  void *addr, size_t len, int flags
#define M_INTF_CLONE  int (*fn)(void *a), void *child_stack, int flags, void *arg
#define M_INTF_PREAD  int file,       void *buf, size_t count, size_t offset, int op
#define M_INTF_PREAD64  int file,       void *buf, size_t count, size_t offset, int op
#define M_INTF_PWRITE int file, const void *buf, size_t count, size_t offset, int op
#define M_INTF_PWRITE64 int file, const void *buf, size_t count, size_t offset, int op
//#define M_INTF_PWRITESYNC int file, const void *buf, size_t count, off_t offset
#define M_INTF_FSYNC  int file
#define M_INTF_FDSYNC int file
#define M_INTF_FTRUNC64 int file, off64_t length, int op
#define M_INTF_OPEN64  const char* path, int oflag, ...
#define M_INTF_SEEK64  int file, off64_t offset, int whence, int op
#define M_INTF_MMAP64  void *addr, size_t len, int prot, int flags, int file, off64_t off
#define M_INTF_MKSTEMP char* file
#define M_INTF_MKSTEMP64 char* file
#define M_INTF_ACCEPT  int file, struct sockaddr *addr, socklen_t *addrlen
#define M_INTF_SOCKET  int domain, int type, int protocol
#define M_INTF_UNLINK  const char* path
#define M_INTF_POSIX_FALLOCATE int file, off_t offset, off_t len
#define M_INTF_POSIX_FALLOCATE64 int file, off_t offset, off_t len
#define M_INTF_FALLOCATE int file, int mode, off_t offset, off_t len
#define M_INTF_STAT const char *path, struct stat *buf
#define M_INTF_STAT64 const char *path, struct stat64 *buf
#define M_INTF_FSTAT int file, struct stat *buf
#define M_INTF_FSTAT64 int file, struct stat64 *buf
#define M_INTF_LSTAT const char *path, struct stat *buf
#define M_INTF_LSTAT64 const char *path, struct stat64 *buf
/* Now all the metadata operations */
#define M_INTF_MKDIR const char *path, uint32_t mode
#define M_INTF_RENAME const char *old, const char *new
#define M_INTF_LINK const char *path1, const char *path2
#define M_INTF_SYMLINK const char *path1, const char *path2
#define M_INTF_RMDIR const char *path
/* All the *at operations */
#define M_INTF_OPENAT int dirfd, const char* path, int oflag, ...
#define M_INTF_UNLINKAT  int dirfd, const char* path, int flags
#define M_INTF_SYMLINKAT const char* old_path, int newdirfd, const char* new_path
#define M_INTF_MKDIRAT int dirfd, const char* path, mode_t mode





#ifdef _STAT_VER
#define ORCHFS_ALL_OPS	(OPEN) (LIBC_OPEN64) (OPENAT) (CREAT) (CLOSE) (ACCESS) \
						(SEEK) (TRUNC) (FTRUNC) (LINK) (UNLINK) (FSYNC) \
						(READ) (READ2) (WRITE) (PREAD) (PREAD64) (PWRITE) (PWRITE64) (XSTAT) (STAT64) (FXSTAT) (FSTAT64) (LSTAT) (RENAME)\
						(MKDIR) (RMDIR) (FSTATFS) (FDATASYNC) (FCNTL) (FCNTL2) \
						(OPENDIR) (CLOSEDIR) (READDIR) (READDIR64) (ERROR) (SYNC_FILE_RANGE) \
						(FOPEN) (FPUTS) (FGETS) (FWRITE) (FREAD) (FCLOSE) (FSEEK) (FFLUSH)
#else 
#define ORCHFS_ALL_OPS	(OPEN) (LIBC_OPEN64) (OPENAT) (CREAT) (CLOSE) (ACCESS) \
						(SEEK) (TRUNC) (FTRUNC) (LINK) (UNLINK) (FSYNC) \
						(READ) (READ2) (WRITE) (PREAD) (PREAD64) (PWRITE) (PWRITE64) (STAT) (STAT64) (FSTAT) (FSTAT64) (LSTAT) (RENAME)\
						(MKDIR) (RMDIR) (FSTATFS) (FDATASYNC) (FCNTL) (FCNTL2) \
						(OPENDIR) (CLOSEDIR) (READDIR) (READDIR64) (ERROR) (SYNC_FILE_RANGE) \
						(FOPEN) (FPUTS) (FGETS) (FWRITE) (FREAD) (FCLOSE) (FSEEK) (FFLUSH)
#endif



#define PREFIX(call)				(real_##call)


#define TYPE_REL_SYSCALL(op) 		typedef RETT_##op (*real_##op##_t)(INTF_##op);
#define TYPE_REL_SYSCALL_WRAP(r, data, elem) 		TYPE_REL_SYSCALL(elem)

BOOST_PP_SEQ_FOR_EACH(TYPE_REL_SYSCALL_WRAP, placeholder, ORCHFS_ALL_OPS)

static struct real_ops{
	#define DEC_REL_SYSCALL(op) 	real_##op##_t op;
	#define DEC_REL_SYSCALL_WRAP(r, data, elem) 	DEC_REL_SYSCALL(elem)
	BOOST_PP_SEQ_FOR_EACH(DEC_REL_SYSCALL_WRAP, placeholder, ORCHFS_ALL_OPS)
} real_ops;

void insert_real_op(){
	#define FILL_REL_SYSCALL(op) 	real_ops.op = dlsym(RTLD_NEXT, MK_STR3(ALIAS_##op));
	#define FILL_REL_SYSCALL_WRAP(r, data, elem) 	FILL_REL_SYSCALL(elem)
	BOOST_PP_SEQ_FOR_EACH(FILL_REL_SYSCALL_WRAP, placeholder, ORCHFS_ALL_OPS)
}
#define OP_DEFINE(op)		RETT_##op ALIAS_##op(INTF_##op)

static int inited = 0;
OP_DEFINE(OPEN){
	// printf("open call! %s %d\n", path, oflag & O_CREAT);
	// fflush(stdout);
	if(real_ops.OPEN == NULL){
		insert_real_op();
	}
	int dirflag=0;
	if(path[0]=='/'&&path[1]=='O'&&path[2]=='r')
		dirflag=1;
	// fprintf(stderr,"open call\n");
	// fprintf(stderr,"open %s\n",path);
	// fprintf(stderr,"oflag=%x\n",oflag);
	if(*path == '\\' || *path != '/' || path[1] == '\\' || dirflag){
		// printf("myopen call! %s %d\n", path, oflag & O_CREAT);
		int ret;
		if(oflag & O_CREAT){
			// fprintf(stderr,"open 111111\n");
			va_list ap;
			mode_t mode;
			va_start(ap, oflag);
			mode = va_arg(ap, mode_t);
			//printf("222222\n");
			PRINT_FUNC;

			char * p = path;
			while(*p == '\\'){
				p ++;
			}
			if(p[1] == '\\'){
				p++;
				*p = '/';
			}
			if(dirflag)
				p+=3;
			ret = orchfs_open(p, oflag, mode);
			// fprintf(stderr,"O_CREAT open ret=%d\n",ret+1024);
			if(ret == -1 && *path != '\\' && !dirflag){
				return real_ops.OPEN(path, oflag, mode);
			}
		}
		else{
			PRINT_FUNC;
			if(*path=='\\')
				ret = orchfs_open(path + 1, oflag);
			else if(dirflag)
				ret = orchfs_open(path + 3, oflag);
			else ret=orchfs_open(path,oflag);
			// printf("open ret=%d\n",ret);
			if(ret == -1 && *path != '\\' &&!dirflag){

				return real_ops.OPEN(path, oflag);
			}
		}
		if(ret == -1){
			exit(0);
			int len=strlen(path);
			//printf("len=%d path[len-4]=%c path[len-3]=%c path[len-2]=%c path[len-1]=%c\n",len,path[len-4],path[len-3],path[len-2],path[len-1]);
			char newpath[20];
			strcpy(newpath,path);
			// if(path[len-4]=='.'&&path[len-3]=='s'&&path[len-2]=='s'&&path[len-1]=='t')
			// {
			// 	newpath[len-3]='l';newpath[len-2]='d';newpath[len-1]='b';newpath[len]='\0';
			// }
			// printf("my open error\n");
			// printf("test open again!\n");
			// printf("open again path=%s dirflag=%d\n",newpath,dirflag);
			// va_list ap;
			// oflag|=O_CREAT;
			// oflag|=O_RDWR;
			if(*path=='\\')
				ret = orchfs_open(newpath + 1, oflag);
			else if(dirflag)
				ret = orchfs_open(newpath + 3, oflag);
			else ret=orchfs_open(newpath,oflag);
			// fprintf(stderr,"open again ret=%d\n",ret+1024);
			// if(oflag & O_CREAT)
			// 	printf("O_CREAT\n");
			// if(oflag & O_RDONLY)
			// 	printf("O_RDONLY\n");
			// if(oflag & O_WRONLY)
			// 	printf("O_WRONLY\n");
			// if(oflag & O_RDWR)
			// 	printf("O_RDWR\n");
			// if(oflag & O_APPEND)
			// 	printf("O_APPEND\n");
			// if(oflag & O_SYNC)
			// 	printf("O_SYNC\n");
			// exit(0);
			return ret;
		}
		return ret + ORCH_FD_OFFSET;
	}
	else{
		// fprintf(stderr,"real open:%s\n",path);
		if(oflag & O_CREAT){
			va_list ap;
			mode_t mode;
			va_start(ap, oflag);
			mode = va_arg(ap, mode_t);
			return real_ops.OPEN(path, oflag, mode);
		}
		else{
			int ret_fd = real_ops.OPEN(path, oflag);
			// printf("ret_fd: %d\n",ret_fd);
			// fflush(stdout);
			return ret_fd;
		}
	}
}


OP_DEFINE(LIBC_OPEN64){
	fprintf(stderr,"libc_open call\n");
	fflush(stdout);
	exit(1);
	// printf("libc_open %s\n",path);
// 	int dirflag=0;
// 	if(path[0]=='/'&&path[1]='O'&&path[2]=='r')
// 		dirflag=1;
// 	if(*path == '\\' || *path != '/' || dirflag){
// 		//printf("my lic_open\n");
// 		int ret;
// 		if(oflag & O_CREAT){
// 			va_list ap;
// 			mode_t mode;
// 			va_start(ap, oflag);
// 			mode = va_arg(ap, mode_t);
// 			PRINT_FUNC;
// #ifdef WRAPPER_DEBUG
// 			printf("\t\tpath: %s\n", path);
// #endif
// 			if(*path=='\\')
// 				ret = orchfs_open(path + 1, oflag, mode);
// 			else if(dirflag)
// 				ret = orchfs_open(path + 3, oflag, mode);
// 			else ret = orchfs_open(path, oflag, mode);
// 			if(ret == -1 && *path != '\\' && !dirflag){
// 				return real_ops.OPEN(path, oflag, mode);
// 			}
// 		}
// 		else{
// #ifdef WRAPPER_DEBUG
// 			printf("\t\tpath: %s\n", path);
// #endif
// 			PRINT_FUNC;
// 			if(*path=='\\')
// 				ret = orchfs_open(path + 1, oflag);
// 			else if(dirflag)
// 				ret = orchfs_open(path + 3, oflag);
// 			else ret = orchfs_open(path, oflag);
// 			if(ret == -1 && *path != '\\' && !dirflag){
// 				return real_ops.OPEN(path, oflag);
// 			}
// 		}
// 		if(ret == -1){
// 			return ret;
// 		}
// #ifdef WRAPPER_DEBUG
// 		printf("open returned: %d\n", ret + ORCH_FD_OFFSET);
// #endif
// 		return ret + ORCH_FD_OFFSET;
// 	}
// 	else{
// 		if(oflag & O_CREAT){
// 			va_list ap;
// 			mode_t mode;
// 			va_start(ap, oflag);
// 			mode = va_arg(ap, mode_t);
// 			return real_ops.OPEN(path, oflag, mode);
// 		}
// 		else{
// 			return real_ops.OPEN(path, oflag);
// 		}
// 	}
}

OP_DEFINE(OPENAT){
	printf("openat call %s\n",path);
	fflush(stdout);
	// printf("openat %s\n",path);
	int dirflag=0;
	if(path[0]=='/'&&path[1]=='O'&&path[2]=='r')
		dirflag=1;
	if(*path == '\\' || *path != '/' || dirflag){
		//printf("my openat\n");
		int ret;
		if(oflag & O_CREAT){
			va_list ap;
			mode_t mode;
			va_start(ap, oflag);
			mode = va_arg(ap, mode_t);
			PRINT_FUNC;
			if(*path=='\\')
				ret = orchfs_openat(dirfd - ORCH_FD_OFFSET,path + 1, oflag, mode);
			else if(dirflag)
				ret = orchfs_openat(dirfd - ORCH_FD_OFFSET,path + 3, oflag, mode);
			else ret = orchfs_openat(dirfd - ORCH_FD_OFFSET,path, oflag, mode);
			//ret = orchfs_openat(dirfd - ORCH_FD_OFFSET, (*path == '\\')? path + 1 : path, oflag, mode);
			if(ret == -1 && *path != '\\' && !dirflag){
				return real_ops.OPENAT(dirfd, path, oflag, mode);
			}
		}
		else{
			PRINT_FUNC;
			if(*path=='\\')
				ret = orchfs_openat(dirfd - ORCH_FD_OFFSET,path + 1, oflag);
			else if(dirflag)
				ret = orchfs_openat(dirfd - ORCH_FD_OFFSET,path + 3, oflag);
			else ret = orchfs_openat(dirfd - ORCH_FD_OFFSET,path, oflag);
			//ret = orchfs_openat(dirfd - ORCH_FD_OFFSET, (*path == '\\')? path + 1 : path, oflag);
			if(ret == -1 && *path != '\\' && !dirflag){
				return real_ops.OPENAT(dirfd, path, oflag);
			}
		}
		if(ret == -1){
			return ret;
		}
		return ret + ORCH_FD_OFFSET;
	}
	else{
		if(oflag & O_CREAT){
			va_list ap;
			mode_t mode;
			va_start(ap, oflag);
			mode = va_arg(ap, mode_t);
			return real_ops.OPENAT(dirfd, path, oflag, mode);
		}
		else{
			return real_ops.OPENAT(dirfd, path, oflag);
		}
	}
}

OP_DEFINE(CREAT){
	// printf("creat call\n");
	// fflush(stdout);
	// printf("creat path=%s\n",path);
	int dirflag=0;
	if(path[0]=='/'&&path[1]=='O'&&path[2]=='r')
		dirflag=1;
	if(*path == '\\' || *path != '/' || dirflag){
		PRINT_FUNC;
		// printf("my creat\n");
		int ret;
		if(*path=='\\')
			ret = orchfs_open(path + 1,O_CREAT, mode);
		else if(dirflag)
			ret = orchfs_open(path + 3,O_CREAT, mode);
		else 
			ret = orchfs_open(path, O_CREAT, mode);
		//int ret = orchfs_open((*path == '\\')? path + 1 : path, O_CREAT, mode);
		if(ret == -1 && *path != '\\' && !dirflag){
			return real_ops.CREAT(path, mode);
		}
		if(ret != -1){
			orchfs_close(ret);
		}
		return 0;
	}
	else{
		return real_ops.CREAT(path, mode);
	}
}

OP_DEFINE(CLOSE){
	// printf("close call\n");
	// printf("clse fd=%d\n",file);
	// fflush(stdout);
	if(file >= ORCH_FD_OFFSET){
		PRINT_FUNC;
		// fprintf(stderr,"my close\n");
		return orchfs_close(file - ORCH_FD_OFFSET);
	}
	else{
		return real_ops.CLOSE(file);
	}
}

OP_DEFINE(ACCESS){
	// printf("access call\n");
	// printf("access %s\n",pathname);
	// fflush(stdout);
	int dirflag=0;
	if(pathname[0]=='/'&&pathname[1]=='O'&&pathname[2]=='r')
		dirflag=1;
	if(*pathname == '\\' || *pathname != '/' ||dirflag){
		// PRINT_FUNC;
		// printf("my access %d\n",dirflag);
		int ret;
		if(*pathname=='\\')
		{
			ret = orchfs_open(pathname + 1, mode);
		}
		else if(dirflag)
		{
			ret = orchfs_open(pathname + 3, mode);
		}
		else 
		{
			ret = orchfs_open(pathname, mode);
		}
		//int ret = orchfs_access((*pathname == '\\')? pathname + 1 : pathname, mode);
		if(ret == -1 && *pathname != '\\' && !dirflag){
			return real_ops.ACCESS(pathname, mode);
		}
		if(ret != -1){
			return 0;
		}
		return -1;
	}
	else{
		return real_ops.ACCESS(pathname, mode);
	}
}

OP_DEFINE(SEEK){
	// printf("seek call, fd=%d\n",file);
	// fflush(stdout);
	if(file >= ORCH_FD_OFFSET)
	{
		PRINT_FUNC;
		return orchfs_lseek(file - ORCH_FD_OFFSET, offset, whence);
	}
	else
	{
		return real_ops.SEEK(file, offset, whence);
	}
}

OP_DEFINE(TRUNC){
	// printf("trunc call\n");
	// fflush(stdout);
	// printf("trunc path=%s length=%d\n",path,length);
	// fflush(stdout);
	int dirflag=0;
	if(path[0]=='/'&&path[1]=='O'&&path[2]=='r')
		dirflag=1;
	if(*path == '\\' || *path != '/' || dirflag){
		PRINT_FUNC;
		//printf("my trunc\n");
		int ret;
		if(*path=='\\')
			ret = orchfs_truncate(path + 1, length);
		else if(dirflag)
			ret = orchfs_truncate(path + 3, length);
		else ret = orchfs_truncate(path, length);
		if(ret == -1){
			if(*path != '\\' && !dirflag){
				return real_ops.TRUNC(path, length);
			}
			return -1;
		}
		return 0;
	}
	else{
		return real_ops.TRUNC(path, length);
	}
}

OP_DEFINE(FTRUNC){
	printf("ftrunc call\n");
	fflush(stdout);
	//printf("fd=%d\n",file);
	// int op = 0;
	// if(file >= ORCH_FD_OFFSET){
	// 	PRINT_FUNC;
	// 	//printf("my frunc\n");
	// 	return orchfs_ftruncate(file - ORCH_FD_OFFSET, length, op);
	// }
	// else{
	// 	return real_ops.FTRUNC(file, length);
	// }
}

/*LINk*/
OP_DEFINE(LINK){
	printf("link call\n");
	fflush(stdout);
	return 0;
	//printf("path1=%s path2=%s\n",path1,path2);
	// int dirflag=0;
	// if(path1[0]=='/'&&path1[1]=='O'&&path1[2]=='r')
	// 	dirflag=1;
	// if(*path1 == '\\' || *path1 != '/' || dirflag){
	// 	PRINT_FUNC;
	// 	//printf("my link\n");
	// 	char *p=path1;
	// 	if(*path1=='\\') path1=path1+1;
	// 	if(dirflag) path1=path1+3;
	// 	if(path2=='\\') path2=path2+1;
	// 	if(path2[0]=='/'&&path2[1]=='O'&&path2[2]=='r') path2=path2+3;
	// 	if(orchfs_link(path1, path2) == -1){
	// 		if(*p != '\\' && !dirflag){
	// 			return real_ops.LINK(path1, path2);
	// 		}
	// 		return -1;
	// 	}
	// 	return 0;
	// }
	// else{
	// 	return real_ops.LINK(path1, path2);
	// }
}

OP_DEFINE(UNLINK){
	// printf("unlink path=%s\n",path);
	// fflush(stdout);
	int dirflag=0;
	if(path[0]=='/'&&path[1]=='O'&&path[2]=='r')
		dirflag=1;
	if(*path == '\\' || *path != '/' || dirflag){
		PRINT_FUNC;
		//printf("my unlink\n");
		int ret;
		if(*path=='\\')
			ret = orchfs_unlink(path + 1);
		else if(dirflag)
			ret = orchfs_unlink(path + 3);
		else ret = orchfs_unlink(path);
		if(ret == -1){
			if(*path != '\\' && !dirflag){
				return real_ops.UNLINK(path);
			}
			return -1;
		}
#ifdef orchfs_DEBUG
		printf("unlinked: %s\n", path);
#endif
		return 0;
	}
	else{
		return real_ops.UNLINK(path);
	}
}

OP_DEFINE(FSYNC){
	// printf("fsync call\n");
	// printf("fsync fd=%d\n",file);
	// fflush(stdout);
	if(file >= ORCH_FD_OFFSET){
		// printf("my fsync\n");
		return 0;
	}
	else{
		return real_ops.FSYNC(file);
	}
}

OP_DEFINE(READ){
	// fflush(stdout);
	if(file >= ORCH_FD_OFFSET){
		PRINT_FUNC;
		// printf("read call fd=%d %d\n",file,length);
		int64_t read_len = orchfs_read(file - ORCH_FD_OFFSET, buf, length);
		// char* buf_char = buf;
		// printf("read data: %s\n",buf_char);
		return read_len;
	}
	else{
		if(real_ops.READ == NULL){
			insert_real_op();
		}
		// char* buf_char = buf;
		int64_t read_len = real_ops.READ(file, buf, length);
		// printf("read data: %s\n",buf_char);
		return read_len;
	}
}

OP_DEFINE(READ2){
	// printf("read2 call fd=%d\n",file);
	// fflush(stdout);
	if(file >= ORCH_FD_OFFSET){
		PRINT_FUNC;
		//printf("my read2\n");
		return orchfs_read(file - ORCH_FD_OFFSET, buf, length);
	}
	else{
		return real_ops.READ2(file, buf, length);
	}
}

OP_DEFINE(WRITE){
	// char* buf_char = buf;
	// fflush(stdout);
	if(file >= ORCH_FD_OFFSET){
		// PRINT_FUNC;
		// printf("write call fd=%d %d\n",file,length);
		int64_t write_len = orchfs_write(file - ORCH_FD_OFFSET, buf, length);
		// printf("write_len: %d %d\n",file,write_len);
		// char new_buf[4096] = {0};
		// orchfs_pread(file - ORCH_FD_OFFSET, new_buf, write_len, 0);
		// for(int i = 0; i < write_len; i++)
		// {
		// 	printf("info: %d %d %d\n",file,buf_char[i],new_buf[i]);
		// }
		return write_len;
	}
	else{
		return real_ops.WRITE(file, buf, length);
	}
}

OP_DEFINE(PREAD){
	// printf("pread call fd=%d count=%d offset=%d\n",file,count,offset);
	// fflush(stdout);
	if(file >= ORCH_FD_OFFSET){
		PRINT_FUNC;
		// fprintf(stderr,"my pread\n");
		// fprintf(stderr,"\t\tread: %lu\n", count);
		return orchfs_pread(file - ORCH_FD_OFFSET, buf, count, offset);
	}
	else{
		return real_ops.PREAD(file, buf, count, offset);
	}
}

OP_DEFINE(PREAD64){
	// printf("pread64 call fd=%d count=%d offset=%d\n",file,count,offset);
	// fflush(stdout);
	if(file >= ORCH_FD_OFFSET){
		PRINT_FUNC;
		// fprintf(stderr,"my pread64\n");
		// fprintf(stderr,"\t\tread: %lu\n", count);
		return orchfs_pread(file - ORCH_FD_OFFSET, buf, count, offset);
	}
	else{
		return real_ops.PREAD64(file, buf, count, offset);
	}
}

OP_DEFINE(PWRITE){
	// char* buf_char = buf;
	if(file >= ORCH_FD_OFFSET){
		PRINT_FUNC;
		//printf("111111\n");
		// printf("pwrite call fd=%d %lld %lld\n",file,count,offset);
		return orchfs_pwrite(file - ORCH_FD_OFFSET, buf, count, offset);
	}
	else{
		//printf("wrapper %d\n",__LINE__);
		// printf("file=%d count=%llu offset=%llu\n",file,count,offset);
		return real_ops.PWRITE(file, buf, count, offset);
	}
}

OP_DEFINE(PWRITE64){
	// fprintf(stderr,"pwrite64 call fd=%d\n",file);
	if(file >= ORCH_FD_OFFSET){
		PRINT_FUNC;
		// fprintf(stderr,"my pwrite64\n");
		//printf("now work in PWRITE64\n");
		return orchfs_pwrite(file - ORCH_FD_OFFSET, buf, count, offset);
	}
	else{
		//printf("wrapper %d\n",__LINE__);
		return real_ops.PWRITE64(file, buf, count, offset);
	}
}

#ifdef _STAT_VER
OP_DEFINE(XSTAT){
	// printf("stat call\n");
	// fflush(stdout);
	// printf("xstat path=%s\n",path);
	// fflush(stdout);
	int dirflag=0;
	if(path[0]=='/'&&path[1]=='O'&&path[2]=='r')
		dirflag=1;
	if(*path == '\\' || *path != '/' || dirflag){
		PRINT_FUNC;
		//printf("my stat\n");
		int ret;
		if(*path=='\\')
			ret = orchfs_stat( path + 1, buf);
		else if(dirflag)
			ret = orchfs_stat( path + 3, buf);
		else ret = orchfs_stat( path , buf);
		if(ret == -1){
			// if(*path != '\\' && !dirflag){
			// 	return real_ops.XSTAT(_STAT_VER, path, buf);
			// }
			return -1;
		}
		return 0;
	}
	else{
		return real_ops.XSTAT(_STAT_VER, path, buf);
	}
}
#else 
OP_DEFINE(STAT){
	if(real_ops.STAT == NULL){
		insert_real_op();
	}
	// printf("stat call\n");
	// fflush(stdout);
	// printf("stat path=%s\n",path);
	// fflush(stdout);
	int dirflag=0;
	if(path[0]=='/'&&path[1]=='O'&&path[2]=='r')
		dirflag=1;
	if(*path == '\\' || *path != '/' || dirflag){
		PRINT_FUNC;
		//printf("my stat\n");
		int ret;
		if(*path=='\\')
			ret = orchfs_stat( path + 1, buf);
		else if(dirflag)
			ret = orchfs_stat( path + 3, buf);
		else ret = orchfs_stat( path , buf);
		if(ret == -1){
			// if(*path != '\\' && !dirflag){
			// 	return real_ops.STAT(path, buf);
			// }
			return -1;
		}
		return 0;
	}
	else{
		return real_ops.STAT(path, buf);
	}
}
#endif

OP_DEFINE(STAT64){
	// printf("stat64 call\n");
	// fflush(stdout);
	// fprintf(stderr,"stat64 path=%s\n",path);
	// fflush(stdout);
	int dirflag=0;
	if(path[0]=='/'&&path[1]=='O'&&path[2]=='r')
		dirflag=1;
	if(*path == '\\' || *path != '/' || dirflag){
		PRINT_FUNC;
		//printf("my stat64\n");
		int ret;
		if(*path=='\\')
			ret = orchfs_stat( path + 1, (struct stat*)buf);
		else if(dirflag)
			ret = orchfs_stat( path + 3, (struct stat*)buf);
		else ret = orchfs_stat( path , (struct stat*)buf);
		if(ret == -1){
			if(*path != '\\' && !dirflag){
				return real_ops.STAT64(path, (struct stat*)buf);
			}
			return -1;
		}
		return 0;
	}
	else{
		return real_ops.STAT64(path, buf);
	}
}

#ifdef _STAT_VER
OP_DEFINE(FXSTAT){
	// printf("fstat call\n");
	// fflush(stdout);
	// printf("fxfd=%d\n",file);
	if(file >= ORCH_FD_OFFSET){
		PRINT_FUNC;
		//printf("my fstat\n");
		return orchfs_fstat(file - ORCH_FD_OFFSET, buf);
	}
	else{
		return real_ops.FXSTAT(_STAT_VER, file, buf);
	}
}
#else 
OP_DEFINE(FSTAT){
	// printf("fstat call\n");
	// fflush(stdout);
	// printf("fd=%d\n",file);
	if(file >= ORCH_FD_OFFSET){
		PRINT_FUNC;
		//printf("my fstat\n");
		return orchfs_fstat(file - ORCH_FD_OFFSET, buf);
	}
	else{
		return real_ops.FSTAT(file, buf);
	}
}
#endif

OP_DEFINE(FSTAT64){
	// fflush(stdout);
	// fprintf(stderr,"fd=%d\n",file);
	// fflush(stdout);
	if(file >= ORCH_FD_OFFSET){
		PRINT_FUNC;
		//printf("my fstat64\n");
		return orchfs_fstat(file - ORCH_FD_OFFSET, (struct stat*)buf);
	}
	else{
		return real_ops.FSTAT64(file, (struct stat*)buf);
	}
}


OP_DEFINE(LSTAT){
	// printf("lstat call\n");
	// fflush(stdout);
	// fprintf(stderr,"lstat path=%s\n",path);
	// fflush(stdout);
	int dirflag=0;
	if(path[0]=='/'&&path[1]=='O'&&path[2]=='r')
		dirflag=1;
	if(*path == '\\' || *path != '/'){
		PRINT_FUNC;
		//printf("my lstat\n");
		int ret;
		if(*path=='\\')
			ret = orchfs_stat( path + 1,buf);
		else if(dirflag)
			ret = orchfs_stat( path + 3,buf);
		else ret = orchfs_stat( path , buf);
		if(ret == -1){
			if(*path != '\\' && !dirflag){
				return real_ops.LSTAT(path, buf);
			}
			return -1;
		}
		return 0;
	}
	else{
		return real_ops.LSTAT(path, buf);
	}
}

OP_DEFINE(RENAME){
	// printf("oldname=%s newname=%s\n",old,new);
	// fflush(stdout);
	int dirflag=0;
	if(old[0]=='/'&&old[1]=='O'&&old[2]=='r')
		dirflag=1;
	if(*old == '\\' || *old != '/' || dirflag){
		PRINT_FUNC;
		//printf("my rename\n");
		char *p=new;
		if(*old=='\\') old=old+1;
		if(dirflag) old=old+3;
		if(*new=='\\') new=new+1;
		if(new[0]=='/'&&new[1]=='O'&&new[2]=='r') new=new+3;
		//printf("old=%s new=%s\n",old,new);
		if(orchfs_rename( old, new) == -1)
		{
			if(*p != '\\'){
				return real_ops.RENAME(new, new);
			}
			return -1;
		}
		return 0;
	}
	else{
		return real_ops.RENAME(old, new);
	}
}

OP_DEFINE(MKDIR){
	// fprintf(stderr,"mkdir call\n");
	// printf("mkdir path=%s mode=%d\n",path,mode);
	// fflush(stdout);
	int dirflag=0;
	if(path[0]=='/'&&path[1]=='O'&&path[2]=='r')
		dirflag=1;
	if(*path == '\\' || *path != '/' ||dirflag){
		// fprintf(stderr,"my mkdir\n");
		// PRINT_FUNC;
		int ret;
		if(*path=='\\')
			ret = orchfs_mkdir(path + 1, mode);
		else if(dirflag)
			ret = orchfs_mkdir(path + 3, mode);
		else 
			ret = orchfs_mkdir(path , mode);
		// fprintf(stderr,"mkdir over\n");
		if(ret == -1){
			// if(*path != '\\' && !dirflag){
			// 	return real_ops.MKDIR(path, mode);
			// }
			// printf("mkdir error1\n");
			// fflush(stdout);
			return -1;
		}
		// fprintf(stderr,"mkdir return\n");
		return 0;
	}
	else{
		// fprintf(stderr,"real mkdir:%s\n",path);
		return real_ops.MKDIR(path, mode);
	}
}

OP_DEFINE(RMDIR){
	// printf("rmdir path=%s\n",path);
	// fflush(stdout);
	int dirflag=0;
	if(path[0]=='/'&&path[1]=='O'&&path[2]=='r')
		dirflag=1;
	if(*path == '\\' || *path != '/' || dirflag){
		// PRINT_FUNC;
		int ret;
		if(*path=='\\')
			ret = orchfs_rmdir(path + 1);
		else if(dirflag)
			ret = orchfs_rmdir(path + 3);
		else 
			ret = orchfs_rmdir(path);
		// fprintf(stderr,"mkdir over\n");
		if(ret == -1){
			// if(*path != '\\' && !dirflag){
			// 	return real_ops.MKDIR(path, mode);
			// }
			fprintf(stderr,"rmdir error\n");
			return -1;
		}
		// fprintf(stderr,"mkdir return\n");
		return 0;
		//printf("my rmdir\n");
		// if(orchfs_rmdir((*path == '\\')? path + 1 : path) == -1){
		// 	if(*path != '\\'){
		// 		return real_ops.RMDIR(path);
		// 	}
		// 	return -1;
		// }
	}
	else
	{
		return real_ops.RMDIR(path);
	}
}

OP_DEFINE(FSTATFS){
	// printf("fstatfs call\n");
	// fflush(stdout);
	//printf("fd=%d\n",fd);
	if(fd >= ORCH_FD_OFFSET){
		PRINT_FUNC;
		//printf("my fstatfs\n");
		return orchfs_fstatfs(fd - ORCH_FD_OFFSET, buf);
	}
	else{
		return real_ops.FSTATFS(fd, buf);
	}
}

OP_DEFINE(FDATASYNC){
	// printf("fdatasync call\n");
	// fflush(stdout);
	//printf("fd=%d\n",fd);
	if(fd >= ORCH_FD_OFFSET){
		//printf("my fdatasync\n");
		return 0;
	}
	else{
		return real_ops.FDATASYNC(fd);
	}
}

OP_DEFINE(FCNTL){
	// printf("fcntl call\n");
	// fflush(stdout);
	// exit(0);
	// printf("fd=%d\n",fd);
	va_list ap;
	int ret;
	if(fd >= ORCH_FD_OFFSET){
		PRINT_FUNC;
		//printf("my fcntl\n");
		switch (cmd)
		{
		case F_GETFD :
			return FD_CLOEXEC;
			break;
		case F_GETFL :
			// printf("got GETFL!\n");
			ret = orchfs_fcntl(fd - ORCH_FD_OFFSET, cmd);
			return ret;
			break;
		case F_SETFD :
			return 0;
			break;
		case F_SETFL :
			va_start(ap, cmd);
			int val = va_arg(ap, int);
			return orchfs_fcntl(fd - ORCH_FD_OFFSET, cmd, val);
			break;
		case F_SET_RW_HINT:
			return 0;
			break;
		default:
			return 0;
			break;
		}
	}
	else{
		va_list ap;
		va_start(ap, cmd);
		switch (cmd)
		{
		case F_GETFD :
		case F_GETFL :
			real_ops.FCNTL(fd, cmd);
			break;
		case F_SETFD :
		case F_SETFL :
			
			real_ops.FCNTL(fd, cmd, va_arg(ap, int));
		case F_SET_RW_HINT:
			return real_ops.FCNTL(fd, cmd, va_arg(ap, void*));
			break;
		default:
			return real_ops.FCNTL(fd, cmd);
			break;
		}
	}
	return 0;
}

OP_DEFINE(OPENDIR){
	// printf("opendir %s\n",path);
	// fflush(stdout);
	int dirflag=0;
	if(path[0]=='/'&&path[1]=='O'&&path[2]=='r')
		dirflag=1;
	if(*path == '\\' || *path != '/' || dirflag){
		// PRINT_FUNC;
		// printf("my opendir\n");
		DIR* ret;
		if(*path=='\\')
			ret = orchfs_opendir(path + 1);
		else if(dirflag)
			ret = orchfs_opendir(path + 3);
		else 
			ret = orchfs_opendir(path );
		if(ret == NULL){
			if(*path != '\\' && !dirflag){
				return real_ops.OPENDIR(path);
			}
			return NULL;
		}
		return (DIR*)((uint64_t)ret);
	}
	else{
		if(inited == 0){
			insert_real_op();
		}
		return real_ops.OPENDIR(path);
	}
}

OP_DEFINE(CLOSEDIR){
	// printf("closedir call fd=%d\n",(int)(uint64_t)dirp);
	// fflush(stdout);
	if((uint64_t)dirp < ORCH_MAX_FD){
		PRINT_FUNC;
		//printf("my closedir\n");
		return orchfs_closedir((DIR*) ((uint64_t)dirp));
	}
	else{
		return real_ops.CLOSEDIR(dirp);
	}
}

OP_DEFINE(READDIR){
	// printf("readdir call, fd=%d\n",(int)(uint64_t)dirp);
	// fflush(stdout);
	if((uint64_t)dirp < ORCH_MAX_FD){
		PRINT_FUNC;
		// struct dirent* ret = orchfs_readdir((DIR*) ((uint64_t)dirp));
		// printf("my readdir ret %lld %d\n",ret,sizeof(ret));
		// fflush(stdout);
		// return ret;
		return orchfs_readdir((DIR*) ((uint64_t)dirp));
	}
	else{
		return real_ops.READDIR(dirp);
	}
}

OP_DEFINE(READDIR64){
	// printf("readdir64 call\n");
	// fflush(stdout);
	exit(0);
	//printf("fd=%d\n",(int)(uint64_t)dirp);
	// if((uint64_t)dirp < ORCH_MAX_FD){
	// 	PRINT_FUNC;
	// 	//printf("my readdir64\n");
	// 	return (struct dirent64*)orchfs_readdir((DIR*) ((uint64_t)dirp));
	// }
	// else{
	// 	return real_ops.READDIR64(dirp);
	// }
}

// static int err;

OP_DEFINE(ERROR){
	// printf("error call\n");
	// fflush(stdout);
	return real_ops.ERROR();
	// if(real_ops.ERROR==0){
	// 	insert_real_op();
	// }
	// if(*orchfs_errno() == 0){
	// 	return real_ops.ERROR();
	// }
	// else{
	// 	// err = *orchfs_errno();
	// 	// *orchfs_errno() = 0;
	// 	// return &err;
	// 	exit(0);
	// }
}


OP_DEFINE(SYNC_FILE_RANGE){
	// printf("sync_file_range call\n");
	// fflush(stdout);
	//printf("fd=%d\n",fd);
	if(fd >= ORCH_FD_OFFSET ){
		PRINT_FUNC;
		//printf("my sync_file_range\n");
		return 0;
	}
	else{
		return real_ops.SYNC_FILE_RANGE(fd, offset, nbytes, flags);
	}
}

// /*******************************************************
//  * File stream functions
//  *******************************************************/
OP_DEFINE(FOPEN){
	// printf("fopen call! %s\n",path);
	// fflush(stdout);
	int dirflag=0;
	if(path[0]=='/'&&path[1]=='O'&&path[2]=='r')
		dirflag=1;
	if(*path == '\\' || *path != '/' || dirflag){
		FILE * ret = NULL;
		//printf("my fopen\n");
		PRINT_FUNC;
		if(*path=='\\')
			ret = _fopen(path + 1, mode);
		else if(dirflag)
			ret = _fopen(path + 3, mode);
		else ret = _fopen(path , mode);
		//ret = _fopen((*path == '\\')? path + 1 : path, mode);
		if(ret == NULL && *path != '\\' && !dirflag){
			return real_ops.FOPEN(path, mode);
		}
		return ret;
	}
	else{
		return real_ops.FOPEN(path, mode);
	}
	return real_ops.FOPEN(path, mode);
}

/*op*/
OP_DEFINE(FPUTS){
	// printf("fputs call\n");
	// fflush(stdout);
	if(1){
		if((stream->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_ORCHFS){
			//printf("my fputs\n");
			PRINT_FUNC;
			return _fputs(str, stream);
		}
	}
	return real_ops.FPUTS(str, stream);
}
OP_DEFINE(FGETS){
	// printf("fgets call\n");
	// fflush(stdout);
	if(stream){
		if((stream->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_ORCHFS){
			PRINT_FUNC;
			//printf("my fgets\n");
			return _fgets(str, n, stream);
		}
	}
	return real_ops.FGETS(str, n, stream);
}

OP_DEFINE(FWRITE){
	// char* buf_char = buf;
	// printf("fwrite call: %d\n",length);
	// printf("fd0: %d\n",fp->_fileno);
	fflush(stdout);
	if(1){
		if((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_ORCHFS)
		{
			// PRINT_FUNC;
			// printf("my fwrite %d %d\n",length, nmemb);
			return _fwrite(buf, length, nmemb, fp);
			// return nmemb;
		}
	}
	return real_ops.FWRITE(buf, length, nmemb, fp);
}

OP_DEFINE(FREAD){
	// printf("fread call: %d\n",length);
	fflush(stdout);
	if(fp){
		if((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_ORCHFS){
			PRINT_FUNC;
			//printf("my fread\n");
			return _fread(buf, length, nmemb, fp);
		}
	}
	return real_ops.FREAD(buf, length, nmemb, fp);
}

OP_DEFINE(FCLOSE){
	// printf("fclose call\n");
	// fflush(stdout);
	if(fp){
		if((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_ORCHFS){
			PRINT_FUNC;
			//printf("my fclose\n");
			return _fclose(fp);
		}
	}
	return real_ops.FCLOSE(fp);
}

OP_DEFINE(FSEEK){
	// printf("fseek call\n");
	// fflush(stdout);
	if(fp){
		if((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_ORCHFS){
			PRINT_FUNC;
			//printf("my fseek\n");
			return _fseek(fp, offset, whence);
		}
	}
	return real_ops.FSEEK(fp, offset, whence);
}

OP_DEFINE(FFLUSH){
	// printf("fflush call\n");
	// fflush(stdout);
	if(fp){
		if((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_ORCHFS){
			PRINT_FUNC;
			//printf("my fflush\n");
			return _fflush(fp);
		}
	}
	return real_ops.FFLUSH(fp);
}

// OP_DEFINE(MMAP){
// 	printf("mmap call d=%d\n",file);
// 	fflush(stdout);
// 	return real_ops.MMAP(addr, len, prot, flags, file, off);
// }

static __attribute__((constructor(100) )) void init_method(void)
{
	//printf("1111\n");
    if(real_ops.ERROR == 0){
		insert_real_op();
		inited = 1;
	}

	// printf("Starting to initialize ctFS. \nInstalling real syscalls...\n");
    // fprintf(stderr,"Real syscall installed. Initializing ORCHFS...\n");
#ifdef ORCHFS_ATOMIC_WRITE
	printf("Atomic write is enabled!\n");
#endif
	if(real_ops.ERROR != 0){
		// orchfs_init(0);
		// printf("------------ORCHFS init----------------\n");
		init_libfs();
		fprintf(stderr,"OrchFS initialized. \nNow the program begins.\n");
		fflush(stdout);
		return;
	}
	printf("Fialed to init\n");
	inited = 0;
}

static __attribute__((destructor(101))) void over_method(void)
{
	close_libfs();
	// fprintf(stderr,"libFS over %"PRId64"\n",all_read_size);
	fprintf(stderr,"libFS over\n");
	fflush(stdout);
}

#ifdef __cplusplus
}
#endif
