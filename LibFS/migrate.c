#include "index.h"
#include "io_thdpool.h"
#include "lib_shm.h"
#include "lib_inode.h"
#include "migrate.h"
#include "meta_cache.h"
#include "runtime.h"

#include "../util/LRU.h"
#include "../config/config.h"

migrate_info_pt init_migrate_info()
{
    migrate_info_pt ret_pt = malloc(sizeof(migrate_info_t));

    ret_pt->LRU_info = create_LRU_module(DEFAULT_LRU_MAX_ITEM);


    ret_pt->migrate_num = DEFAULT_MIGRATE_NUM;
    ret_pt->all_nvm_page = MAX_PAGE_NUM;
    ret_pt->nvm_page_used = 0;            
    ret_pt->can_use_page_num = ret_pt->all_nvm_page / 100 * CAN_USE_PERCENTAGE;
    ret_pt->mig_threshold = ret_pt->can_use_page_num / 100 * MIGRATE_PERCENTAGE;

    // ret_pt->mig_threshold = 500;
    ret_pt->mig_state = SLEEP;

    ret_pt->all_mig_blk = 0;
    ret_pt->max_page_use = 0;
    fprintf(stderr,"mig_threshold %lld %lld\n",ret_pt->mig_threshold,ret_pt->migrate_num);

    return ret_pt;
}

int do_migrate_operation(migrate_info_pt mig_info)
{
    LRU_node_info_pt mig_pos_pt = get_and_eliminate_LRU_node(mig_info->LRU_info);
    // fprintf(stderr,"mig_pos_pt: %lld\n",mig_pos_pt);
    if(mig_pos_pt == NULL)
        return 0;
    int64_t mig_off = mig_pos_pt->offset;
    int64_t mig_ino = mig_pos_pt->ino_id;


    int now_fd = get_unused_fd(mig_ino, O_RDWR);
    // fprintf(stderr,"now_fd: %d %lld %lld\n",now_fd, mig_off,mig_ino);
    orch_inode_pt now_ino_pt = fd_to_inodept(now_fd);
    // orch_inode_pt now_ino_pt = inodeid_to_memaddr(mig_ino);
    // fprintf(stderr,"root_id1: %lld %lld\n",now_ino_pt,mig_ino);
    root_id_t root_id = now_ino_pt->i_idxroot;


    void* blk_data_sp = malloc(ORCH_BLOCK_SIZE);
    void* page_data_sp = malloc(ORCH_PAGE_SIZE);
    for(int i = 0; i < VLN_SLOT_SUM; i++)
    {
        read_data_from_devs(page_data_sp, ORCH_PAGE_SIZE, mig_pos_pt->nvm_page_addr[i]);
        memcpy(blk_data_sp + ORCH_PAGE_SIZE*i, page_data_sp, ORCH_PAGE_SIZE);
    }
    int64_t new_ssd_blk_id = require_ssd_block_id();
    int64_t new_ssd_addr = ssdblk_to_devaddr(new_ssd_blk_id);


    int64_t* para_sp_pt = malloc(3 * sizeof(int64_t));
	para_sp_pt[0] = SHM_WRITE; 
	para_sp_pt[1] = new_ssd_addr;
	para_sp_pt[2] = ORCH_BLOCK_SIZE;
	sendreq_by_shm(para_sp_pt, 3*sizeof(int64_t), blk_data_sp, ORCH_BLOCK_SIZE);
	free(para_sp_pt);
    // fprintf(stderr,"check_flag: %d\n",check_flag);

    // fprintf(stderr,"root_id: %d %lld %lld\n",root_id,mig_off,mig_ino);
    file_lock_rdlock(now_fd);
    lock_range_lock(root_id, mig_ino, mig_off, mig_off);

    // off_info_t blk_info = query_offset_info(root_id, mig_ino, mig_off);
    int check_flag = 1;
    // if(blk_info.ndtype != VIR_LEAF_NODE)
    //     check_flag = 0;
    // else
    // {
    //     for(int i = 0; i < VLN_SLOT_SUM; i++)
    //     {
    //         if(blk_info.nvm_page_id[i] == EMPTY_BLK_TYPE)
    //             check_flag = 0;
    //     }
    // }

    // fprintf(stderr,"check_flag: %d\n",check_flag);

    if(check_flag == 1)
    {
        change_virnd_to_ssdblk(root_id, mig_ino, mig_off, new_ssd_blk_id);

        unlock_range_lock(root_id, mig_ino, mig_off, mig_off);
        file_lock_unlock(now_fd);

        __sync_fetch_and_sub(&(mig_info->nvm_page_used), 8);

        mig_info->all_mig_blk++;
        release_fd(now_fd);
    }
    else
    {
        unlock_range_lock(root_id, mig_ino, mig_off, mig_off);
        file_lock_unlock(now_fd);
        release_fd(now_fd);
    }
    free(blk_data_sp);
    free(page_data_sp);
    return 1;
}

