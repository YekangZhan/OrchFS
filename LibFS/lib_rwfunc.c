#include "runtime.h"
#include "index.h"
#include "meta_cache.h"
#include "libspace.h"
#include "lib_log.h"
#include "lib_inode.h"
#include "orchfs.h"
#include "io_thdpool.h"
#include "migrate.h"

#include "../config/config.h"
#include "../KernelFS/device.h"


#ifdef SSD_NOT_PARALLEL
    #define MAX_SPLIT_SSDBLK                 128
#else
    #define MAX_SPLIT_SSDBLK                 ORCH_MAX_SPLIT_BLK
#endif

#define STRATA_THRESHOLD                 6

void strata_write_whole_blk(off_info_t blk_info, int64_t write_pos, int64_t write_len, 
                        void* write_buf, io_task_pt* ssd_task_end_pt)
{
    // orch_rt.blk_time++;
    if(write_pos==0 && write_len==ORCH_BLOCK_SIZE)
    {
        int64_t start_page_id = (write_pos >> ORCH_PAGE_BW);
        int64_t end_page_id = ((write_pos+write_len-1) >> ORCH_PAGE_BW);
        int16_t* bufmeta_info = malloc(ORCH_BUFMETA_SIZE);
        for(int i = start_page_id; i <= end_page_id; i++)
        {
            // read_data_from_devs(bufmeta_info, ORCH_BUFMETA_SIZE, blk_info.buf_meta_id[i]);
            bufmeta_info[0] = 0;
            write_data_to_devs(bufmeta_info, ORCH_BUFMETA_SIZE, blk_info.buf_meta_id[i]);
        }


        io_task_pt now_task_pt = *ssd_task_end_pt;
        now_task_pt->next = malloc(sizeof(io_task_t));
        now_task_pt->next->next = NULL;
        *ssd_task_end_pt = now_task_pt->next;
            

        now_task_pt->io_start_addr = blk_info.ssd_dev_addr;
        now_task_pt->io_type = WRITE_OP | STRATA_OP;
        now_task_pt->io_len = ORCH_BLOCK_SIZE;
        now_task_pt->io_data_buf = write_buf;

        // write_data_to_devs(write_buf, ORCH_BLOCK_SIZE, blk_info.ssd_dev_addr);
        free(bufmeta_info);
        return;
    }
    else
        goto para_error;
para_error:
    fprintf(stderr,"parameter error!\n");
    exit(0);
}

/* Write operation to buffer
 */
void strata_write(off_info_t blk_info, int64_t write_pos, int64_t write_len, void* write_buf,
                    io_task_pt* ssd_task_end_pt, io_task_pt* nvm_task_end_pt)
{
// #ifdef COUNT_TIME
//     struct timespec time_start;
// 	struct timespec time_end;
// #endif
    
    // printf("write: %lld %lld\n",write_pos,write_len);
    int64_t start_page_id = (write_pos >> ORCH_PAGE_BW);
    int64_t end_page_id = ((write_pos+write_len-1) >> ORCH_PAGE_BW);


    int64_t now_page_pos = (write_pos & ((1 << ORCH_PAGE_BW)-1) );
    int64_t need_write_len = write_len;
    int64_t now_wbuf_addr = (int64_t)write_buf;

    int16_t* bufmeta_info = malloc(ORCH_BUFMETA_SIZE);
    // printf("write: %lld %lld\n",start_page_id,end_page_id);

    int blk_4k_num = end_page_id-start_page_id+1, ssd_write_flag = 0;
    int64_t ssd_start_page_id = start_page_id;
    int64_t ssd_end_page_id = end_page_id;
    if(now_page_pos > 0)
        blk_4k_num -= 1, ssd_start_page_id++;
    if((write_pos+write_len-1) % ORCH_PAGE_SIZE < ORCH_PAGE_SIZE - 1)
        blk_4k_num -= 1, ssd_end_page_id--;
    if(blk_4k_num >= STRATA_THRESHOLD)
    {
        ssd_write_flag = 1;
        void* now_wbuf_pt1 = (void*)(now_wbuf_addr + ORCH_PAGE_SIZE - now_page_pos);

        io_task_pt now_task_pt = *ssd_task_end_pt;
        now_task_pt->next = malloc(sizeof(io_task_t));
        now_task_pt->next->next = NULL;
        *ssd_task_end_pt = now_task_pt->next;
            

        now_task_pt->io_start_addr = blk_info.ssd_dev_addr + ORCH_PAGE_SIZE*ssd_start_page_id;
        now_task_pt->io_type = WRITE_OP;
        now_task_pt->io_len = ORCH_PAGE_SIZE*blk_4k_num;
        now_task_pt->io_data_buf = now_wbuf_pt1;
        // write_data_to_devs(now_wbuf_pt1, ORCH_PAGE_SIZE*blk_4k_num, blk_info.ssd_dev_addr + ORCH_PAGE_SIZE*ssd_start_page_id);
    }

    for(int64_t i = start_page_id; i <= end_page_id; i++)
    {
        if(blk_info.nvm_page_id[i] == EMPTY_BLKID)
            goto strata_node_error;
        // printf("read1: %" PRId64" %" PRId64" %" PRId64"\n", bufmeta_info, ORCH_BUFMETA_SIZE, blk_info.buf_meta_id[i]);
        read_data_from_devs(bufmeta_info, ORCH_BUFMETA_SIZE, blk_info.buf_meta_id[i]);
        int16_t buf_num = bufmeta_info[0];
        if(now_page_pos == 0 && need_write_len >= ORCH_PAGE_SIZE)
        {
            if(ssd_write_flag == 0)
            {
                bufmeta_info[0] = 1;
                bufmeta_info[1] = 0;
                bufmeta_info[2] = ORCH_PAGE_SIZE;

                void* now_wbuf_pt = (void*)now_wbuf_addr;

                io_task_pt now_task_pt = *nvm_task_end_pt;
                now_task_pt->next = malloc(sizeof(io_task_t));
                now_task_pt->next->next = NULL;
                *nvm_task_end_pt = now_task_pt->next;
                    
                now_task_pt->io_start_addr = blk_info.nvm_page_id[i];
                now_task_pt->io_type = WRITE_OP | STRATA_OP;
                now_task_pt->io_len = ORCH_PAGE_SIZE;
                now_task_pt->io_data_buf = now_wbuf_pt;
                // write_data_to_devs(now_wbuf_pt, ORCH_PAGE_SIZE, blk_info.nvm_page_id[i]);
                write_data_to_devs(bufmeta_info, ORCH_BUFMETA_SIZE, blk_info.buf_meta_id[i]);
            }
            else if(ssd_write_flag == 1)
            {
                bufmeta_info[0] = 0;
                write_data_to_devs(bufmeta_info, ORCH_BUFMETA_SIZE, blk_info.buf_meta_id[i]);
            }
            need_write_len -= ORCH_PAGE_SIZE;
            now_wbuf_addr += ORCH_PAGE_SIZE;
        }
        else
        {
            if(bufmeta_info[0] > MAX_BUF_SEG_NUM)
                goto buffer_meta_out_range;
            else if(bufmeta_info[0] == MAX_BUF_SEG_NUM)
            {
// #ifdef COUNT_TIME
//                 clock_gettime(CLOCK_MONOTONIC, &time_start);
// #endif
                // 所有的缓存元数据槽位都用完了，需要清理
                void* page_buf = malloc(ORCH_PAGE_SIZE);
                void* combine_buf = malloc(ORCH_BLOCK_SIZE);
                read_data_from_devs(page_buf, ORCH_PAGE_SIZE, blk_info.nvm_page_id[i]);
                read_data_from_devs(combine_buf + i*ORCH_PAGE_SIZE, ORCH_PAGE_SIZE, blk_info.ssd_dev_addr + i*ORCH_PAGE_SIZE);

                for(int j = 1; j <= bufmeta_info[0]*2; j+=2)
                {
                    int16_t buf_start = bufmeta_info[j];
                    int16_t buf_len = bufmeta_info[j + 1];
                    memcpy(combine_buf + i*ORCH_PAGE_SIZE + buf_start, page_buf + buf_start, buf_len);
                }

                write_data_to_devs(combine_buf + i*ORCH_PAGE_SIZE, ORCH_PAGE_SIZE, blk_info.ssd_dev_addr + i*ORCH_PAGE_SIZE);
                free(page_buf);
                free(combine_buf);
                bufmeta_info[0] = 0;
// #ifdef COUNT_ON
//                 orch_rt.wback_num += 1;
// #endif
// #ifdef COUNT_TIME
//                 clock_gettime(CLOCK_MONOTONIC, &time_end);
//                 long time2 = calc_diff(time_start, time_end);
//                 orch_rt.wback_time += time2;
// #endif
            }

            bufmeta_info[0] += 1;
            int64_t now_write_size = ORCH_PAGE_SIZE - now_page_pos;
            if(need_write_len < now_write_size)
                now_write_size = need_write_len;
            bufmeta_info[bufmeta_info[0] * 2 - 1] = now_page_pos;
            bufmeta_info[bufmeta_info[0] * 2] = now_write_size;

            void* now_wbuf_pt = (void*)now_wbuf_addr;

            io_task_pt now_task_pt = *nvm_task_end_pt;
            now_task_pt->next = malloc(sizeof(io_task_t));
            now_task_pt->next->next = NULL;
            *nvm_task_end_pt = now_task_pt->next;
                    
            now_task_pt->io_start_addr = blk_info.nvm_page_id[i] + now_page_pos;
            now_task_pt->io_type = WRITE_OP | STRATA_OP;
            now_task_pt->io_len = now_write_size;
            now_task_pt->io_data_buf = now_wbuf_pt;

            // write_data_to_devs(now_wbuf_pt, now_write_size, blk_info.nvm_page_id[i] + now_page_pos);
            write_data_to_devs(bufmeta_info, ORCH_BUFMETA_SIZE, blk_info.buf_meta_id[i]);
            need_write_len -= now_write_size;
            now_wbuf_addr += now_write_size;
        }
        now_page_pos = 0;
    }
    free(bufmeta_info);
    // printf("strata write end\n");
    return;
buffer_meta_out_range:
    printf("The buffer num is too big! -- write\n");
    exit(1);
strata_node_error:
    printf("The structure of strata node is error!\n");
}


