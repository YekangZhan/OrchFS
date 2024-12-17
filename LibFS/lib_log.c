#include "lib_log.h"
#include "req_kernel.h"
#include "../config/log_config.h"
#include "../config/config.h"
#include "../KernelFS/device.h"

#ifdef __cplusplus       
extern "C"{
#endif

void init_mem_log()
{
    memlog_alloc_cur = 0;
    for(int i = 0; i < ORCH_MAX_FD; i++)
    {
        log_per_op[i].used_flag = LOG_SPACE_NOT_USED;
        log_per_op[i].log_len = 0;
        log_per_op[i].log_space = NULL;
        log_per_op[i].now_write_pt = NULL;
    }

    int64_t dev_logsp_id = request_log_seg();
    // printf("log id: %" PRId64" \n",dev_logsp_id);
    dev_log_info.dev_log_id = dev_logsp_id;
    int64_t dev_logsp_offset = dev_logsp_id * LOG_SEGMENT_SIZE;
    dev_log_info.dev_log_baseaddr = OFFSET_LOG + dev_logsp_offset;
    dev_log_info.dev_log_woffset = LOG_META_SIZE;
    dev_log_info.dev_log_wpt = dev_log_info.dev_log_woffset + dev_log_info.dev_log_baseaddr;
    dev_log_info.dev_log_wrange = OFFSET_LOG + dev_logsp_offset + LOG_SEGMENT_SIZE;

    pthread_spin_init(&(dev_log_info.wlog_lock), PTHREAD_PROCESS_SHARED);
}

void free_mem_log()
{

    for(int i = 0; i < ORCH_MAX_FD; i++)
    {
        if(log_per_op[i].used_flag == LOG_SPACE_USED)
        {
            // return_log_memspace(i);
        }
    }
    pthread_spin_destroy(&(dev_log_info.wlog_lock));

    // submit_log_segment(devlog_sp->dev_logseg_id);
}


void write_create_log(int64_t create_blkid, int blk_type)
{
    pthread_spin_lock(&(dev_log_info.wlog_lock));
    if(blk_type < MIN_BLKTYPE_OP || blk_type > MAX_BLKTYPE_OP)
        goto block_type_error;


    int64_t log_basic_info = (((blk_type | CREATE_OP) * 1LL) << 32);
    int64_t lsp_arr[5] = {0};
    lsp_arr[0] = log_basic_info;
    lsp_arr[1] = create_blkid;


    int add_log_len = 2 * sizeof(int64_t);
    if(dev_log_info.dev_log_wpt + add_log_len >= dev_log_info.dev_log_wrange)
    {
        dev_log_info.dev_log_wpt = dev_log_info.dev_log_woffset + dev_log_info.dev_log_baseaddr;
        // printf("pos: %lld\n", dev_log_info.dev_log_wpt);
    }
    write_data_to_devs(lsp_arr, add_log_len, dev_log_info.dev_log_wpt);
    dev_log_info.dev_log_wpt += add_log_len;
    assert(dev_log_info.dev_log_wpt <= dev_log_info.dev_log_wrange);
    pthread_spin_unlock(&(dev_log_info.wlog_lock));
    return;
block_type_error:
    pthread_spin_unlock(&(dev_log_info.wlog_lock));
    printf("input block type error! --- write_create_log\n");
    exit(1);
}


void write_delete_log(int64_t delete_blkid, int blk_type)
{
    pthread_spin_lock(&(dev_log_info.wlog_lock));
    if(blk_type < MIN_BLKTYPE_OP || blk_type > MAX_BLKTYPE_OP)
        goto block_type_error;
    

    int64_t log_basic_info = (((blk_type | DELETE_OP) * 1LL) << 32);
    int64_t lsp_arr[5] = {0};
    lsp_arr[0] = log_basic_info;
    lsp_arr[1] = delete_blkid;


    int add_log_len = 2 * sizeof(int64_t);
    if(dev_log_info.dev_log_wpt + add_log_len >= dev_log_info.dev_log_wrange)
    {
        dev_log_info.dev_log_wpt = dev_log_info.dev_log_woffset + dev_log_info.dev_log_baseaddr;
        // printf("pos: %lld\n", dev_log_info.dev_log_wpt);
    }
    write_data_to_devs(lsp_arr, add_log_len, dev_log_info.dev_log_wpt);
    dev_log_info.dev_log_wpt += add_log_len;
    // printf("pos1: %lld\n", dev_log_info.dev_log_wpt);
    assert(dev_log_info.dev_log_wpt <= dev_log_info.dev_log_wrange);
    pthread_spin_unlock(&(dev_log_info.wlog_lock));
    return;
block_type_error:
    pthread_spin_unlock(&(dev_log_info.wlog_lock));
    printf("input block type error! --- write_delete_log\n");
    exit(1);
}


void write_change_log(int64_t change_blkid, int blk_type, void* log_data, int32_t off, int32_t len)
{
    pthread_spin_lock(&(dev_log_info.wlog_lock));
    if(blk_type < MIN_BLKTYPE_OP || blk_type > MAX_BLKTYPE_OP)
        goto block_type_error;
    
    int64_t log_basic_info = (((blk_type | CHANGE_OP) * 1LL) << 32) + len;
    int64_t change_pos_info = ((off*1LL) << 32) + len;
    

    int32_t log_combine_len = 3*sizeof(int64_t) + len;
    void* log_sp_pt = malloc(log_combine_len);


    int64_t* lsp_int64pt = (int64_t*)log_sp_pt;
    lsp_int64pt[0] = log_basic_info;
    lsp_int64pt[1] = change_blkid;
    lsp_int64pt[2] = change_pos_info;
    memcpy(lsp_int64pt + 3, log_data, len);


    int add_log_len = log_combine_len;
    if(dev_log_info.dev_log_wpt + add_log_len >= dev_log_info.dev_log_wrange)
    {
        dev_log_info.dev_log_wpt = dev_log_info.dev_log_woffset + dev_log_info.dev_log_baseaddr;
        // printf("pos: %lld\n", dev_log_info.dev_log_wpt);
    }
    write_data_to_devs(log_sp_pt, add_log_len, dev_log_info.dev_log_wpt);
    dev_log_info.dev_log_wpt += add_log_len;
    assert(dev_log_info.dev_log_wpt <= dev_log_info.dev_log_wrange);
    // printf("pos2: %lld\n", dev_log_info.dev_log_wpt);
    pthread_spin_unlock(&(dev_log_info.wlog_lock));
    return;
block_type_error:
    pthread_spin_unlock(&(dev_log_info.wlog_lock));
    printf("input block type error! --- write_change_log\n");
    exit(1);
}

// /* 内部函数，给内存日志段打包，添加头部和尾部 */
// void pack_mem_log(int log_space_id)
// {
//     int lp_id = log_space_id;
//     // 添加尾部信息
//     int32_t* lsp_int32pt = (int32_t*)log_per_op[lp_id].now_write_pt;
//     lsp_int32pt[0] = LOG_END;
//     // 修改日志长度
//     log_per_op[lp_id].log_len += 4;
//     // 修改头部信息
//     int64_t new_head_info = ((1LL*LOG_HEAD) << 32) + log_per_op[lp_id].log_len;
//     int64_t* lsp_int64pt = (int64_t*)log_per_op[lp_id].log_space;
//     lsp_int64pt[0] = head_info;
// }

// void memlog_to_dev(int log_space_id, int sync_flag)
// {
//     int lp_id = log_space_id;
//     // 处理日志区间不够的情况
//     if(devlog_sp->dev_logseg_offset + log_per_op[lp_id].log_len >= LOG_SEGMENT_SIZE)
//     {
//         submit_log_segment(devlog_sp->dev_logseg_id);
//         int64_t devlog_sp_id = alloc_log_segment_from_dev();
//         devlog_sp->dev_logseg_id = devlog_sp_id;
//         int64_t devlog_sp_addr = devlog_sp_id * LOG_SEGMENT_SIZE;
//         devlog_sp->dev_logseg_baseaddr = OFFSET_LOG + devlog_sp_addr;
//         devlog_sp->dev_logseg_offset = LOG_META_SIZE;
//     }

//     // 把日志写入设备
//     pack_mem_log(log_space_id);
//     int64_t dev_addr = devlog_sp->dev_logseg_offset + devlog_sp->dev_logseg_baseaddr;
//     int64_t now_log_len = log_per_op[lp_id].log_len;
//     void* npw_lsp = devlog_sp->log_space;
//     write_data_to_devs(now_lsp, now_log_len, dev_addr);

//     // 更改设备日志保存的信息
//     devlog_sp->dev_logseg_offset += log_per_op[lp_id].log_len;
//     if(sync_flag == SYNC_LOG_MODE)
//         submit_part_log_segment(devlog_sp->dev_logseg_id, dev_addr, now_log_len);
    
//     // 清空对应的内存区域
//     log_per_op[lp_id].log_len = MEMLOG_HEAD_SIZE;
//     int64_t mem_log_addr = (int64_t)log_per_op[lp_id].log_space;
//     log_per_op[lp_id].now_write_pt = (void*)(mem_log_addr + MEMLOG_HEAD_SIZE);
// }

// int regist_log_memspace()
// {
//     int ret_logsp_id = 0;
//     for(int i = 0; i < OPERATION_NUM; i++)
//     {
//         int cur = (memlog_alloc_cur + i) % OPERATION_NUM;
//         if(log_per_op[cur].used_flag == LOG_SPACE_NOT_USED)
//         {
//             log_per_op[cur].used_flag = LOG_SPACE_USED;
//             memlog_alloc_cur = (cur + 1) % OPERATION_NUM;
//             log_per_op[cur].log_space = malloc(MEMLOG_SPACE_SIZE);
//             if(log_per_op[cur].log_space == NULL)
//                 goto mem_alloc_error;
            
//             // 将内存日志中的头写好
//             int64_t head_info = ((1LL*LOG_HEAD) << 32);
//             int64_t* mem_log_intpt = (int64_t*)log_per_op[cur].log_space;
//             mem_log_intp[0] = head_info;
//             // 更新其他内存日志结构中的信息
//             int64_t mem_log_addr = (int64_t)log_per_op[cur].log_space;
//             log_per_op[cur].now_write_pt = (void*)(mem_log_addr + MEMLOG_HEAD_SIZE);
//             log_per_op[cur].log_len = MEMLOG_HEAD_SIZE;
//             return cur;
//         }
//         cur = (cur+ + 1) % OPERATION_NUM;
//     }
// used_up:
//     printf("log space used up!\n");
//     exit(1);
// mem_alloc_error:
//     printf("memory alloc error!\n");
//     exit(1);
// }

// void return_log_memspace(int log_space_id)
// {
//     int lp_id = log_space_id;
//     if(log_per_op[lp_id].log_len > MEMLOG_HEAD_SIZE)
//     {
//         memlog_to_dev(log_space_id);
//     }
//     if(log_per_op[lp_id].log_space != NULL)
//         free(log_per_op[lp_id].log_space);
//     log_per_op[lp_id].now_write_pt = NULL;
//     log_per_op[lp_id].used_flag = LOG_SPACE_NOT_USED;
// }

// // 第一个8B为块的类型和操作类型
// // 后面8B是块号
// void write_create_log(int log_space_id, int64_t create_blkid, int blk_type)
// {
//     if(blk_type < MIN_BLKTYPE_OP || blk_type > MAX_BLKTYPE_OP)
//         goto block_type_error;
    
//     // 初始化基本信息
//     int lp_id = log_space_id;
//     int64_t lsp_addr = (int64_t)log_per_op[lp_id].now_write_pt;

//     // 写好头部和块号
//     int64_t log_basic_info = (((blk_type | CREATE_OP) * 1LL) << 32);
//     int64_t* lsp_int64pt = (int64_t*)lsp_addr;
//     lsp_int64pt[0] = log_basic_info;
//     lsp_int64pt[1] = create_blkid;

//     // 更新信息
//     int add_log_len = 2 * sizeof(int64_t);
//     log_per_op[lp_id].now_write_pt = (void*)(lsp_addr + add_log_len);
//     log_per_op[lp_id].log_len += add_log_len;
//     return;
// block_type_error:
//     printf("input block type error! --- write_create_log\n");
//     exit(1);
// }

// // 第一个8B为块的类型和操作类型
// // 后面8B是块号
// void write_delete_log(int log_space_id, int64_t delete_blkid, int blk_type)
// {
//     if(blk_type < MIN_BLKTYPE_OP || blk_type > MAX_BLKTYPE_OP)
//         goto block_type_error;
//     // 初始化基本信息
//     int lsp_id = log_space_id;
//     int64_t lsp_addr = (int64_t)log_per_op[lsp_id].now_write_pt;
//     // 写好头部和块号
//     int64_t log_basic_info = (((blk_type | DELETE_OP) * 1LL) << 32);
//     int64_t* lsp_int64pt = (int64_t*)lsp_addr;
//     lsp_int64pt[0] = log_basic_info;
//     lsp_int64pt[1] = delete_blkid;
//     // 更新信息
//     int add_log_len = 2 * sizeof(int64_t);
//     log_per_op[lsp_id].now_write_pt = (void*)(lsp_addr + add_log_len);
//     log_per_op[lsp_id].log_len += add_log_len;
//     return;
// block_type_error:
//     printf("input block type error! --- write_delete_log\n");
//     exit(1);
// }

// // 第一个8B为块的类型和操作类型
// // 第二个8B是块号
// // 第三个8B是改变的位置信息
// // 后面是改变后的信息
// void write_change_log(int log_space_id, int64_t change_blkid, int blk_type, 
//                     void* log_data, int32_t off, int32_t len)
// {
//     if(blk_type < MIN_BLKTYPE_OP || blk_type > MAX_BLKTYPE_OP)
//         goto block_type_error;
//     // 初始化基本信息
//     int lsp_id = log_space_id;
//     int64_t lsp_addr = (int64_t)log_per_op[lsp_id].now_write_pt;
//     // 写好头部和块号
//     int64_t log_basic_info = (((blk_type | CHANGE_OP) * 1LL) << 32) + len;
//     int64_t change_pos_info = ((off*1LL) << 32) + len;
//     int64_t* lsp_int64pt = (int64_t*)lsp_addr;
//     lsp_int64pt[0] = log_basic_info;
//     lsp_int64pt[1] = change_blkid;
//     lsp_int64pt[2] = change_pos_info;
//     // 更新元数据信息
//     int add_log_len = 3 * sizeof(int64_t);
//     log_per_op[lsp_id].now_write_pt = (void*)(lsp_addr + add_log_len);
//     log_per_op[lsp_id].log_len += add_log_len;
//     // 复制修改信息
//     memcpy(log_per_op[lsp_id].now_write_pt, log_data, len);
//     int64_t new_lsp_addr = (int64_t)log_per_op[lsp_id].now_write_pt;
//     log_per_op[lsp_id].now_write_pt = (void*)(new_lsp_addr + len);
//     log_per_op[lsp_id].log_len += len;
//     return;
// block_type_error:
//     printf("input block type error! --- write_change_log\n");
//     exit(1);
// }

#ifdef __cplusplus
}
#endif
