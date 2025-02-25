#include "meta_cache.h"
#include "index.h"
#include "../util/hashtable.h"
#include "../util/orch_list.h"
#include "../config/config.h"
#include "../KernelFS/device.h"

#ifdef __cplusplus       
extern "C"{
#endif


void init_metadata_cache()
{
    cache_data[INODE_CACHE].seg_blknum = INODE_CACHE_EXTBLKS;
    cache_data[INODE_CACHE].unit_size = ORCH_INODE_SIZE;
    cache_data[INODE_CACHE].seg_num = MAX_INODE_NUM / INODE_CACHE_EXTBLKS;
    int64_t this_seg_num = cache_data[INODE_CACHE].seg_num;
    cache_data[INODE_CACHE].sync_flist = malloc(sizeof(int32_t) * this_seg_num);
    cache_data[INODE_CACHE].meta_memsp_pt = malloc(sizeof(char*) * this_seg_num);
    cache_data[INODE_CACHE].seg_lock_pt = malloc(sizeof(pthread_spinlock_t) * this_seg_num);
    for(int64_t i = 0; i < this_seg_num; i++)
    {
        cache_data[INODE_CACHE].meta_memsp_pt[i] = NULL;
        cache_data[INODE_CACHE].sync_flist[i] = 0;
        pthread_spin_init(cache_data[INODE_CACHE].seg_lock_pt + i, PTHREAD_PROCESS_SHARED);
    }

    cache_data[IDXND_CACHE].seg_blknum = IDXND_CACHE_EXTBLKS;
    cache_data[IDXND_CACHE].unit_size = ORCH_IDX_SIZE;
    cache_data[IDXND_CACHE].seg_num = MAX_INDEX_NUM / IDXND_CACHE_EXTBLKS;
    this_seg_num = cache_data[IDXND_CACHE].seg_num;
    cache_data[IDXND_CACHE].sync_flist = malloc(sizeof(int32_t) * this_seg_num);
    cache_data[IDXND_CACHE].meta_memsp_pt = malloc(sizeof(char*) * this_seg_num);
    cache_data[IDXND_CACHE].seg_lock_pt = malloc(sizeof(pthread_spinlock_t) * this_seg_num);
    for(int64_t i = 0; i < this_seg_num; i++)
    {
        cache_data[IDXND_CACHE].meta_memsp_pt[i] = NULL;
        cache_data[IDXND_CACHE].sync_flist[i] = 0;
        pthread_spin_init(cache_data[IDXND_CACHE].seg_lock_pt + i, PTHREAD_PROCESS_SHARED);
    }
    // fprintf(stderr,"index create!\n");

    cache_data[VIRND_CACHE].seg_blknum = VIRND_CACHE_EXTBLKS;
    cache_data[VIRND_CACHE].unit_size = ORCH_VIRND_SIZE;
    cache_data[VIRND_CACHE].seg_num = MAX_VIRND_NUM / VIRND_CACHE_EXTBLKS;
    this_seg_num = cache_data[VIRND_CACHE].seg_num;
    cache_data[VIRND_CACHE].sync_flist = malloc(sizeof(int32_t) * this_seg_num);
    cache_data[VIRND_CACHE].meta_memsp_pt = malloc(sizeof(char*) * this_seg_num);
    cache_data[VIRND_CACHE].seg_lock_pt = malloc(sizeof(pthread_spinlock_t) * this_seg_num);
    for(int64_t i = 0; i < this_seg_num; i++)
    {
        cache_data[VIRND_CACHE].meta_memsp_pt[i] = NULL;
        cache_data[VIRND_CACHE].sync_flist[i] = 0;
        pthread_spin_init(cache_data[VIRND_CACHE].seg_lock_pt + i, PTHREAD_PROCESS_SHARED);
    }
    // fprintf(stderr,"virnd create!\n");
    return;
// mem_error:
//     printf("malloc error! -- init_metadata_memory\n");
//     exit(0);
}

void sync_inode(int64_t inode_id)
{
    int64_t seg_id = inode_id / INODE_CACHE_EXTBLKS;
    int64_t seg_pos = inode_id % INODE_CACHE_EXTBLKS;
    assert(seg_id < cache_data[INODE_CACHE].seg_num);
    pthread_spin_lock(cache_data[INODE_CACHE].seg_lock_pt + seg_id);
    int64_t wback_len = ORCH_INODE_SIZE * INODE_CACHE_EXTBLKS;
    int64_t wdev_addr = OFFSET_INODE + seg_id*wback_len + seg_pos*ORCH_INODE_SIZE;
    int64_t wmem_addr = (int64_t)cache_data[INODE_CACHE].meta_memsp_pt[seg_id] + seg_pos*ORCH_INODE_SIZE;
    write_data_to_devs((void*)wmem_addr, ORCH_INODE_SIZE, wdev_addr);
    pthread_spin_unlock(cache_data[INODE_CACHE].seg_lock_pt + seg_id);
}

void sync_index_blk(int64_t indexid)
{
    int64_t seg_id = indexid / IDXND_CACHE_EXTBLKS;
    int64_t seg_pos = indexid % IDXND_CACHE_EXTBLKS;
    assert(seg_id < cache_data[IDXND_CACHE].seg_num);
    pthread_spin_lock(cache_data[IDXND_CACHE].seg_lock_pt + seg_id);
    int64_t wback_len = ORCH_IDX_SIZE * IDXND_CACHE_EXTBLKS;
    int64_t wdev_addr = OFFSET_INDEX + seg_id*wback_len + seg_pos*ORCH_IDX_SIZE;
    int64_t wmem_addr = (int64_t)cache_data[IDXND_CACHE].meta_memsp_pt[seg_id] + seg_pos*ORCH_IDX_SIZE;
    write_data_to_devs((void*)wmem_addr, ORCH_IDX_SIZE, wdev_addr);
    pthread_spin_unlock(cache_data[IDXND_CACHE].seg_lock_pt + seg_id);
    // if(indexid == 0)
    // {
    //     idx_root_pt sync_root_pt1 = wmem_addr;
    //     // printf("check info: %lld %lld %lld %lld\n", seg_id, seg_pos, wmem_addr, 
    //     //     sync_root_pt1->idx_entry_blkid);
    // }
}

void sync_virnd_blk(int64_t virnodeid)
{
    int64_t seg_id = virnodeid / VIRND_CACHE_EXTBLKS;
    int64_t seg_pos = virnodeid % VIRND_CACHE_EXTBLKS;
    assert(seg_id < cache_data[VIRND_CACHE].seg_num);
    pthread_spin_lock(cache_data[VIRND_CACHE].seg_lock_pt + seg_id);
    int64_t wback_len = ORCH_VIRND_SIZE * VIRND_CACHE_EXTBLKS;
    int64_t wdev_addr = OFFSET_VIRND+ seg_id*wback_len + seg_pos*ORCH_VIRND_SIZE;
    int64_t wmem_addr = (int64_t)cache_data[VIRND_CACHE].meta_memsp_pt[seg_id] + seg_pos*ORCH_VIRND_SIZE;
    write_data_to_devs((void*)wmem_addr, ORCH_VIRND_SIZE, wdev_addr);
    pthread_spin_unlock(cache_data[VIRND_CACHE].seg_lock_pt + seg_id);
}


void close_metadata_cache()
{
    for(int i = MIN_CACHE_ID; i <= MAX_CACHE_ID; i++)
    {
        int64_t this_seg_num = cache_data[i].seg_num;
        // fprintf(stderr,"begin %d %lld\n",i,this_seg_num);
        for(int64_t j = 0; j < this_seg_num; j++)
        {
            // fprintf(stderr,"idx %d\n",j);
            pthread_spin_lock(cache_data[i].seg_lock_pt + j);
            if(cache_data[i].meta_memsp_pt[j] != NULL)
            {
                // 写回数据
                int64_t wback_len = 0, wdev_addr = 0;
                if(i == INODE_CACHE)
                {
                    wback_len = ORCH_INODE_SIZE * INODE_CACHE_EXTBLKS;
                    wdev_addr = OFFSET_INODE+ j*wback_len;
                }
                else if(i == IDXND_CACHE)
                {
                    wback_len = ORCH_IDX_SIZE * IDXND_CACHE_EXTBLKS;
                    wdev_addr = OFFSET_INDEX+ j*wback_len;
                }
                else if(i == VIRND_CACHE)
                {
                    wback_len = ORCH_VIRND_SIZE * VIRND_CACHE_EXTBLKS;
                    wdev_addr = OFFSET_VIRND+ j*wback_len;
                }
                // printf("waddr: %lld %lld\n",wback_len, wdev_addr);
                write_data_to_devs(cache_data[i].meta_memsp_pt[j], wback_len, wdev_addr);
                free(cache_data[i].meta_memsp_pt[j]);
                cache_data[i].sync_flist[j] = 0;
            }
            pthread_spin_unlock(cache_data[i].seg_lock_pt + j);
            pthread_spin_destroy(cache_data[i].seg_lock_pt + j);
        }
        free(cache_data[i].sync_flist);
        // fprintf(stderr,"lock %d %lld %lld\n",i,this_seg_num,cache_data[i].seg_lock_pt);
        if(cache_data[i].seg_lock_pt != NULL)
            free((void*)cache_data[i].seg_lock_pt);
        else
        {
            fprintf(stderr,"lock structure error1!\n");
            exit(0);
        }
        // fprintf(stderr,"%d %"PRId64" %"PRId64"\n",i,this_seg_num,cache_data[i].meta_memsp_pt);
        if(cache_data[i].meta_memsp_pt != NULL)
            free(cache_data[i].meta_memsp_pt);
        else
        {
            fprintf(stderr,"lock structure error3!\n");
            exit(0);
        }
    }
    return;
}


void create_file_metadata_cache(int64_t inode_id)
{
    int64_t seg_id = inode_id / INODE_CACHE_EXTBLKS;
    // int64_t seg_pos = inode_id % INODE_CACHE_EXTBLKS;
    assert(seg_id < cache_data[INODE_CACHE].seg_num);
    pthread_spin_lock(cache_data[INODE_CACHE].seg_lock_pt + seg_id);
    if(cache_data[INODE_CACHE].meta_memsp_pt[seg_id] == NULL)
    {
        int64_t malloc_size = cache_data[INODE_CACHE].unit_size * cache_data[INODE_CACHE].seg_blknum;
        cache_data[INODE_CACHE].meta_memsp_pt[seg_id] = malloc(malloc_size);
        if(cache_data[INODE_CACHE].meta_memsp_pt[seg_id] == NULL)
            goto malloc_failed;
       
        int64_t rback_len = ORCH_INODE_SIZE * INODE_CACHE_EXTBLKS;
        int64_t rdev_addr = OFFSET_INODE + seg_id*rback_len;
        // printf("addr0: %lld\n",rdev_addr);
        read_data_from_devs(cache_data[INODE_CACHE].meta_memsp_pt[seg_id], rback_len, rdev_addr);
    }
    pthread_spin_unlock(cache_data[INODE_CACHE].seg_lock_pt + seg_id);
    // fprintf(stderr,"addr: %lld\n",cache_data[INODE_CACHE].meta_memsp_pt[seg_id]);
    // void* target_pt = cache_data[INODE_CACHE].meta_memsp_pt[seg_id] + seg_pos*ORCH_INODE_SIZE;
    cache_data[INODE_CACHE].sync_flist[seg_id] = 1;
    // memset(target_pt, 0x00, ORCH_INODE_SIZE);
    return;

malloc_failed:
    printf("malloc failed!\n");
    exit(1);
}


void delete_file_metadata_cache(int64_t inode_id)
{
    return;
}

void close_file_metadata_cache(int64_t inode_id)
{
    return;
}


void* inodeid_to_memaddr(int64_t inodeid)
{
    int64_t seg_id = inodeid / INODE_CACHE_EXTBLKS;
    int64_t seg_pos = inodeid % INODE_CACHE_EXTBLKS;
    assert(seg_id < cache_data[INODE_CACHE].seg_num);
    pthread_spin_lock(cache_data[INODE_CACHE].seg_lock_pt + seg_id);
    if(cache_data[INODE_CACHE].meta_memsp_pt[seg_id] == NULL)
    {
        goto error;
    }
    pthread_spin_unlock(cache_data[INODE_CACHE].seg_lock_pt + seg_id);
    // fprintf(stderr,"addr1: %lld\n",cache_data[INODE_CACHE].meta_memsp_pt[seg_id]);
    void* target_pt = cache_data[INODE_CACHE].meta_memsp_pt[seg_id] + seg_pos*ORCH_INODE_SIZE;
    cache_data[INODE_CACHE].sync_flist[seg_id] = 1;
    // fprintf(stderr,"addr2: %lld\n",target_pt);
    return target_pt;
error:
    pthread_spin_unlock(cache_data[INODE_CACHE].seg_lock_pt + seg_id);
    printf("inode does not exist! -- inodeid_to_memaddr\n");
    exit(0);
}


void* indexid_to_memaddr(int64_t inodeid, int64_t indexid, int create_flag)
{
    int64_t seg_id = indexid / IDXND_CACHE_EXTBLKS;
    int64_t seg_pos = indexid % IDXND_CACHE_EXTBLKS;
    if(seg_id >= cache_data[IDXND_CACHE].seg_num)
    {
        printf("seg_id: %"PRId64" %"PRId64" %"PRId64"\n",indexid ,inodeid, cache_data[IDXND_CACHE].seg_num);
        fflush(stdout);
        goto id_error;
    }
    pthread_spin_lock(cache_data[IDXND_CACHE].seg_lock_pt + seg_id);
    if(cache_data[IDXND_CACHE].meta_memsp_pt[seg_id] == NULL)
    {
        if(create_flag == CREATE)
        {
            int64_t malloc_size = cache_data[IDXND_CACHE].unit_size * cache_data[IDXND_CACHE].seg_blknum;
            cache_data[IDXND_CACHE].meta_memsp_pt[seg_id] = malloc(malloc_size);
            if(cache_data[IDXND_CACHE].meta_memsp_pt[seg_id] == NULL)
                goto malloc_failed;
            
            int64_t rback_len = ORCH_IDX_SIZE * IDXND_CACHE_EXTBLKS;
            int64_t rdev_addr = OFFSET_INDEX + seg_id*rback_len;
            // printf("read: %lld %lld %lld\n",cache_data[IDXND_CACHE].meta_memsp_pt[seg_id], rdev_addr, rback_len);
            read_data_from_devs(cache_data[IDXND_CACHE].meta_memsp_pt[seg_id], rback_len, rdev_addr);
        }
        else
            goto error;
    }
    pthread_spin_unlock(cache_data[IDXND_CACHE].seg_lock_pt + seg_id);
    void* target_pt = cache_data[IDXND_CACHE].meta_memsp_pt[seg_id] + seg_pos*ORCH_IDX_SIZE;
    cache_data[IDXND_CACHE].sync_flist[seg_id] = 1;
    return target_pt;
error:
    pthread_spin_unlock(cache_data[IDXND_CACHE].seg_lock_pt + seg_id);
    printf("inode does not exist! -- indexid_to_memaddr\n");
    exit(0);
malloc_failed:
    printf("malloc failed!\n");
    exit(1);
id_error:
    printf("seg_id is error!\n");
    exit(1);
}


void* virnodeid_to_memaddr(int64_t inodeid, int64_t virnodeid, int create_flag)
{
    int64_t seg_id = virnodeid / VIRND_CACHE_EXTBLKS;
    int64_t seg_pos = virnodeid % VIRND_CACHE_EXTBLKS;
    assert(seg_id < cache_data[VIRND_CACHE].seg_num);
    pthread_spin_lock(cache_data[VIRND_CACHE].seg_lock_pt + seg_id);
    if(cache_data[VIRND_CACHE].meta_memsp_pt[seg_id] == NULL)
    {
        if(create_flag == CREATE)
        {
            int64_t malloc_size = cache_data[VIRND_CACHE].unit_size * cache_data[VIRND_CACHE].seg_blknum;
            cache_data[VIRND_CACHE].meta_memsp_pt[seg_id] = malloc(malloc_size);
            if(cache_data[VIRND_CACHE].meta_memsp_pt[seg_id] == NULL)
                goto malloc_failed;
            
            int64_t rback_len = ORCH_VIRND_SIZE * VIRND_CACHE_EXTBLKS;
            int64_t rdev_addr = OFFSET_VIRND+ seg_id*rback_len;
            read_data_from_devs(cache_data[VIRND_CACHE].meta_memsp_pt[seg_id], rback_len, rdev_addr);
        }
        else
            goto error;
    }
    pthread_spin_unlock(cache_data[VIRND_CACHE].seg_lock_pt + seg_id);
    void* target_pt = cache_data[VIRND_CACHE].meta_memsp_pt[seg_id] + seg_pos*ORCH_VIRND_SIZE;
    cache_data[VIRND_CACHE].sync_flist[seg_id] = 1;
    return target_pt;
error:
    pthread_spin_unlock(cache_data[VIRND_CACHE].seg_lock_pt + seg_id);
    printf("inode does not exist! -- virnodeid_to_memaddr\n");
    exit(0);
malloc_failed:
    printf("malloc failed!\n");
    exit(1);
}

int64_t bufmetaid_to_devaddr(int64_t buf_meta_id)
{
    if(buf_meta_id < 0 || buf_meta_id >= MAX_BUFMETA_NUM)
        goto buf_meta_id_error;
    int64_t ret_addr = buf_meta_id * ORCH_BUFMETA_SIZE + OFFSET_BUFMETA;
    return ret_addr;
buf_meta_id_error:
    printf("buf meta id: %" PRId64" is error!\n", buf_meta_id);
    exit(1);
}

int64_t nvmpage_to_devaddr(int64_t nvm_page_id)
{
    if(nvm_page_id < 0 || nvm_page_id >= MAX_PAGE_NUM)
        goto nvm_page_id_error;
    int64_t ret_addr = nvm_page_id * ORCH_PAGE_SIZE + OFFSET_PAGE;
    return ret_addr;
nvm_page_id_error:
    printf("nvm page id: %" PRId64" is error!\n", nvm_page_id);
    exit(1);
}

int64_t ssdblk_to_devaddr(int64_t ssd_blk_id)
{
    if(ssd_blk_id < 0 || ssd_blk_id >= MAX_BLOCK_NUM)
        goto ssd_blk_id_error;
    int64_t ret_addr = ssd_blk_id * ORCH_BLOCK_SIZE + OFFSET_BLOCK;
    return ret_addr;
ssd_blk_id_error:
    printf("ssd block id: %" PRId64" is error!\n", ssd_blk_id);
    exit(1);
}

#ifdef __cplusplus
}
#endif