void read_strata_info(off_info_t blk_info, int64_t read_pos, int64_t read_len, void* read_buf)
{
    void* combine_buf = malloc(ORCH_BLOCK_SIZE);
    int16_t* bufmeta_info = malloc(ORCH_BUFMETA_SIZE);
    void* page_buf = malloc(ORCH_PAGE_SIZE);
    read_data_from_devs(combine_buf, ORCH_BLOCK_SIZE, blk_info.ssd_dev_addr);
    // printf("ssd addr: %lld\n",blk_info.ssd_dev_addr);

    int64_t start_page_id = (read_pos >> ORCH_PAGE_BW);
    int64_t end_page_id = ((read_pos+read_len-1) >> ORCH_PAGE_BW);
    int64_t now_page_pos = (read_pos & ((1 << ORCH_PAGE_BW)-1) );
    for(int64_t i = start_page_id; i <= end_page_id; i++)
    {
        if(blk_info.nvm_page_id[i] == EMPTY_BLKID)
            continue;
        // printf("read0: %" PRId64" %" PRId64" %" PRId64"\n", bufmeta_info, ORCH_BUFMETA_SIZE, blk_info.buf_meta_id[i]);
        read_data_from_devs(bufmeta_info, ORCH_BUFMETA_SIZE, blk_info.buf_meta_id[i]);
        int16_t buf_num = bufmeta_info[0];
        if(buf_num > MAX_BUF_SEG_NUM)
            goto buffer_meta_out_range;
        read_data_from_devs(page_buf, ORCH_PAGE_SIZE, blk_info.nvm_page_id[i]);
        for(int j = 1; j <= buf_num*2; j+=2)
        {
            int16_t buf_start = bufmeta_info[j];
            int16_t buf_len = bufmeta_info[j + 1];
            // printf("meta info: %d %d %d %lld\n",buf_num,buf_start,buf_len,blk_info.buf_meta_id[i]);
            memcpy(combine_buf + i*ORCH_PAGE_SIZE + buf_start, page_buf + buf_start, buf_len);
        }
        now_page_pos = 0;
    }

    memcpy(read_buf, combine_buf+read_pos, read_len);
    free(combine_buf);
    free(bufmeta_info);
    free(page_buf);
    // printf("strata read end\n");
    return;
buffer_meta_out_range:
    printf("The buffer num is too big! -- read\n");
    exit(1);
}


void create_strata_structure(root_id_t root_id, int64_t ino_id, off_info_pt blk_info_pt, 
                            int64_t blk_offset, int64_t start_pos, int64_t len)
{
    int64_t start_page_id = (start_pos >> ORCH_PAGE_BW);
    int64_t end_page_id = ((start_pos + len - 1) >> ORCH_PAGE_BW);
    // printf("start end: %lld %lld %lld %lld\n",start_page_id,end_page_id,blk_offset,blk_info_pt->ssd_dev_addr);

    if(blk_info_pt->ndtype == SSD_BLOCK)
        blk_info_pt->ndtype = STRATA_NODE;
    else if(blk_info_pt->ndtype == VIR_LEAF_NODE)
        goto blk_type_error;

    for(int64_t i = start_page_id; i <= end_page_id; i++)
    {
        if(blk_info_pt->nvm_page_id[i] == EMPTY_BLKID)
        {
            int64_t new_nvm_page_id = require_nvm_page_id();
            int64_t bufmeta_id = require_buffer_metadata_id();
            insert_strata_page_and_metabuf(root_id, ino_id, blk_offset, i, new_nvm_page_id, bufmeta_id);
            blk_info_pt->nvm_page_id[i] = nvmpage_to_devaddr(new_nvm_page_id);
            blk_info_pt->buf_meta_id[i] = bufmetaid_to_devaddr(bufmeta_id);
            // printf("addr1: %lld     ",blk_info_pt->buf_meta_id[i]);

            int16_t* init_bufmeta_data = malloc(ORCH_BUFMETA_SIZE);
            init_bufmeta_data[0] = 0;
            write_data_to_devs(init_bufmeta_data, ORCH_BUFMETA_SIZE, blk_info_pt->buf_meta_id[i]);
            free(init_bufmeta_data);
        }
    }
    return;
blk_type_error:
    printf("The block can not change to strata node");
    exit(1);
}


void aligned_ssd_op(off_info_pt blk_info_pt, int64_t begin_idx, int64_t end_idx,
                    io_task_pt* task_end_pt, void* buf, int io_type)
{
    int64_t now_start_addr = blk_info_pt[begin_idx].ssd_dev_addr;
    int64_t now_io_len = ORCH_BLOCK_SIZE;
    int64_t now_buf_addr = (int64_t)buf;
    int blk_count = 1;
    // printf("range: %lld %lld\n",begin_idx,end_idx);
    for(int64_t i = begin_idx + 1; i <= end_idx + 1; i++)
    {
        if(i == end_idx+1 || blk_count >= MAX_SPLIT_SSDBLK || 
            blk_info_pt[i].ssd_dev_addr != blk_info_pt[i-1].ssd_dev_addr+ORCH_BLOCK_SIZE)
        {
            io_task_pt now_task_pt = *task_end_pt;
            now_task_pt->next = malloc(sizeof(io_task_t));
            now_task_pt->next->next = NULL;
            *task_end_pt = now_task_pt->next;
            
            now_task_pt->io_start_addr = now_start_addr;
            now_task_pt->io_type = io_type;
            now_task_pt->io_len = now_io_len;
            now_task_pt->io_data_buf = (void*)now_buf_addr;
            
            now_start_addr = blk_info_pt[i].ssd_dev_addr;
            now_buf_addr += now_task_pt->io_len;
            now_io_len = ORCH_BLOCK_SIZE;
            blk_count = 1;
        }
        else
        {
            now_io_len += ORCH_BLOCK_SIZE;
            blk_count++;
        }
    }
    return;
}


