#ifndef PROTOCOL_H
#define PROTOCOL_H

#ifdef __cplusplus       
extern "C"{
#endif

#define MIN_RIG_FUNC_CODE                0x0100
#define REGISTER_CODE                    0x0100
#define MAX_RIG_FUNC_CODE                0x0100

// The code infomation about allocate block
#define MIN_ALLOC_FUNC_CODE              0x0000
#define ALLOC_INODE_FUNC                 0x0000
#define ALLOC_INXND_FUNC                 0x0001
#define ALLOC_VIRND_FUNC                 0x0002
#define ALLOC_BUFMETA_FUNC               0x0003
#define ALLOC_PAGE_FUNC                  0x0004
#define ALLOC_BLOCK_FUNC                 0x0005
#define ALLOC_LOG_FUNC                   0x0006
#define MAX_ALLOC_FUNC_CODE              0x0006

// The code infomation about deallocate block
#define MIN_DEALLOC_FUNC_CODE            0x0010
#define DEALLOC_INODE_FUNC               0x0010
#define DEALLOC_INXND_FUNC               0x0011
#define DEALLOC_VIRND_FUNC               0x0012
#define DEALLOC_BUFMETA_FUNC             0x0013
#define DEALLOC_PAGE_FUNC                0x0014
#define DEALLOC_BLOCK_FUNC               0x0015
#define DEALLOC_LOG_FUNC                 0x0016
#define MAX_DEALLOC_FUNC_CODE            0x0016

// The code infomation about log
#define MIN_LOG_FUNC_CODE                0x1000
#define ALLOC_LOG_SEG_CODE               0x1000
#define MAX_LOG_FUNC_CODE                0x1000

#ifdef __cplusplus
}
#endif

#endif