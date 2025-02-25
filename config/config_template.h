#ifndef ORCHFS_CONFIG_H
#define ORCHFS_CONFIG_H

#ifdef __cplusplus       
extern "C"{
#endif

// #define ORCHFS_ATOMIC_WRITE
// #define ORCHFS_ATOMIC_WRITE_USE_UNDO

#define MPK_DEFAULT		0
#define MPK_META		1
#define MPK_FILE		2

// #define NEWBASELINE

#define INF 0xffffffffffffffff

// bitwidth
#define BW_4KiB           12
#define BW_8KiB           13
#define BW_16KiB          14
#define BW_32KiB          15
#define BW_64KiB          16
#define BW_128KiB         17
#define BW_256KiB         18

// standard block size
#define SIZE_4KiB           (1 << BW_4KiB)
#define SIZE_16KiB          (1 << BW_16KiB)
#define SIZE_32KiB          (1 << BW_32KiB)
#define SIZE_64KiB          (1 << BW_64KiB)
#define SIZE_128KiB         (1 << BW_128KiB)
#define SIZE_256KiB         (1 << BW_256KiB)

// block size
#define ORCH_SUPER_BLK_SIZE             512LL
#define ORCH_INODE_SIZE                 512LL
#define ORCH_IDX_SIZE                   1024LL
#define ORCH_VIRND_SIZE                 256LL
#define ORCH_BUFMETA_SIZE               128LL
#define ORCH_PAGE_SIZE                  ((int64_t) 1 << BW_4KiB)
#define ORCH_BLOCK_SIZE                 ((int64_t) 1 << BW_32KiB)

// block bitwidth
#define ORCH_BLOCK_BW                   BW_32KiB
#define ORCH_PAGE_BW                    BW_4KiB

#define ORCH_ALLOC_PROT_SIZE            64

#define ORCH_DAX_ALLOC_SIZE             ((uint64_t)0x01 << 41)

// address namespace
#define OFFSET_START                   ((uint64_t) 0)                                      // 0B
#define OFFSET_SUPER_BLK               ((uint64_t) 0)                                      // 0B
#define OFFSET_INODE_BMP               ((uint64_t) 4 << 10)                                // 4KiB
#define OFFSET_IDX_BMP                 ((uint64_t) 1 << 20)                                // 1MiB
#define OFFSET_VIRND_BMP               ((uint64_t) 5 << 20)                                // 5MiB
#define OFFSET_BUFMETA_BMP             ((uint64_t) 8 << 20)                                // 8MiB
#define OFFSET_PAGE_BMP                ((uint64_t) 12 << 20)                               // 12MiB
#define OFFSET_BLOCK_BMP               ((uint64_t) 40 << 20)                               // 40MB
#define OFFSET_LOG                     ((uint64_t) 100 << 20)                              // 100MiB
#define OFFSET_INODE                   ((uint64_t) 1 << 30)                                // 1GiB
#define OFFSET_INDEX                   ((uint64_t) 2 << 30)                                // 2GiB
#define OFFSET_VIRND                   ((uint64_t) 5 << 30)                                // 5GiB
#define OFFSET_BUFMETA                 ((uint64_t) 7 << 30)                                // 7GiB
#define OFFSET_PAGE                    ((uint64_t) 10 << 30)                               // 10GiB
#define OFFSET_BLOCK                   ((uint64_t) 118 << 30)                              // 118GiB
#define OFFSET_END                     ((uint64_t) 1 << 40)                                // 1TiB

// bitmap size
#define SIZE_INODE_BMP                 (OFFSET_IDX_BMP - OFFSET_INODE_BMP)
#define SIZE_IDX_BMP                   (OFFSET_VIRND_BMP - OFFSET_IDX_BMP)
#define SIZE_VIRND_BMP                 (OFFSET_BUFMETA_BMP - OFFSET_VIRND_BMP)
#define SIZE_BUFMETA_BMP               (OFFSET_PAGE_BMP - OFFSET_BUFMETA_BMP)
#define SIZE_PAGE_BMP                  (OFFSET_BLOCK_BMP - OFFSET_PAGE_BMP)
#define SIZE_BLOCK_BMP                 (OFFSET_LOG - OFFSET_BLOCK_BMP)

// max block num
#define MAX_INODE_NUM                  ((OFFSET_INDEX - OFFSET_INODE) / ORCH_INODE_SIZE)
#define MAX_INDEX_NUM                  ((OFFSET_VIRND - OFFSET_INDEX) / ORCH_IDX_SIZE)
#define MAX_VIRND_NUM                  ((OFFSET_BUFMETA - OFFSET_VIRND) / ORCH_VIRND_SIZE)
#define MAX_BUFMETA_NUM                ((OFFSET_PAGE - OFFSET_BUFMETA) / ORCH_BUFMETA_SIZE)
#define MAX_PAGE_NUM                   ((OFFSET_BLOCK - OFFSET_PAGE) / ORCH_PAGE_SIZE)
#define MAX_BLOCK_NUM                  ((OFFSET_END - OFFSET_BLOCK) / ORCH_BLOCK_SIZE)    


// device
#define ORCH_DEV_NVM_PATH               "/dev/dax"
#define ORCH_DEV_SSD_PATH               "/dev/nvme"

// threads
#define ORCH_CONFIG_NVMTHD              5
#define ORCH_CONFIG_SSDTHD              32

// SPLIT
#define ORCH_MAX_SPLIT_BLK              1             

#define ORCH_INODE_BITLOCK_SLOTS        65536
#define ORCH_INODE_RW_SLOTS			    65536
#define ORCH_MAX_NAME				    231
#define ORCH_MAGIC_NAME            	    "ComFS_v1.0"
#define ORCH_MAGIC_NUM            	    0x37356217

#define ORCH_FAILSAFE_NFRAMES		    32

// #define FILEBENCH
// #define SSD_NOT_PARALLEL
#ifdef FILEBENCH
    // pass
#else
    // #define MIGRATTE_ON
    // #define COUNT_ON
    // #define COUNT_TIME
#endif

#define PAGE_SHIFT				    12
#define PMD_SHIFT					21
#define PTRS_PER_PMD				512
#define PTRS_PER_PTE				512
#define PMD_SIZE					(((uint64_t) 0x01) << PMD_SHIFT)
#define PMD_MASK					(~(PMD_SIZE - 1))

// errors

#define	EPERM		 1	/* Operation not permitted */
#define	ENOENT		 2	/* No such file or directory */
#define	ESRCH		 3	/* No such process */
#define	EINTR		 4	/* Interrupted system call */
#define	EIO		     5	/* I/O error */
#define	ENXIO		 6	/* No such device or address */
#define	E2BIG		 7	/* Argument list too long */
#define	ENOEXEC		 8	/* Exec format error */
#define	EBADF		 9	/* Bad file number */
#define	ECHILD		10	/* No child processes */
#define	EAGAIN		11	/* Try again */
#define	ENOMEM		12	/* Out of memory */
#define	EACCES		13	/* Permission denied */
#define	EFAULT		14	/* Bad address */
#define	ENOTBLK		15	/* Block device required */
#define	EBUSY		16	/* Device or resource busy */
#define	EEXIST		17	/* File exists */
#define	EXDEV		18	/* Cross-device link */
#define	ENODEV		19	/* No such device */
#define	ENOTDIR		20	/* Not a directory */
#define	EISDIR		21	/* Is a directory */
#define	EINVAL		22	/* Invalid argument */
#define	ENFILE		23	/* File table overflow */
#define	EMFILE		24	/* Too many open files */
#define	ENOTTY		25	/* Not a typewriter */
#define	ETXTBSY		26	/* Text file busy */
#define	EFBIG		27	/* File too large */
#define	ENOSPC		28	/* No space left on device */
#define	ESPIPE		29	/* Illegal seek */
#define	EROFS		30	/* Read-only file system */
#define	EMLINK		31	/* Too many links */
#define	EPIPE		32	/* Broken pipe */
#define	EDOM		33	/* Math argument out of domain of func */
#define	ERANGE		34	/* Math result not representable */
#define COMFS_HACK

// #define DAX_DEBUGGING 1

#ifdef __cplusplus
}
#endif

#endif