void unaligned_nvm_op(off_info_t blk_info, int64_t blk_pos, int64_t io_len, 
                        io_task_pt* task_end_pt, void* buf, int io_type)
{
    int64_t start_page_off = (blk_pos >> ORCH_PAGE_BW);
    int64_t now_page_pos = (blk_pos & ((1LL<<ORCH_PAGE_BW)-1));

    int64_t now_page_off = start_page_off;
    int64_t now_buf_addr = (int64_t)buf;

    while(io_len > 0)
    {
        io_task_pt now_task_pt = *task_end_pt;
        now_task_pt->next = malloc(sizeof(io_task_t));
        now_task_pt->next->next = NULL;
        *task_end_pt = now_task_pt->next;


        now_task_pt->io_start_addr = blk_info.nvm_page_id[now_page_off] + now_page_pos;
        now_task_pt->io_type = io_type;
        if(io_len >= ORCH_PAGE_SIZE - now_page_pos)
            now_task_pt->io_len = ORCH_PAGE_SIZE - now_page_pos;
        else
            now_task_pt->io_len = io_len;
        now_task_pt->io_data_buf = (void*)now_buf_addr;


        now_page_off++;
        io_len -= now_task_pt->io_len;
        now_buf_addr += now_task_pt->io_len;
        now_page_pos = 0;
    }
    return;
}


void aligned_nvm_op(off_info_t blk_info, io_task_pt* task_end_pt, void* buf, int io_type)
{
    int64_t now_buf_addr = (int64_t)buf;

    for(int i = 0; i < VLN_SLOT_SUM; i++)
    {
        io_task_pt now_task_pt = *task_end_pt;
        now_task_pt->next = malloc(sizeof(io_task_t));
        now_task_pt->next->next = NULL;
        *task_end_pt = now_task_pt->next;

        now_task_pt->io_start_addr = blk_info.nvm_page_id[i];
        now_task_pt->io_type = io_type;
        now_task_pt->io_len = ORCH_PAGE_SIZE;
        now_task_pt->io_data_buf = (void*)now_buf_addr;

        now_buf_addr += now_task_pt->io_len;
    }
    return;
}


void all_unaligned_op(off_info_t blk_info, int64_t blk_pos, int64_t io_len, io_task_pt* ssd_task_end_pt,
                        io_task_pt* nvm_task_end_pt, void* buf, int io_type)
{
    if(blk_info.ndtype == SSD_BLOCK)
    {
        if(io_type == WRITE_OP)
            goto blk_info_error;
    
        io_task_pt now_task_pt = *ssd_task_end_pt;
        now_task_pt->next = malloc(sizeof(io_task_t));
        now_task_pt->next->next = NULL;
        *ssd_task_end_pt = now_task_pt->next;

        now_task_pt->io_start_addr = blk_info.ssd_dev_addr + blk_pos;
        now_task_pt->io_type = io_type;
        now_task_pt->io_len = io_len;
        now_task_pt->io_data_buf = buf;
        // printf("task info: %lld %lld %lld\n",now_task_pt->io_start_addr,now_task_pt->io_len,now_task_pt->io_data_buf);
    }
    else if(blk_info.ndtype == VIR_LEAF_NODE)
    {
        unaligned_nvm_op(blk_info, blk_pos, io_len, nvm_task_end_pt, buf, io_type);
    }
    else if(blk_info.ndtype == STRATA_NODE)
    {
        if(io_type == WRITE_OP)
        {
            strata_write(blk_info, blk_pos, io_len, buf, ssd_task_end_pt, nvm_task_end_pt);
        }
        else if(io_type == READ_OP)
        {
            read_strata_info(blk_info, blk_pos, io_len, buf);
        }
    }
    return;
blk_info_error:
    printf("block infomation is error!\n");
    exit(1);
}

void all_aligned_op(root_id_t root_id, ino_id_t ino_id, off_info_pt blk_info_pt, int64_t begin_idx, 
                    int64_t end_idx, io_task_pt* ssd_task_end_pt, io_task_pt* nvm_task_end_pt, void* buf, int io_type)
{
    int64_t begin0 = begin_idx, end0 = -1;
    int64_t now_rbuf_addr = (int64_t)buf;
    // printf("range1: %lld %lld\n",begin_idx,end_idx);
    for(int64_t i = begin_idx + 1; i <= end_idx + 1; i++)
    {
        if(i == end_idx + 1 || blk_info_pt[i].ndtype != blk_info_pt[i-1].ndtype)
        {
            end0 = i-1;
            // printf("ndtype: %lld\n",blk_info_pt[i-1].ndtype);
            if(blk_info_pt[i-1].ndtype == SSD_BLOCK)
            {
                void* now_rbuf = (void*)now_rbuf_addr;
                aligned_ssd_op(blk_info_pt, begin0, end0, ssd_task_end_pt, now_rbuf, io_type);
                now_rbuf_addr += (end0-begin0+1) * ORCH_BLOCK_SIZE;
            }
            else if(blk_info_pt[i-1].ndtype == VIR_LEAF_NODE)
            {
                for(int64_t j = begin0; j <= end0; j++)
                {
                    void* now_rbuf = (void*)now_rbuf_addr;
                    aligned_nvm_op(blk_info_pt[j], nvm_task_end_pt, now_rbuf, io_type);
                    now_rbuf_addr += ORCH_BLOCK_SIZE;
                }
            }
            else if(blk_info_pt[i-1].ndtype == STRATA_NODE)
            {
                for(int64_t j = begin0; j <= end0; j++)
                {
                    void* now_rbuf = (void*)now_rbuf_addr;
                    if(io_type == READ_OP)
                        read_strata_info(blk_info_pt[j], 0, ORCH_BLOCK_SIZE, now_rbuf);
                    else if(io_type == WRITE_OP)
                    {
                        int64_t now_blk_off = blk_info_pt[j].offset_ans;
                        create_strata_structure(root_id, ino_id, blk_info_pt + j, now_blk_off, 0, ORCH_BLOCK_SIZE);
                        // strata_write(blk_info_pt[j], 0, ORCH_BLOCK_SIZE, now_rbuf, ssd_task_end_pt, nvm_task_end_pt);
                        strata_write_whole_blk(blk_info_pt[j], 0, ORCH_BLOCK_SIZE, now_rbuf, ssd_task_end_pt);
                    }
                    now_rbuf_addr += ORCH_BLOCK_SIZE;
                }
            }
            begin0 = i;
        }
    }
    return;
}

void append_blk(root_id_t root_id, ino_id_t ino_id, int64_t off_start, int64_t off_end, int64_t file_end_byte)
{
    int64_t need_ssd_blk = 0, need_nvm_page = 0;
    int64_t end_blk_pos = (file_end_byte & ((1LL<<ORCH_BLOCK_BW)-1));
    if((end_blk_pos+1) % ORCH_BLOCK_SIZE != 0)
    {
        need_ssd_blk = off_end - off_start;
        need_nvm_page = end_blk_pos / ORCH_PAGE_SIZE + 1;
    }
    else
        need_ssd_blk = off_end - off_start + 1;
    
    // printf("need_ssd_blk: %lld\n",need_ssd_blk);
    // printf("need_nvm_blk: %lld %lld\n", end_blk_pos, need_nvm_page);
    assert(need_ssd_blk != 0 || need_nvm_page != 0);

    if(need_ssd_blk != 0)
    {
        ssd_blk_id_t* ssd_id_arr = malloc(sizeof(ssd_blk_id_t) * need_ssd_blk);
        for(int64_t j = 0; j < need_ssd_blk; j++)
        {
            ssd_id_arr[j] = require_ssd_block_id();
            // printf("%lld ",ssd_id_arr[j]);
        }
        append_ssd_blocks(root_id, ino_id, need_ssd_blk, ssd_id_arr);
        free(ssd_id_arr);
        // printf("\n");
    }

    if(need_nvm_page != 0)
    {
        nvm_page_id_t* nvm_id_arr = malloc(sizeof(nvm_page_id_t) * need_nvm_page);
        for(int64_t j = 0; j < need_nvm_page; j++)
        {
            nvm_id_arr[j] = require_nvm_page_id();
        }
        append_nvm_pages(root_id, ino_id, need_nvm_page, nvm_id_arr);
        free(nvm_id_arr);
    }
}


