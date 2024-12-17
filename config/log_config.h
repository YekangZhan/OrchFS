#ifndef LOG_CONFIG_H
#define LOG_CONFIG_H

#ifdef __cplusplus       
extern "C"{
#endif

// operation
#define CREATE_OP                    0x0100
#define DELETE_OP                    0x0200
#define CHANGE_OP                    0x0300

// add index
#define MIN_BLKTYPE_OP               0x0000
#define INODE_OP                     0x0000
#define IDXND_OP                     0x0001
#define VIRND_OP                     0x0002
#define BUFMETA_OP                   0x0003
#define PAGE_OP                      0x0004
#define BLOCK_OP                     0x0005
#define MAX_BLKTYPE_OP               0x0005


#define LOG_HEAD                     0xffff
#define LOG_END                      0x0f0f

#define LOG_SEGMENT_SIZE             (1024*1024)
#define LOG_META_SIZE                512
#define MAX_OPEN_FILE                1024

struct metadata_log_t
{
    int64_t space_blkid;
    void* changed_data;
    int32_t change_start_off;
    int32_t change_len;
};
typedef struct metadata_log_t metadata_log_t;
typedef metadata_log_t* metadata_log_pt;

#ifdef __cplusplus
}
#endif

#endif