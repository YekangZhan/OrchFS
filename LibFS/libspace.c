#include "libspace.h"
#include "lib_socket.h"
#include "req_kernel.h"
#include "runtime.h"

#include "../config/config.h"

#ifdef __cplusplus       
extern "C"{
#endif


int apply_lib_alloc_ext(int ext_type)
{
    if(ext_type < MIN_EXT_ID || ext_type > MAX_EXT_ID)
        goto type_error;
    
    if(lib_alloc_info[ext_type].blk_id_arr == NULL)
        lib_alloc_info[ext_type].blk_id_arr = malloc(ext_size_arr[ext_type] * sizeof(int64_t));
    if(lib_alloc_info[ext_type].blk_used_flag_arr == NULL)
        lib_alloc_info[ext_type].blk_used_flag_arr = malloc(ext_size_arr[ext_type] * sizeof(int32_t));
    if(lib_alloc_info[ext_type].blk_id_arr == NULL || lib_alloc_info[ext_type].blk_used_flag_arr == NULL)
        goto alloc_error;
    
   
    lib_alloc_info[ext_type].ext_blk_capacity = ext_size_arr[ext_type];
    lib_alloc_info[ext_type].used_blk_num = 0;
    lib_alloc_info[ext_type].blk_type = ext_type;
    lib_alloc_info[ext_type].last_cur = -1;
    
    
    memset(lib_alloc_info[ext_type].blk_used_flag_arr, 0x00, ext_size_arr[ext_type] * sizeof(int32_t));

    
    void* ret_info_buf = malloc(1024*1024);
    // printf("ext_type: %d\n",ext_type);

    if(ext_type == INODE_EXT)
        request_inode_id_arr(ret_info_buf, MAX_INODE_EXT_SIZE, RET_BLK_ID);
    else if(ext_type == IDXND_EXT)
        request_idxnd_id_arr(ret_info_buf, MAX_IDXND_EXT_SIZE, RET_BLK_ID);
    else if(ext_type == VIRND_EXT)
        request_virnd_id_arr(ret_info_buf, MAX_VIRND_EXT_SIZE, RET_BLK_ID);
    else if(ext_type == BUFMETA_EXT)
        request_bufmeta_id_arr(ret_info_buf, MAX_BUFMETA_EXT_SIZE, RET_BLK_ID);
    else if(ext_type == PAGE_EXT)
        request_page_id_arr(ret_info_buf, MAX_PAGE_EXT_SIZE, RET_BLK_ID);
    else if(ext_type == BLOCK_EXT)
        request_block_id_arr(ret_info_buf, MAX_BLOCK_EXT_SIZE, RET_BLK_ID);
    
    int64_t* ret_info_int64_pt = (int64_t*)ret_info_buf;
    int alloced_blknum = ret_info_int64_pt[1];

    // printf("\n");
    // printf("%d %d\n",ext_type,ret_info_int64_pt[2]);
    for(int i = 0; i < alloced_blknum; i++)
    {
        assert(i < ext_size_arr[ext_type]);
        lib_alloc_info[ext_type].blk_id_arr[i] = ret_info_int64_pt[i+2];
        // printf("%d ",ret_info_int64_pt[i+2]);
        if(i >= 1)
        {
            if(ret_info_int64_pt[i+2] != ret_info_int64_pt[i+1] + 1)
            {
                fprintf(stderr,"lib alloc blk id error! --- %d %d\n",alloced_blknum,ext_type);
                for(int j = 0; j < alloced_blknum; j++)
                {
                    fprintf(stderr,"%"PRId64" ",ret_info_int64_pt[j+2]);
                }
                fprintf(stderr,"\n");
                exit(1);
            }
        }
    }
    // printf("\n");
    free(ret_info_buf);
    return 0;
type_error:
    printf("extent type error!\n");
    return -1;
alloc_error:
    printf("alloc memory error!\n");
    return -1;
}


int dealloc_block_to_dev(int ext_type)
{
    if(ext_type < MIN_EXT_ID || ext_type > MAX_EXT_ID)
        goto type_error;
    void* send_data = (void*)lib_dealloc_info[ext_type].blk_id_arr;
    
    int blk_num = lib_dealloc_info[ext_type].dealloc_blk_num;
    if(lib_dealloc_info[ext_type].dealloc_blk_num == 0)
        return 0;
    // printf("ext_type1: %d %lld\n",ext_type,lib_dealloc_info[ext_type].dealloc_blk_num);
    \
    if(ext_type == INODE_EXT)
        send_dealloc_inode_req(send_data, blk_num, PAR_BLK_ID);
    else if(ext_type == IDXND_EXT)
        send_dealloc_idxnd_req(send_data, blk_num, PAR_BLK_ID);
    else if(ext_type == VIRND_EXT)
        send_dealloc_virnd_req(send_data, blk_num, PAR_BLK_ID);
    else if(ext_type == BUFMETA_EXT)
        send_dealloc_bufmeta_req(send_data, blk_num, PAR_BLK_ID);
    else if(ext_type == PAGE_EXT)
        send_dealloc_page_req(send_data, blk_num, PAR_BLK_ID);
    else if(ext_type == BLOCK_EXT)
        send_dealloc_block_req(send_data, blk_num, PAR_BLK_ID);
    lib_dealloc_info[ext_type].dealloc_blk_num = 0;
    return 0;
type_error:
    printf("extent type error!\n");
    return -1;
}


int apply_lib_dealloc_ext(int ext_type)
{
    if(ext_type < MIN_EXT_ID || ext_type > MAX_EXT_ID)
        goto type_error;
    if(lib_dealloc_info[ext_type].blk_id_arr != NULL)
        free(lib_dealloc_info[ext_type].blk_id_arr);
    
    lib_dealloc_info[ext_type].ext_blk_capacity = ext_size_arr[ext_type];
    lib_dealloc_info[ext_type].dealloc_blk_num = 0;
    lib_dealloc_info[ext_type].blk_type = ext_type;
    lib_dealloc_info[ext_type].blk_id_arr = malloc(ext_size_arr[ext_type] * sizeof(int64_t));
    if(lib_dealloc_info[ext_type].blk_id_arr == NULL)
        goto alloc_error;
    return 0;
type_error:
    printf("extent type error!\n");
    return -1;
alloc_error:
    printf("alloc memory error!\n");
    return -1;
}

int64_t find_block_from_ext(int ext_type)
{
    alloc_ext_info_pt alloc_info_pt = &(lib_alloc_info[ext_type]);
    // printf("aa: %lld %lld\n",alloc_info_pt->used_blk_num,alloc_info_pt->ext_blk_capacity);
    if(alloc_info_pt->used_blk_num < alloc_info_pt->ext_blk_capacity)
    {
        alloc_info_pt->last_cur += 1;
        alloc_info_pt->used_blk_num += 1;
        alloc_info_pt->blk_used_flag_arr[alloc_info_pt->last_cur] = 1;
        // if(alloc_info_pt->blk_id_arr[alloc_info_pt->last_cur] == 0)
        // {
        //     printf("cur: %" PRId64 "\n",alloc_info_pt->last_cur);
        //     for(int i = 0; i < ext_size_arr[ext_type]; i++)
        //         printf("%" PRId64 "  ",alloc_info_pt->blk_id_arr[i]);
        //     printf("\n");
        // }
        // printf("%lld %lld\n",ext_type,alloc_info_pt->blk_id_arr[alloc_info_pt->last_cur]);
        return alloc_info_pt->blk_id_arr[alloc_info_pt->last_cur];
    }
    // not found, need to apply for a new space
    return -1;
}


int64_t add_block_to_dealloc_ext(int ext_type, int64_t dealloc_blk_id)
{
    dealloc_ext_info_pt dealloc_info_pt = &(lib_dealloc_info[ext_type]);
    // printf("%lld %lld, ",dealloc_info_pt->dealloc_blk_num,dealloc_info_pt->ext_blk_capacity);
    if(dealloc_info_pt->dealloc_blk_num < MAX_DEALLOC_BLK)
    {
        dealloc_info_pt->blk_id_arr[dealloc_info_pt->dealloc_blk_num] = dealloc_blk_id;
        dealloc_info_pt->dealloc_blk_num += 1;
        return 0;
    }
    return -1;
}

/* Request a specific type of block and perform space retrieval processing
 */
int64_t require_block(int ext_type)
{
    int64_t ret_blk_info = find_block_from_ext(ext_type);
    // printf("ret_blk_info: %lld\n",ret_blk_info);
    if(ret_blk_info == -1)
    {
        int ret = apply_lib_alloc_ext(ext_type);
        if(ret == -1)
            goto error;
        ret_blk_info = find_block_from_ext(ext_type);
        if(ret_blk_info == -1)
            goto error;
        return ret_blk_info;
    }
    else
        return ret_blk_info;
error:
    printf("require error! type: %d\n",ext_type);
    exit(0);
}

/* Declare a block number to be reclaimed in user space
 */
void add_dealloc_block(int ext_type, int64_t dealloc_blk_id)
{
    int64_t ret_blk_info = add_block_to_dealloc_ext(ext_type, dealloc_blk_id);

    if(ret_blk_info == -1)
    {
        // printf("pre extid1: %d\n",ext_type);
        int ret = dealloc_block_to_dev(ext_type);
        if(ret == -1)
            goto error;
        ret_blk_info = add_block_to_dealloc_ext(ext_type, dealloc_blk_id);
        if(ret_blk_info == -1)
            goto error;
    }
    return;
error:
    printf("dealloc error! type: %d\n",ext_type);
    exit(0);
}

int init_all_ext()
{
    ext_size_arr[INODE_EXT] = MAX_INODE_EXT_SIZE;
    ext_size_arr[IDXND_EXT] = MAX_IDXND_EXT_SIZE;
    ext_size_arr[VIRND_EXT] = MAX_VIRND_EXT_SIZE;
    ext_size_arr[BUFMETA_EXT] = MAX_BUFMETA_EXT_SIZE;
    ext_size_arr[PAGE_EXT] = MAX_PAGE_EXT_SIZE;
    ext_size_arr[BLOCK_EXT] = MAX_BLOCK_EXT_SIZE;

    for(int i = MIN_EXT_ID; i <= MAX_EXT_ID; i++)
    {
        pthread_spin_init(&(lib_alloc_info[i].alloc_lock), PTHREAD_PROCESS_SHARED);
        int error_info = apply_lib_alloc_ext(i);
        if(error_info != 0)
        {
            printf("alloc extent add error! type: %d\n",i);
            exit(0);
        }
    }
    for(int i = MIN_EXT_ID; i <= MAX_EXT_ID; i++)
    {
        pthread_spin_init(&(lib_dealloc_info[i].dealloc_lock), PTHREAD_PROCESS_SHARED);
        int error_info = apply_lib_dealloc_ext(i);
        if(error_info != 0)
        {
            printf("dealloc extent add error! type: %d\n",i);
            exit(0);
        }
    }
    return 0;
}

int return_all_ext()
{
    fprintf(stderr,"ret ext!\n");
    for(int i = MIN_EXT_ID; i <= MAX_EXT_ID; i++)
        pthread_spin_lock(&(lib_dealloc_info[i].dealloc_lock));
    for(int i = MIN_EXT_ID; i <= MAX_EXT_ID; i++)
    {
        int64_t start_cur = lib_alloc_info[i].last_cur+1;
        for(int64_t j = start_cur; j < lib_alloc_info[i].ext_blk_capacity; j++)
        {
            add_dealloc_block(i, lib_alloc_info[i].blk_id_arr[j]);
        }
    }
    for(int i = MIN_EXT_ID; i <= MAX_EXT_ID; i++)
    {
        // printf("pre extid: %d\n",i);
        if(lib_dealloc_info->dealloc_blk_num == 0)
            continue;
        int error_info = dealloc_block_to_dev(i);
        if(error_info != 0)
        {
            printf("extent delete error! type: %d\n",i);
            return -1;
        }
    }
    for(int i = MIN_EXT_ID; i <= MAX_EXT_ID; i++)
        pthread_spin_unlock(&(lib_dealloc_info[i].dealloc_lock));
    
    for(int i = MIN_EXT_ID; i <= MAX_EXT_ID; i++)
    {
        pthread_spin_destroy(&(lib_alloc_info[i].alloc_lock));
        pthread_spin_destroy(&(lib_dealloc_info[i].dealloc_lock));
    }
    fprintf(stderr,"ret ext end!\n");
    return 0;
}

int64_t require_inode_id()
{
    pthread_spin_lock(&(lib_alloc_info[INODE_EXT].alloc_lock));
    int64_t ret_blkid = require_block(INODE_EXT);
    pthread_spin_unlock(&(lib_alloc_info[INODE_EXT].alloc_lock));
    return ret_blkid;
}

int64_t require_index_node_id()
{
// #ifdef COUNT_ON
//     orch_rt.used_tree_nodes += 1;
// #endif
    pthread_spin_lock(&(lib_alloc_info[IDXND_EXT].alloc_lock));
    int64_t ret_blkid = require_block(IDXND_EXT);
    pthread_spin_unlock(&(lib_alloc_info[IDXND_EXT].alloc_lock));
    return ret_blkid;
}

int64_t require_virindex_node_id()
{
// #ifdef COUNT_ON
//     orch_rt.used_vir_nodes += 1;
// #endif
    pthread_spin_lock(&(lib_alloc_info[VIRND_EXT].alloc_lock));
    int64_t ret_blkid = require_block(VIRND_EXT);
    pthread_spin_unlock(&(lib_alloc_info[VIRND_EXT].alloc_lock));
    return ret_blkid;
}

int64_t require_buffer_metadata_id()
{
// #ifdef COUNT_ON
//     orch_rt.used_pm_units += 1;
// #endif
    pthread_spin_lock(&(lib_alloc_info[BUFMETA_EXT].alloc_lock));
    int64_t ret_blkid = require_block(BUFMETA_EXT);
    pthread_spin_unlock(&(lib_alloc_info[BUFMETA_EXT].alloc_lock));
    return ret_blkid;
}

int64_t require_nvm_page_id()
{
// #ifdef COUNT_ON
//     orch_rt.used_pm_pages += 1;
// #endif
    pthread_spin_lock(&(lib_alloc_info[PAGE_EXT].alloc_lock));
    int64_t ret_blkid = require_block(PAGE_EXT);
    // if(ret_blkid < 10)
    //     fprintf(stderr, "pageid: %" PRId64 " \n",ret_blkid);
    pthread_spin_unlock(&(lib_alloc_info[PAGE_EXT].alloc_lock));
    return ret_blkid;
}

int64_t require_ssd_block_id()
{
// #ifdef COUNT_ON
//     orch_rt.used_ssd_blks += 1;
// #endif
    pthread_spin_lock(&(lib_alloc_info[BLOCK_EXT].alloc_lock));
    int64_t ret_blkid = require_block(BLOCK_EXT);
    pthread_spin_unlock(&(lib_alloc_info[BLOCK_EXT].alloc_lock));
    return ret_blkid;
}

void release_inode(int64_t inode_id)
{
    pthread_spin_lock(&(lib_dealloc_info[INODE_EXT].dealloc_lock));
    add_dealloc_block(INODE_EXT, inode_id);
    pthread_spin_unlock(&(lib_dealloc_info[INODE_EXT].dealloc_lock));
}

void release_index_node(int64_t idx_id)
{
    pthread_spin_lock(&(lib_dealloc_info[IDXND_EXT].dealloc_lock));
    add_dealloc_block(IDXND_EXT, idx_id);
    pthread_spin_unlock(&(lib_dealloc_info[IDXND_EXT].dealloc_lock));
}

void release_virindex_node(int64_t virnd_id)
{
    pthread_spin_lock(&(lib_dealloc_info[VIRND_EXT].dealloc_lock));
    add_dealloc_block(VIRND_EXT, virnd_id);
    pthread_spin_unlock(&(lib_dealloc_info[VIRND_EXT].dealloc_lock));
}

void release_buffer_metadata(int64_t buf_id)
{
    pthread_spin_lock(&(lib_dealloc_info[BUFMETA_EXT].dealloc_lock));
    add_dealloc_block(BUFMETA_EXT, buf_id);
    pthread_spin_unlock(&(lib_dealloc_info[BUFMETA_EXT].dealloc_lock));
}

void release_nvm_page(int64_t page_id)
{
    pthread_spin_lock(&(lib_dealloc_info[PAGE_EXT].dealloc_lock));
    add_dealloc_block(PAGE_EXT, page_id);
    pthread_spin_unlock(&(lib_dealloc_info[PAGE_EXT].dealloc_lock));
}

void release_ssd_block(int64_t block_id)
{
    pthread_spin_lock(&(lib_dealloc_info[BLOCK_EXT].dealloc_lock));
    add_dealloc_block(BLOCK_EXT, block_id);
    pthread_spin_unlock(&(lib_dealloc_info[BLOCK_EXT].dealloc_lock));
}

#ifdef __cplusplus
}
#endif