void acquire_op_blk_info(root_id_t root_id, ino_id_t ino_id, off_info_pt blk_info_pt, 
                        int64_t start_blk_off, int64_t end_blk_off, int64_t file_end_byte)
{
    // printf("off range: %" PRId64" %" PRId64"\n",start_blk_off,end_blk_off);
    // void* root_pt_out = indexid_to_memaddr(ino_id, root_id, NOT_CREATE);
    // printf("out root pt: %lld\n",root_pt_out);
    for(int64_t i = start_blk_off; i <= end_blk_off; i++)
    {
        int64_t arr_idx = i - start_blk_off; 
        blk_info_pt[arr_idx] = query_offset_info(root_id, ino_id, i);
        // printf("type: %" PRId64" %" PRId64"\n",i,blk_info_pt[arr_idx].ndtype);
        if(blk_info_pt[arr_idx].ndtype == EMPTY_BLK_TYPE)
        {
            append_blk(root_id, ino_id, i, end_blk_off, file_end_byte);
            for(int64_t j = i; j <= end_blk_off; j++)
            {
                int64_t app_arr_idx = j - start_blk_off; 
                blk_info_pt[app_arr_idx] = query_offset_info(root_id, ino_id, j);
                if(blk_info_pt[app_arr_idx].ndtype == EMPTY_BLK_TYPE)
                {
                    // printf("blk type: %" PRId64" %" PRId64"\n",app_arr_idx, root_id);
                    // printf("off: %" PRId64" %" PRId64"\n",start_blk_off, end_blk_off);
                    goto index_error;
                }
            }
            break;
        }
        else if(blk_info_pt[arr_idx].ndtype == VIR_LEAF_NODE)
        {
            if(i == end_blk_off)
            {
                int64_t page_need_num = file_end_byte / ORCH_PAGE_SIZE + 1;
                
                for(int j = 0; j < page_need_num; j++)
                {
                    if(blk_info_pt[arr_idx].nvm_page_id[j] == EMPTY_BLKID)
                    {
                        int64_t nvm_page_id = require_nvm_page_id();
                        append_single_nvm_page(root_id, ino_id, nvm_page_id);
                    }
                }
                blk_info_pt[arr_idx] = query_offset_info(root_id, ino_id, i);

            #ifdef MIGRATTE_ON
                if(file_end_byte + 1 == ORCH_BLOCK_SIZE)
                    add_migrate_node(orch_rt.mig_rtinfo_pt, blk_info_pt + arr_idx, ino_id, VLN_SLOT_SUM);
            #endif
            }
            else
            {
                if(blk_info_pt[arr_idx].nvm_page_id[VLN_SLOT_SUM - 1] == EMPTY_BLKID)
                {
                    for(int j = 0; j < VLN_SLOT_SUM; j++)
                    {
                        if(blk_info_pt[arr_idx].nvm_page_id[j] == EMPTY_BLKID)
                        {
                            int64_t nvm_page_id = require_nvm_page_id();
                            append_single_nvm_page(root_id, ino_id, nvm_page_id);
                        }
                    }
                    blk_info_pt[arr_idx] = query_offset_info(root_id, ino_id, i);

                #ifdef MIGRATTE_ON
                    add_migrate_node(orch_rt.mig_rtinfo_pt, blk_info_pt + arr_idx, ino_id, VLN_SLOT_SUM);
                #endif
                
                }
                else
                {
                #ifdef MIGRATTE_ON
                    add_migrate_node(orch_rt.mig_rtinfo_pt, blk_info_pt + arr_idx, ino_id, VLN_SLOT_SUM);
                #endif
                }
            }
        }
    }
    return;
index_error:
    printf("The structure of index is error!\n");
    exit(1);
}

int count_task_num(io_task_pt task_head_pt, io_task_pt task_end_pt)
{
    int task_num = 0;
    io_task_pt now_task_pt = task_head_pt;
    while(now_task_pt != task_end_pt)
    {
        task_num ++;
        // printf("pt: %lld\n",now_task_pt);
        now_task_pt = now_task_pt->next;
    }
    return task_num;
}

/* Execute NVM tasks
 */