void* wait_and_exec_migrate(void* para_arg)
{
    mig_pth_frame_pt arg = (mig_pth_frame_pt)para_arg;
	io_pth_pool_pt pool = arg->pool;
	int nowtid = arg->tid;
    migrate_info_pt mig_info = arg->mig_info_pt;
    // fprintf(stderr,"mig_info->migrate_num: %d %lld\n",nowtid,mig_info->migrate_num);
	while(1)
	{
	    sem_wait(&(pool->migrate_sem));

        if(pool->shutdown == 1)
		{
			// printf("thd: %d end\n",nowtid);
			sem_post(&(pool->shutdown_sem));
			break;
		}
        for(int i = 0; i < mig_info->migrate_num; i++)
        {
            // fprintf(stderr,"---------do migration!-----------\n");
            int success_flag = do_migrate_operation(arg->mig_info_pt);
            // fprintf(stderr,"success_flag %d\n",success_flag);
            if(success_flag == 0)
                break;
        }
        __sync_fetch_and_and(&(mig_info->mig_state), SLEEP);
    }
}

void add_migrate_node(migrate_info_pt mig_info, struct offset_info_t* off_info, 
                    int64_t ino_id, int64_t new_page_num)
{
    int64_t key = (ino_id << 32) + off_info->offset_ans;
    LRU_node_info_t new_LRU_node;
    new_LRU_node.ino_id = ino_id;
    new_LRU_node.offset = off_info->offset_ans;
    for(int i = 0; i < VLN_SLOT_SUM; i++)
        new_LRU_node.nvm_page_addr[i] = off_info->nvm_page_id[i];
    // fprintf(stderr,"offans: %lld %lld\n",ino_id,off_info->offset_ans);
    int ret = add_LRU_node(mig_info->LRU_info, key, &new_LRU_node, sizeof(LRU_node_info_t));
    // fprintf(stderr,"ret: %d\n",ret);
    __sync_fetch_and_add(&(mig_info->nvm_page_used), new_page_num);
    if(mig_info->nvm_page_used > mig_info->max_page_use)
        mig_info->max_page_use = mig_info->nvm_page_used;

    if(mig_info->nvm_page_used > mig_info->mig_threshold)
    {
        // Exceeding the threshold, do migration
        if(__sync_fetch_and_or(&(mig_info->mig_state), DO_MIGRATE) == SLEEP)
        {
            sem_post(&(orch_io_scheduler.migrate_sem));
            // clock_gettime(CLOCK_MONOTONIC, &mig_info->last_migrate_time);
        }
    }
}

void free_migrate_info(migrate_info_pt mig_info_pt)
{
    fprintf(stderr,"all mig num: %lld\n",mig_info_pt->all_mig_blk);
    fprintf(stderr,"max page num: %lld\n",mig_info_pt->max_page_use);
    free_LRU_module(mig_info_pt->LRU_info);
    free(mig_info_pt);
}