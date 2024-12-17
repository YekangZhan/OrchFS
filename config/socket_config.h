#ifndef SOC_CONFIG_H
#define SOC_CONFIG_H

#ifdef __cplusplus       
extern "C"{
#endif

/*The interaction between KernelFS and LibFS depends on the socket. 
  The interaction between KernelFS and LibFS is similar to a cross media 
  file system called Strata (SOSP 17).*/

// kernelFS receive port
#define ALLOC_OP_PORT                  40010
#define DEALLOC_OP_PORT                40011
#define LOG_OP_PORT                    40012
#define CACHE_OP_PORT                  40013
#define REGISTER_PORT                  40014

#define CLOSE_FS_PORT                  30000
#define CLOSE_FS_MESSAGE               0x3678268   

// libFS receive port
#define RECV_BLK_PORT                  30110
#define RECV_LOG_SP_PORT               30210
#define RECV_CACHE_PORT                30310
#define RECV_PID_PORT                  30510
#define LIB_RECV_POER_NUM              4

// port type
#define KERNEL_LISTEN_PORT             0
#define LIB_LISTEN_PORT                1
#define KERNEL_SEND_PORT               0
#define LIB_SEND_PORT                  1

// work flag
#define CAN_WORK                       0
#define CAN_NOT_WORK                   1 

// IP
#define LOCAL_IP                       "127.0.0.1" 

// max buffer len
#define MAX_BUFFER_LEN                 (1024*1024)

#define SOC_INODE_TYPE                 1
#define SOC_IDX_NODE_TYPE              2
#define SOC_VIR_NODE_TYPE              3
#define SOC_BUFMETA_TYPE               4
#define SOC_PAGE_TYPE                  5
#define SOC_BLOCK_TYPE                 6
#define SOC_LOG_TYPE                   7

// end message
#define END_MESSAGE                    0xf832ea4
#define END_MESSAGE_SEG_NUM            8
#define END_MESSAGE_LEN                32

#ifdef __cplusplus
}
#endif

#endif