void do_nvm_task(io_task_pt task_head_pt, io_task_pt task_end_pt)
{
// #ifdef COUNT_TIME
//     struct timespec pm_time_start;
//     struct timespec pm_time_end;
// #endif
// #ifdef COUNT_TIME
//     clock_gettime(CLOCK_MONOTONIC, &pm_time_start);
// #endif

    io_task_pt now_task_pt = task_head_pt;
    io_pth_pool_pt pool = &orch_io_scheduler;
    while(now_task_pt != task_end_pt)
    {
        
        sem_wait(&(pool->nvm_rw_sem));
        if((now_task_pt->io_type & READ_OP) != 0)
		{
			io_task_pt nt_pt = now_task_pt;
			if(nt_pt->io_len < 0 || nt_pt->io_data_buf == NULL)
				goto cannot_read;
            // printf("nvm read: %lld %lld %lld\n",nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
			read_data_from_devs(nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
        }
		else if((now_task_pt->io_type & WRITE_OP) != 0)
		{
			io_task_pt nt_pt = now_task_pt;
			if(nt_pt->io_len < 0 || nt_pt->io_data_buf == NULL)
				goto cannot_write;
            // printf("nvm write: %lld %lld %lld\n",nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
			write_data_to_devs(nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
            if((now_task_pt->io_type & STRATA_OP) != 0)
                orch_rt.pm_unit_rwsize += nt_pt->io_len;
            else
                orch_rt.pm_page_rwsize += nt_pt->io_len;
        }
        now_task_pt = now_task_pt->next;
        sem_post(&(pool->nvm_rw_sem));
    }
// #ifdef COUNT_TIME
//     clock_gettime(CLOCK_MONOTONIC, &pm_time_end);
//     long time2 = calc_diff(pm_time_start, pm_time_end);
//     orch_rt.pm_time += time2;
// #endif
    return;
cannot_read:
	printf("parameter error -- cannot_read\n");
	exit(1);
cannot_write:
	printf("parameter error -- cannot_write\n");
	exit(1);
}

/* Execute SSD tasks
 */
void do_ssd_task(io_task_pt task_head_pt, io_task_pt task_end_pt)
{
    io_task_pt now_task_pt = task_head_pt;
    // io_pth_pool_pt pool = &orch_io_scheduler;
    while(now_task_pt != task_end_pt)
    {
        if((now_task_pt->io_type & READ_OP) != 0)
		{
			io_task_pt nt_pt = now_task_pt;
			if(nt_pt->io_len < 0 || nt_pt->io_data_buf == NULL)
				goto cannot_read;
			read_data_from_devs(nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
        }
		else if((now_task_pt->io_type & WRITE_OP) != 0)
		{
			io_task_pt nt_pt = now_task_pt;
			if(nt_pt->io_len < 0 || nt_pt->io_data_buf == NULL)
				goto cannot_write;
			write_data_to_devs(nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
        }
        now_task_pt = now_task_pt->next;
    }
    return;
cannot_read:
	printf("parameter error -- cannot_read_ssd\n");
	exit(1);
cannot_write:
	printf("parameter error -- cannot_write_ssd\n");
	exit(1);
}


void free_task_list(io_task_pt task_head_pt)
{
    while(task_head_pt != NULL)
    {
        io_task_pt free_pt = task_head_pt;
        task_head_pt = task_head_pt->next;
        free(free_pt);
    }
    task_head_pt = NULL;
}

void print_task(ino_id_t ino_id, io_task_pt task_head_pt, io_task_pt task_end_pt, int io_type)
{
    io_task_pt now_task_pt = task_head_pt;
    while(now_task_pt != task_end_pt)
    {
        io_task_pt nt = now_task_pt;
        if(io_type == READ_OP)
        {
            printf("readtask_info: %" PRId64" %" PRId64" %" PRId64" \n",
                ino_id, nt->io_start_addr, nt->io_len);
        }
        else if(io_type == WRITE_OP)
        {
            printf("writetask_info: %" PRId64" %" PRId64" %" PRId64" \n",
                ino_id, nt->io_start_addr, nt->io_len);
        }
        now_task_pt = now_task_pt->next;
    }
}


void print_blk_off(off_info_t blk_off_info[], int64_t blk_num)
{
    printf("------------------------print block off begin!--------------------\n");
    for(int i = 0; i < blk_num; i++)
    {
        printf("info: %d %" PRId64"    ",i,blk_off_info[i].ndtype);
        if(blk_off_info[i].ndtype == SSD_BLOCK)
        {
            printf("ssd: %" PRId64" \n",blk_off_info[i].ssd_dev_addr);
        }
        else if(blk_off_info[i].ndtype == VIR_LEAF_NODE)
        {
            printf("nvmpage: ");
            for(int j = 0; j < VLN_SLOT_SUM; j++)
                printf("%" PRId64" ",blk_off_info[i].nvm_page_id[j]);
            printf("\n");
        }
        else if(blk_off_info[i].ndtype == STRATA_NODE)
        {
            printf("strata-ssd: %" PRId64" \n",blk_off_info[i].ssd_dev_addr);
            printf("nvmpage: ");
            for(int j = 0; j < VLN_SLOT_SUM; j++)
                printf("%" PRId64" ",blk_off_info[i].nvm_page_id[j]);
            printf("\n");
            printf("strata meta data: ");
            for(int j = 0; j < VLN_SLOT_SUM; j++)
                printf("%" PRId64" ",blk_off_info[i].buf_meta_id[j]);
            printf("\n");
        }
    }
}

void read_from_file(int fd, int64_t read_start_byte, int64_t read_len, void* read_buf)
{
// #ifdef COUNT_TIME
//     struct timespec index_watch_start;
// 	struct timespec index_watch_end;
// #endif

// #ifdef COUNT_TIME
//     clock_gettime(CLOCK_MONOTONIC, &index_watch_start);
// #endif

    if(read_len <= 0)
        goto len_error;
    if(read_start_byte + read_len > (1LL << (KEY_LEN + ORCH_BLOCK_BW)))
        goto start_byte_error;

    int64_t read_end_byte = read_start_byte + read_len - 1;
    

    int64_t start_blk_off = (read_start_byte >> ORCH_BLOCK_BW);
    int64_t end_blk_off = (read_end_byte >> ORCH_BLOCK_BW);
    int64_t start_blk_pos = (read_start_byte & ((1LL<<ORCH_BLOCK_BW)-1));
    int64_t end_blk_pos = (read_end_byte & ((1LL<<ORCH_BLOCK_BW)-1));

    ino_id_t ino_id = fd_to_inodeid(fd);
    orch_inode_pt now_ino_pt = (orch_inode_pt)inodeid_to_memaddr(ino_id);
    root_id_t root_id = now_ino_pt->i_idxroot;

    // printf("read root id: %" PRId64" %" PRId64" \n",root_id,ino_id); 
    // fflush(stdout);

    int64_t read_blk_num = end_blk_off - start_blk_off + 1;
    off_info_pt blk_info_pt = malloc(read_blk_num * sizeof(off_info_t));


    lock_range_lock(root_id, ino_id, start_blk_off, end_blk_off);
    
    acquire_op_blk_info(root_id, ino_id, blk_info_pt, start_blk_off, end_blk_off, end_blk_pos);

    for(int64_t i = 0; i < read_blk_num; i++)
    {
        if(blk_info_pt[i].ndtype == EMPTY_BLK_TYPE)
            goto index_error;
    }

    // print_blk_off(blk_info_pt, read_blk_num);


    io_task_pt ssd_io_task_head = malloc(sizeof(io_task_t));
    io_task_pt ssd_io_task_end = ssd_io_task_head;
    ssd_io_task_head->next = NULL;
    io_task_pt nvm_io_task_head = malloc(sizeof(io_task_t));
    io_task_pt nvm_io_task_end = nvm_io_task_head;
    nvm_io_task_head->next = NULL;


    int64_t iter_start = 0, iter_end = read_blk_num-1;
    int64_t now_rbuf_addr = (int64_t)read_buf;

    // printf("pos: %lld %lld\n",start_blk_pos,end_blk_pos);

    if(start_blk_pos != 0 || (start_blk_pos == 0 && read_len < ORCH_BLOCK_SIZE))
    {
        iter_start++;
        int64_t first_blk_len = ORCH_BLOCK_SIZE - start_blk_pos;
        if(first_blk_len > read_len)
            first_blk_len = read_len;
        void* now_rbuf = (void*)now_rbuf_addr;
        all_unaligned_op(blk_info_pt[0], start_blk_pos, first_blk_len, &ssd_io_task_end, 
                            &nvm_io_task_end, now_rbuf, READ_OP);
        now_rbuf_addr += first_blk_len;
    }
    if((end_blk_pos+1) % ORCH_BLOCK_SIZE != 0)
        iter_end --;


    // struct timespec index_watch_start;
	// struct timespec index_watch_end;
    // clock_gettime(CLOCK_MONOTONIC, &index_watch_start);
    void* now_rbuf = (void*)now_rbuf_addr;
    all_aligned_op(root_id, ino_id, blk_info_pt, iter_start, iter_end, &ssd_io_task_end, &nvm_io_task_end, now_rbuf, READ_OP);
    // clock_gettime(CLOCK_MONOTONIC, &index_watch_end);
    // long time0 = calc_diff(index_watch_start, index_watch_end);
    // orch_rt.index_time += time0;

    if(iter_end >= iter_start)
        now_rbuf_addr += (iter_end-iter_start+1) * ORCH_BLOCK_SIZE;


    if(start_blk_off != end_blk_off && (end_blk_pos+1) % ORCH_BLOCK_SIZE != 0)
    {
        int64_t endblk_idx = read_blk_num - 1;
        void* now_rbuf = (void*)now_rbuf_addr;
        all_unaligned_op(blk_info_pt[endblk_idx], 0, end_blk_pos + 1, &ssd_io_task_end, 
                        &nvm_io_task_end, now_rbuf, READ_OP);
    }

    // printf("ssd task: %lld\n",read_buf);
    // print_task(ino_id, ssd_io_task_head, ssd_io_task_end, READ_OP);
    // printf("nvm task:\n");
    // print_task(ino_id, nvm_io_task_head, nvm_io_task_end);
// #ifdef COUNT_TIME
//     clock_gettime(CLOCK_MONOTONIC, &index_watch_end);
//     long time1 = calc_diff(index_watch_start, index_watch_end);
//     orch_rt.index_time += time1;
// #endif

// #ifdef COUNT_TIME
//     struct timespec io_time_start;
// 	struct timespec io_time_end;
//     clock_gettime(CLOCK_MONOTONIC, &io_time_start);
// #endif


    int ssd_task_num = count_task_num(ssd_io_task_head, ssd_io_task_end);
    int task_group_id = -1;

#ifdef SSD_NOT_PARALLEL 
    // 关闭SSD的并行
    do_nvm_task(nvm_io_task_head, nvm_io_task_end);
    do_ssd_task(ssd_io_task_head, ssd_io_task_end);
#else 
    if(ssd_task_num > 0)
        task_group_id = add_task(ssd_io_task_head, nvm_io_task_head, ssd_task_num, 0, fd);
    do_nvm_task(nvm_io_task_head, nvm_io_task_end);
    if(ssd_task_num > 0 && task_group_id != -1)
        wait_task_done(task_group_id);
#endif

// #ifdef COUNT_TIME
//     clock_gettime(CLOCK_MONOTONIC, &io_time_end);
//     long time2 = calc_diff(io_time_start, io_time_end);
//     orch_rt.io_time += time2;
// #endif


    unlock_range_lock(root_id, ino_id, start_blk_off, end_blk_off);
    free(blk_info_pt);
    free_task_list(ssd_io_task_head);
    ssd_io_task_head = ssd_io_task_end = NULL;
    free_task_list(nvm_io_task_head);
    nvm_io_task_head = nvm_io_task_end = NULL;

    return;
len_error:
    printf("read len error!\n");
    exit(1);
start_byte_error:
    printf("file start byte error!\n");
    exit(1);
index_error:
    printf("The file index is error!\n");
    exit(1);
}

void write_into_file(int fd, int64_t file_start_byte, int64_t write_len, void* write_buf)
{
// #ifdef COUNT_TIME
//     struct timespec index_watch_start;
// 	struct timespec index_watch_end;
// #endif
    
// #ifdef COUNT_TIME
//     clock_gettime(CLOCK_MONOTONIC, &index_watch_start);
// #endif

    if(write_len <= 0)
        goto len_error;
    if(file_start_byte + write_len > (1LL << (KEY_LEN + ORCH_BLOCK_BW)))
        goto start_byte_error;
    

    int64_t file_end_byte = file_start_byte + write_len - 1;


    int64_t start_blk_off = (file_start_byte >> ORCH_BLOCK_BW);
    int64_t end_blk_off = (file_end_byte >> ORCH_BLOCK_BW);
    int64_t start_blk_pos = (file_start_byte & ((1LL<<ORCH_BLOCK_BW)-1));
    int64_t end_blk_pos = (file_end_byte & ((1LL<<ORCH_BLOCK_BW)-1));


    ino_id_t ino_id = (ino_id_t)fd_to_inodeid(fd);
    orch_inode_pt now_ino_pt = (orch_inode_pt)inodeid_to_memaddr(ino_id);
    root_id_t root_id = now_ino_pt->i_idxroot;

    // printf("write root id: %" PRId64" %" PRId64" \n",root_id,ino_id); 


    lock_range_lock(root_id, ino_id, start_blk_off, end_blk_off);


    int64_t write_blk_num = end_blk_off - start_blk_off + 1;
    off_info_pt blk_info_pt = malloc(write_blk_num * sizeof(off_info_t));
    acquire_op_blk_info(root_id, ino_id, blk_info_pt, start_blk_off, end_blk_off, end_blk_pos);
    // clock_gettime(CLOCK_MONOTONIC, &index_watch_end);
    // long time0 = calc_diff(index_watch_start, index_watch_end);
    // orch_rt.index_time += time0;

    // print_blk_off(blk_info_pt, write_blk_num);
    // for(int i = 0; i < write_blk_num; i++)
    //     printf("%lld ",blk_info_pt[i].ssd_dev_addr);
    // printf("\n");

    io_task_pt ssd_io_task_head = malloc(sizeof(io_task_t));
    io_task_pt ssd_io_task_end = ssd_io_task_head;
    ssd_io_task_head->next = NULL;
    io_task_pt nvm_io_task_head = malloc(sizeof(io_task_t));
    io_task_pt nvm_io_task_end = nvm_io_task_head;
    nvm_io_task_head->next = NULL;


    // printf("blk_pos: %" PRId64" %" PRId64"\n", start_blk_pos, end_blk_pos);
    int64_t iter_start = 0, iter_end = write_blk_num - 1;
    int64_t now_wbuf_addr = (int64_t)write_buf;
    if(start_blk_pos != 0 || (start_blk_pos == 0 && write_len < ORCH_BLOCK_SIZE))
    {
        iter_start++;
        int64_t first_blk_len = ORCH_BLOCK_SIZE - start_blk_pos;
        if(first_blk_len > write_len)
            first_blk_len = write_len;
        void* now_rbuf = (void*)now_wbuf_addr;
        if(blk_info_pt[0].ndtype == SSD_BLOCK || blk_info_pt[0].ndtype == STRATA_NODE)
            create_strata_structure(root_id, ino_id, blk_info_pt, start_blk_off, start_blk_pos, first_blk_len);
        // printf("type: %d\n",blk_info_pt[0].ndtype);
        all_unaligned_op(blk_info_pt[0], start_blk_pos, first_blk_len, &ssd_io_task_end, 
                        &nvm_io_task_end, now_rbuf, WRITE_OP);
        now_wbuf_addr += first_blk_len;
    }
    if((end_blk_pos+1) % ORCH_BLOCK_SIZE != 0)
        iter_end --;
    

    // printf("iter: %" PRId64" %" PRId64"\n", iter_start, iter_end);
    void* now_wbuf = (void*)now_wbuf_addr;
    // clock_gettime(CLOCK_MONOTONIC, &index_watch_start);
    all_aligned_op(root_id, ino_id, blk_info_pt, iter_start, iter_end, &ssd_io_task_end, &nvm_io_task_end, now_wbuf, WRITE_OP);
    // clock_gettime(CLOCK_MONOTONIC, &index_watch_end);
    // long time1 = calc_diff(index_watch_start, index_watch_end);
    // orch_rt.blk_time += time1;
    if(iter_end >= iter_start)
        now_wbuf_addr += (iter_end-iter_start+1) * ORCH_BLOCK_SIZE;
    

    if(start_blk_off != end_blk_off && (end_blk_pos+1) % ORCH_BLOCK_SIZE != 0)
    {
        int64_t endblk_idx = write_blk_num - 1;
        void* now_wbuf = (void*)now_wbuf_addr;
        if(blk_info_pt[endblk_idx].ndtype == SSD_BLOCK || blk_info_pt[endblk_idx].ndtype == STRATA_NODE)
            create_strata_structure(root_id, ino_id, blk_info_pt + endblk_idx, end_blk_off, 0, end_blk_pos + 1);
        all_unaligned_op(blk_info_pt[endblk_idx], 0, end_blk_pos + 1, &ssd_io_task_end,
                         &nvm_io_task_end, now_wbuf, WRITE_OP);
    }

// #ifdef COUNT_TIME
//     clock_gettime(CLOCK_MONOTONIC, &index_watch_end);
//     long time1 = calc_diff(index_watch_start, index_watch_end);
//     orch_rt.index_time += time1;
// #endif

    // printf("ssd task:\n");
    // print_task(ino_id, ssd_io_task_head, ssd_io_task_end, WRITE_OP);
    // printf("nvm task:\n");
    // print_task(ino_id, nvm_io_task_head, nvm_io_task_end, WRITE_OP);


// #ifdef COUNT_TIME
//     struct timespec io_time_start;
// 	struct timespec io_time_end;
//     clock_gettime(CLOCK_MONOTONIC, &io_time_start);
// #endif

    int ssd_task_num = count_task_num(ssd_io_task_head, ssd_io_task_end);
    int task_group_id = -1;

#ifdef SSD_NOT_PARALLEL
    do_nvm_task(nvm_io_task_head, nvm_io_task_end);
    do_ssd_task(ssd_io_task_head, ssd_io_task_end);
#else 
    if(ssd_task_num > 0)
        task_group_id = add_task(ssd_io_task_head, nvm_io_task_head, ssd_task_num, 0, fd);
    if(ssd_task_num > 0 && task_group_id != -1)
        wait_task_done(task_group_id);

// #ifdef COUNT_TIME
    // clock_gettime(CLOCK_MONOTONIC, &io_time_end);
    // long time3 = calc_diff(io_time_start, io_time_end);
    // orch_rt.blk_time += time3;
// #endif
    
    do_nvm_task(nvm_io_task_head, nvm_io_task_end);
#endif

// #ifdef COUNT_TIME
//     clock_gettime(CLOCK_MONOTONIC, &io_time_end);
//     long time2 = calc_diff(io_time_start, io_time_end);
//     orch_rt.io_time += time2;
// #endif

    unlock_range_lock(root_id, ino_id, start_blk_off, end_blk_off);
    free(blk_info_pt);
    free_task_list(ssd_io_task_head);
    ssd_io_task_head = ssd_io_task_end = NULL;
    free_task_list(nvm_io_task_head);
    nvm_io_task_head = nvm_io_task_end = NULL;

    return;
len_error:
    printf("read len error!\n");
    exit(1);
start_byte_error:
    printf("file start byte error!\n");
    exit(1);
}

void do_ssd_task_newbaseline(io_task_pt task_head_pt, io_task_pt task_end_pt)
{
    io_task_pt now_task_pt = task_head_pt;
    while(now_task_pt != task_end_pt)
    {
        if(now_task_pt->io_type == READ_OP)
		{
			io_task_pt nt_pt = now_task_pt;
			if(nt_pt->io_len < 0 || nt_pt->io_data_buf == NULL)
				goto cannot_read;
            // printf("nvm read: %lld %lld %lld\n",nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
			read_data_from_ssds_newbaseline(nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
        }
		else if(now_task_pt->io_type == WRITE_OP)
		{
			io_task_pt nt_pt = now_task_pt;
			if(nt_pt->io_len < 0 || nt_pt->io_data_buf == NULL)
				goto cannot_write;
            // printf("nvm write: %lld %lld %lld\n",nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
			write_data_to_ssds_newbaseline(nt_pt->io_data_buf, nt_pt->io_len, nt_pt->io_start_addr);
        }
        now_task_pt = now_task_pt->next;
    }
    return;
cannot_read:
	printf("parameter error -- cannot_read\n");
	exit(1);
cannot_write:
	printf("parameter error -- cannot_write\n");
	exit(1);
}

void write_into_file_newbaseline(int fd, int64_t file_start_byte, int64_t write_len, void* write_buf)
{
    if(write_len<32*1024)
    {
        write_data_to_nvms_newbaseline(write_buf,write_len,file_start_byte);
        return ;
    }
    int64_t aligned_file_start_byte = file_start_byte - file_start_byte%(32*1024);
    int64_t aligned_write_len = write_len + file_start_byte - aligned_file_start_byte + 32*1024 - (file_start_byte+write_len)%(32*1024);

    if(aligned_write_len <= 0)
        goto len_error;
    if(aligned_file_start_byte + aligned_write_len > (1LL << (KEY_LEN + ORCH_BLOCK_BW)))
        goto start_byte_error;
    
 
    int64_t file_end_byte = aligned_file_start_byte + aligned_write_len - 1;
    

    int64_t start_blk_off = (aligned_file_start_byte >> ORCH_BLOCK_BW);
    int64_t end_blk_off = (file_end_byte >> ORCH_BLOCK_BW);
    int64_t start_blk_pos = (aligned_file_start_byte & ((1LL<<ORCH_BLOCK_BW)-1));
    int64_t end_blk_pos = (file_end_byte & ((1LL<<ORCH_BLOCK_BW)-1));
    // printf("write_blk_pos: %" PRId64" %" PRId64" \n",start_blk_pos,end_blk_pos);


    ino_id_t ino_id = (ino_id_t)fd_to_inodeid(fd);
    orch_inode_pt now_ino_pt = (orch_inode_pt)inodeid_to_memaddr(ino_id);
    root_id_t root_id = now_ino_pt->i_idxroot;

    // printf("write root id: %" PRId64" %" PRId64" \n",root_id,ino_id); 


    lock_range_lock(root_id, ino_id, start_blk_off, end_blk_off);


    int64_t write_blk_num = end_blk_off - start_blk_off + 1;
    off_info_pt blk_info_pt = malloc(write_blk_num * sizeof(off_info_t));
    acquire_op_blk_info(root_id, ino_id, blk_info_pt, start_blk_off, end_blk_off, end_blk_pos);
    // clock_gettime(CLOCK_MONOTONIC, &index_watch_end);
    // long time0 = calc_diff(index_watch_start, index_watch_end);
    // orch_rt.index_time += time0;

    // print_blk_off(blk_info_pt, write_blk_num);
    // for(int i = 0; i < write_blk_num; i++)
    //     printf("%lld ",blk_info_pt[i].ssd_dev_addr);
    // printf("\n");


    io_task_pt ssd_io_task_head = malloc(sizeof(io_task_t));
    io_task_pt ssd_io_task_end = ssd_io_task_head;
    ssd_io_task_head->next = NULL;
    io_task_pt nvm_io_task_head = malloc(sizeof(io_task_t));
    io_task_pt nvm_io_task_end = nvm_io_task_head;
    nvm_io_task_head->next = NULL;


    // printf("blk_pos: %" PRId64" %" PRId64"\n", start_blk_pos, end_blk_pos);
    int64_t iter_start = 0, iter_end = write_blk_num - 1;
    int64_t now_wbuf_addr = (int64_t)write_buf;
    if(start_blk_pos != 0 || (start_blk_pos == 0 && aligned_write_len < ORCH_BLOCK_SIZE))
    {
        iter_start++;
        int64_t first_blk_len = ORCH_BLOCK_SIZE - start_blk_pos;
        if(first_blk_len > aligned_write_len)
            first_blk_len = aligned_write_len;
        void* now_rbuf = (void*)now_wbuf_addr;
        if(blk_info_pt[0].ndtype == SSD_BLOCK || blk_info_pt[0].ndtype == STRATA_NODE)
            create_strata_structure(root_id, ino_id, blk_info_pt, start_blk_off, start_blk_pos, first_blk_len);
        // printf("type: %d\n",blk_info_pt[0].ndtype);
        all_unaligned_op(blk_info_pt[0], start_blk_pos, first_blk_len, &ssd_io_task_end, 
                        &nvm_io_task_end, now_rbuf, WRITE_OP);
        now_wbuf_addr += first_blk_len;
    }
    if((end_blk_pos+1) % ORCH_BLOCK_SIZE != 0)
        iter_end --;
    

    // printf("iter: %" PRId64" %" PRId64"\n", iter_start, iter_end);
    void* now_wbuf = (void*)now_wbuf_addr;
    // clock_gettime(CLOCK_MONOTONIC, &index_watch_start);
    all_aligned_op(root_id, ino_id, blk_info_pt, iter_start, iter_end, &ssd_io_task_end, &ssd_io_task_end, now_wbuf, WRITE_OP);
    // clock_gettime(CLOCK_MONOTONIC, &index_watch_end);
    // long time1 = calc_diff(index_watch_start, index_watch_end);
    // orch_rt.blk_time += time1;
    if(iter_end >= iter_start)
        now_wbuf_addr += (iter_end-iter_start+1) * ORCH_BLOCK_SIZE;
    

    if(start_blk_off != end_blk_off && (end_blk_pos+1) % ORCH_BLOCK_SIZE != 0)
    {
        int64_t endblk_idx = write_blk_num - 1;
        void* now_wbuf = (void*)now_wbuf_addr;
        if(blk_info_pt[endblk_idx].ndtype == SSD_BLOCK || blk_info_pt[endblk_idx].ndtype == STRATA_NODE)
            create_strata_structure(root_id, ino_id, blk_info_pt + endblk_idx, end_blk_off, 0, end_blk_pos + 1);
        all_unaligned_op(blk_info_pt[endblk_idx], 0, end_blk_pos + 1, &ssd_io_task_end,
                         &nvm_io_task_end, now_wbuf, WRITE_OP);
    }
    assert(nvm_io_task_head==nvm_io_task_end);
    io_task_pt now_task_pt = ssd_io_task_head;
    now_task_pt->io_start_addr=now_task_pt->io_start_addr+file_start_byte%(32*1024);
    now_task_pt->io_len=now_task_pt->io_len-file_start_byte%(32*1024);
    while(now_task_pt->next != ssd_io_task_end)
    {
        now_task_pt = now_task_pt->next;
    }
    now_task_pt->io_len=now_task_pt->io_len-( 32*1024 - (file_start_byte+write_len)%(32*1024));
    do_ssd_task_newbaseline(ssd_io_task_head, ssd_io_task_end);

    
    // clock_gettime(CLOCK_MONOTONIC, &index_watch_end);
    // long time2 = calc_diff(index_watch_start, index_watch_end);
    // orch_rt.exec_time += time2;


    unlock_range_lock(root_id, ino_id, start_blk_off, end_blk_off);
    free(blk_info_pt);
    free_task_list(ssd_io_task_head);
    ssd_io_task_head = ssd_io_task_end = NULL;
    free_task_list(nvm_io_task_head);
    nvm_io_task_head = nvm_io_task_end = NULL;

    return;
len_error:
    printf("read len error!\n");
    exit(1);
start_byte_error:
    printf("file start byte error!\n");
    exit(1);
}



void read_from_file_newbaseline(int fd, int64_t read_start_byte, int64_t read_len, void* read_buf)
{
    if(read_len<32*1024)
    {
        read_data_from_nvms_newbaseline(read_buf,read_len,read_start_byte);
        return ;
    }
    int64_t aligned_read_start_byte = read_start_byte - read_start_byte%(32*1024);
    int64_t aligned_read_len = read_len + read_start_byte - aligned_read_start_byte + 32*1024 - (read_start_byte+read_len)%(32*1024);

    if(aligned_read_len <= 0)
        goto len_error;
    if(aligned_read_start_byte + aligned_read_len > (1LL << (KEY_LEN + ORCH_BLOCK_BW)))
        goto start_byte_error;

    int64_t read_end_byte = aligned_read_start_byte + aligned_read_len - 1;
    
    int64_t start_blk_off = (aligned_read_start_byte >> ORCH_BLOCK_BW);
    int64_t end_blk_off = (read_end_byte >> ORCH_BLOCK_BW);
    int64_t start_blk_pos = (aligned_read_start_byte & ((1LL<<ORCH_BLOCK_BW)-1));
    int64_t end_blk_pos = (read_end_byte & ((1LL<<ORCH_BLOCK_BW)-1));


    ino_id_t ino_id = fd_to_inodeid(fd);
    orch_inode_pt now_ino_pt = (orch_inode_pt)inodeid_to_memaddr(ino_id);
    root_id_t root_id = now_ino_pt->i_idxroot;

    // printf("read root id: %" PRId64" %" PRId64" \n",root_id,ino_id); 

    int64_t read_blk_num = end_blk_off - start_blk_off + 1;
    off_info_pt blk_info_pt = malloc(read_blk_num * sizeof(off_info_t));

    lock_range_lock(root_id, ino_id, start_blk_off, end_blk_off);
    
    acquire_op_blk_info(root_id, ino_id, blk_info_pt, start_blk_off, end_blk_off, end_blk_pos);

    for(int64_t i = 0; i < read_blk_num; i++)
    {
        if(blk_info_pt[i].ndtype == EMPTY_BLK_TYPE)
            goto index_error;
    }

    // print_blk_off(blk_info_pt, read_blk_num);

    io_task_pt ssd_io_task_head = malloc(sizeof(io_task_t));
    io_task_pt ssd_io_task_end = ssd_io_task_head;
    ssd_io_task_head->next = NULL;
    io_task_pt nvm_io_task_head = malloc(sizeof(io_task_t));
    io_task_pt nvm_io_task_end = nvm_io_task_head;
    nvm_io_task_head->next = NULL;

    int64_t iter_start = 0, iter_end = read_blk_num-1;
    int64_t now_rbuf_addr = (int64_t)read_buf;

    // printf("pos: %lld %lld\n",start_blk_pos,end_blk_pos);

    if(start_blk_pos != 0 || (start_blk_pos == 0 && aligned_read_len < ORCH_BLOCK_SIZE))
    {
        iter_start++;
        int64_t first_blk_len = ORCH_BLOCK_SIZE - start_blk_pos;
        if(first_blk_len > aligned_read_len)
            first_blk_len = aligned_read_len;
        void* now_rbuf = (void*)now_rbuf_addr;
        all_unaligned_op(blk_info_pt[0], start_blk_pos, first_blk_len, &ssd_io_task_end, 
                            &nvm_io_task_end, now_rbuf, READ_OP);
        now_rbuf_addr += first_blk_len;
    }
    if((end_blk_pos+1) % ORCH_BLOCK_SIZE != 0)
        iter_end --;

    // struct timespec index_watch_start;
	// struct timespec index_watch_end;
    // clock_gettime(CLOCK_MONOTONIC, &index_watch_start);
    void* now_rbuf = (void*)now_rbuf_addr;
    all_aligned_op(root_id, ino_id, blk_info_pt, iter_start, iter_end, &ssd_io_task_end, &nvm_io_task_end, now_rbuf, READ_OP);
    // clock_gettime(CLOCK_MONOTONIC, &index_watch_end);
    // long time0 = calc_diff(index_watch_start, index_watch_end);
    // orch_rt.index_time += time0;

    if(iter_end >= iter_start)
        now_rbuf_addr += (iter_end-iter_start+1) * ORCH_BLOCK_SIZE;

    if(start_blk_off != end_blk_off && (end_blk_pos+1) % ORCH_BLOCK_SIZE != 0)
    {
        int64_t endblk_idx = read_blk_num - 1;
        void* now_rbuf = (void*)now_rbuf_addr;
        all_unaligned_op(blk_info_pt[endblk_idx], 0, end_blk_pos + 1, &ssd_io_task_end, 
                        &nvm_io_task_end, now_rbuf, READ_OP);
    }

    // printf("ssd task: %lld\n",read_buf);
    // print_task(ino_id, ssd_io_task_head, ssd_io_task_end, READ_OP);
    // printf("nvm task:\n");
    // print_task(ino_id, nvm_io_task_head, nvm_io_task_end);
    assert(nvm_io_task_head==nvm_io_task_end);

    io_task_pt now_task_pt = ssd_io_task_head;
    now_task_pt->io_start_addr=now_task_pt->io_start_addr+read_start_byte%(32*1024);
    now_task_pt->io_len=now_task_pt->io_len-read_start_byte%(32*1024);
    while(now_task_pt->next != ssd_io_task_end)
    {
        now_task_pt = now_task_pt->next;
    }
    now_task_pt->io_len=now_task_pt->io_len-( 32*1024 - (read_start_byte+read_len)%(32*1024));
    do_ssd_task_newbaseline(ssd_io_task_head, ssd_io_task_end);

    unlock_range_lock(root_id, ino_id, start_blk_off, end_blk_off);
    free(blk_info_pt);
    free_task_list(ssd_io_task_head);
    ssd_io_task_head = ssd_io_task_end = NULL;
    free_task_list(nvm_io_task_head);
    nvm_io_task_head = nvm_io_task_end = NULL;

    return;
len_error:
    printf("read len error!\n");
    exit(1);
start_byte_error:
    printf("file start byte error!\n");
    exit(1);
index_error:
    printf("The file index is error!\n");
    exit(1);
}
