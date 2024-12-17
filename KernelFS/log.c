#include "log.h"
#include "device.h"

#include "../config/config.h"
#include "../config/log_config.h"

#include "../util/hashtable.h"
#include "../util/orch_list.h"
#include "../util/orch_array.h"

#ifdef __cplusplus       
extern "C"{
#endif

/* TODO for crash consistency
 */
void digest_log(int64_t seg_id)
{
    
}

void init_kernel_log()
{
    log_seg_num = (OFFSET_INODE-OFFSET_LOG) / LOG_SEGMENT_SIZE;

    kmem_log_meta = malloc(log_seg_num * sizeof(klog_meta_t));
    for(int64_t i = 0; i < log_seg_num; i++)
    {
        kmem_log_meta[i].used_flag = 0;
        kmem_log_meta[i].klog_data_sp = NULL;
    }
    for(int i = 0; i < MAX_OPEN_FILE; i++)
        file_log_index[i] = create_array(sizeof(int64_t));
    inoid_to_flidx = init_hashtable(DEFAULT);
}

int64_t alloc_log_segment_from_dev()
{
    for(int64_t i = 0; i < log_seg_num; i++)
    {
        if(kmem_log_meta[i].used_flag == 0)
        {
            kmem_log_meta[i].used_flag = 1;
            kmem_log_meta[i].submit_end_off = -1;
            kmem_log_meta[i].sync_len = 0;
            kmem_log_meta[i].last_submit_time = 0;
            kmem_log_meta[i].not_writed_len = LOG_SEGMENT_SIZE-LOG_META_SIZE;

            int64_t dev_off = OFFSET_LOG + LOG_SEGMENT_SIZE * i;
            write_data_to_devs(kmem_log_meta + i, 5*sizeof(int64_t), dev_off);
            kmem_log_meta[i].klog_data_sp = malloc(LOG_SEGMENT_SIZE);

            return i;
        }
    }
    printf("kernel log space error!\n");
    exit(0);
}

void sync_file_log(int64_t inode_id)
{
    // void* arr_idx_sp = search_kv_from_hashtable(inodeid_to_logarridx, inode_id);
    // int64_t* arr_idx_pt = (int64_t*)arr_idx_sp;
    // int64_t arr_idx = arr_idx_pt[0];
}

void submit_part_log_segment(int64_t seg_id, int64_t start_addr, int64_t submit_len)
{
//     if(kmem_log_meta[seg_id].submit_end_off != start_addr - 1)
//         goto submit_error;
//     kmem_log_meta[seg_id].submit_end_off = start_addr + submit_len -1;
//     kmem_log_meta[seg_id].last_submit_time = 0;
//     kmem_log_meta[seg_id].not_writed_len -= submit_len;
//     int64_t dev_off = OFFSET_LOG + LOG_SEGMENT_SIZE * seg_id;
//     write_data_to_devs(kmem_log_meta + seg_id, 5*sizeof(int64_t), dev_off);
//     int64_t memlog_sp_addr = (int64_t)kmem_log_meta[seg_id].klog_data_sp + start_addr + LOG_META_SIZE;
//     void* mem_sp = (void*)memlog_sp_addr;
//     read_data_from_devs(mem_sp, submit_len, dev_off + LOG_META_SIZE + start_addr);
// submit_error:
//     printf("The range of submitted log is error!\n");
//     return;
}

void submit_log_segment(int64_t seg_id)
{
    // kmem_log_meta[seg_id].last_submit_time = 0;
    // kmem_log_meta[seg_id].clean_flag = CAN_CLEAN;
    // int64_t dev_off = OFFSET_LOG + LOG_SEGMENT_SIZE * seg_id;
    // write_data_to_devs(kmem_log_meta + seg_id, 5*sizeof(int64_t), dev_off);

    // int64_t memlog_sp_addr = (int64_t)kmem_log_meta[seg_id].klog_data_sp + 
    //                     kmem_log_meta[seg_id].submit_end_off + LOG_META_SIZE + 1;
    // void* mem_sp = (void*)memlog_sp_addr;
    // int64_t devlog_start_addr = dev_off + LOG_META_SIZE + kmem_log_meta[seg_id].submit_end_off + 1;
    // read_data_from_devs(mem_sp, kmem_log_meta[seg_id].not_writed_len, devlog_start_addr);

    // digest_log(seg_id);
}

void close_kernel_log()
{
    for(int64_t i = 0; i < log_seg_num; i++)
    {
        if(kmem_log_meta[i].used_flag == 1)
        {
            digest_log(i);
        }
        if(kmem_log_meta[i].klog_data_sp != NULL)
            free(kmem_log_meta[i].klog_data_sp);
    }
    free(kmem_log_meta);
    for(int i = 0; i < MAX_OPEN_FILE; i++)
        free_array(file_log_index[i]);
    free_hashtable(inoid_to_flidx);
}

#ifdef __cplusplus
}
#endif