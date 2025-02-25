#include "index.h"
#include "libspace.h"
#include "meta_cache.h"
#include "lib_log.h"
#include "runtime.h"

#include "../config/config.h"
#include "../config/log_config.h"

#ifdef __cplusplus       
extern "C"{
#endif

void bitlock_acquire(bit_lock_t* bitlock, int lock_pos)
{
    uint64_t * target = bitlock + (lock_pos / 64);
    uint64_t offset = lock_pos % 64;
    while((((uint64_t)0b01 << offset) & __sync_fetch_and_or(target, (uint64_t)0b01 << offset)) != 0){
        // pthread_yield();
    }
}
void bitlock_release(bit_lock_t* bitlock, int unlock_pos)
{
    FETCH_AND_unSET_BIT(bitlock, unlock_pos);
}


void node_init(idx_nd_pt idxnd_spt, int64_t init_zipped_layer, int32_t ntype)
{
    idxnd_spt->ndtype = ntype;
    idxnd_spt->zipped_layer = init_zipped_layer;
    for(int i = 0; i < NODE_SON_CAPACITY/64; i++)
    {
        idxnd_spt->virnd_flag[i] = 0;
        idxnd_spt->bit_lock[i] = 0;
    }
    for(int i = 0; i < NODE_SON_CAPACITY; i++)
        idxnd_spt->son_blk_id[i] = EMPTY_BLKID;
}


/* Initialize an index node
 */
void vir_node_init(vir_nd_pt virnd_spt, int32_t ntype)
{
    virnd_spt->ndtype = ntype;
    virnd_spt->ssd_dev_addr = EMPTY_BLKID;
    virnd_spt->max_pos = 0;
    for(int i = 0; i < VLN_SLOT_SUM; i++)
    {
        virnd_spt->nvm_page_id[i] = EMPTY_BLKID;
        virnd_spt->buf_meta_id[i] = EMPTY_BLKID;
    }
}

idx_root_pt check_root_id(root_id_t root_id, int64_t inode_id)
{
    void* root_blk_memaddr = indexid_to_memaddr(inode_id, root_id, CREATE);
    idx_root_pt root_pt = (idx_root_pt)root_blk_memaddr;
    if(root_pt == NULL)
        goto cache_error;
    
    // if(inode_id == 0)
    // {
    //     printf("check end! %lld %lld %lld\n",inode_id, root_id, 
    //                 root_pt->idx_entry_blkid);
    // }

  
    if(root_pt->idx_entry_blkid == EMPTY_BLKID)
    {
        root_pt->idx_entry_blkid = require_index_node_id();
        if(root_pt->idx_entry_blkid != -1)
        {

            write_create_log(root_pt->idx_entry_blkid, IDXND_OP);
            void* idxnd_memaddr = indexid_to_memaddr(inode_id, root_pt->idx_entry_blkid, CREATE);
            idx_nd_pt idx_node_pt = (idx_nd_pt)idxnd_memaddr;
            if(idx_node_pt == NULL)
                goto cache_error;

            node_init(idx_node_pt, MAX_ZIPPED_LAYER, LEAF_NODE);
            write_change_log(root_pt->idx_entry_blkid, IDXND_OP, idx_node_pt, 0, ORCH_IDX_SIZE);
            return root_pt;
        }
        else
            goto alloc_error;
    }
    else
    {
        return root_pt;
    }
cache_error:
    printf("root cache create error!\n");
    exit(0);
alloc_error:
    printf("root create error!\n");
    exit(0);
}


index_id_t search_leaf_node(index_id_t now_idxnd_id, int64_t inode_id,
                            int32_t searched_len, int64_t blk_offset)
{
    if(now_idxnd_id == EMPTY_BLKID)
        return -1;

    // printf("sln: %lld %lld\n",inode_id,now_idxnd_id);
    void* idxnd_memaddr = indexid_to_memaddr(inode_id, now_idxnd_id, CREATE);
    idx_nd_pt now_idxnd = (idx_nd_pt)idxnd_memaddr;
    if(now_idxnd == NULL)
        goto cache_error;


    int32_t zipped_len = now_idxnd->zipped_layer * LAYER_BITWIDTH;

    int move_bits = KEY_LEN - searched_len - zipped_len;
    int64_t zipped_part_bit = (((1ULL << zipped_len) - 1) << move_bits);
    int64_t zipped_part = ((zipped_part_bit & blk_offset) >> move_bits);
    // printf("blk_offset: %" PRId64" %" PRId64" %" PRId64"\n",blk_offset,now_idxnd_id,move_bits);
    // printf("zip info: %" PRId64" %" PRId64"\n",zipped_len,zipped_part);

    if(zipped_part != 0)
        goto not_exist;
    else
    {
        if(now_idxnd->ndtype == LEAF_NODE)
            return now_idxnd_id;
        else
        {
            int move_bits = KEY_LEN - searched_len - zipped_len - LAYER_BITWIDTH;
            int64_t next_layer_bit = (((1ULL << LAYER_BITWIDTH) - 1) << move_bits);
            int64_t next_layer_key = ((next_layer_bit & blk_offset) >> move_bits);
            int32_t new_searched_len = searched_len + zipped_len + LAYER_BITWIDTH;
            int64_t next_layer_idxid = now_idxnd->son_blk_id[next_layer_key];

            // printf("next_info: %" PRId64" %" PRId64" %" PRId64"\n", 
            //                 next_layer_key, new_searched_len,next_layer_idxid);
            
            return search_leaf_node(next_layer_idxid, inode_id, new_searched_len, blk_offset);
        }
    }
cache_error:
    printf("root cache create error! -- search_leaf_node\n");
    exit(0);
not_exist:
    return -1;
}


int insert_node_into_index(index_id_t now_idxnd_id, int64_t inode_id, int64_t* father_pt, 
                            int32_t searched_len, uint64_t blk_offset)
{
    // printf("node_id: %" PRId64" \n",now_idxnd_id);
    void* idxnd_memaddr = indexid_to_memaddr(inode_id, now_idxnd_id, CREATE);
    idx_nd_pt now_idxnd = (idx_nd_pt)idxnd_memaddr;
    if(now_idxnd == NULL)
        goto cache_error;
    

    int32_t zipped_len = now_idxnd->zipped_layer * LAYER_BITWIDTH;
    int move_bits = KEY_LEN - searched_len - zipped_len;
    uint64_t zipped_part_bit = (((1ULL << zipped_len) - 1) << move_bits);
    uint64_t zipped_part = ((zipped_part_bit & blk_offset) >> move_bits);
    // printf("zip info0 %" PRId64" %" PRId32"\n",zipped_part,zipped_len);

    if(now_idxnd->ndtype == LEAF_NODE && zipped_part == 0)
    {
        return 0;
    }
    else if(now_idxnd->ndtype == NOT_LEAF_NODE && zipped_part == 0)
    {
        int move_bits = KEY_LEN - searched_len - zipped_len - LAYER_BITWIDTH;
        int64_t son_arridx_bit = (((1LL << LAYER_BITWIDTH) - 1) << move_bits);
        int64_t son_arridx = ((son_arridx_bit & blk_offset) >> move_bits);
        // printf("nonleaf: %" PRId64" %" PRId64" %d\n",son_arridx_bit,son_arridx,move_bits);

        if(now_idxnd->son_blk_id[son_arridx] == EMPTY_BLKID)
        {
            int64_t new_idx_id = require_index_node_id();
            if(new_idx_id != -1)               
            {
                write_create_log(new_idx_id, IDXND_OP);
                void* idx_memaddr = indexid_to_memaddr(inode_id, new_idx_id, CREATE);
                idx_nd_pt new_idx_pt = (idx_nd_pt)idx_memaddr;
                if(new_idx_pt == NULL)
                    goto cache_error;
                
                int newnd_type = NOT_LEAF_NODE;
                if(searched_len + zipped_len + LAYER_BITWIDTH == KEY_LEN - LAYER_BITWIDTH)
                    newnd_type = LEAF_NODE;
                node_init(new_idx_pt, 0, newnd_type);
                write_change_log(new_idx_id, IDXND_OP, new_idx_pt, 0, ORCH_IDX_SIZE);

                now_idxnd->son_blk_id[son_arridx] = new_idx_id;
                write_change_log(now_idxnd_id, IDXND_OP, now_idxnd, 32+son_arridx*8, 8);
            }
        }
        int64_t next_layer_idxid = now_idxnd->son_blk_id[son_arridx];
        int64_t* next_father_pt = &(now_idxnd->son_blk_id[son_arridx]);
        int modify_flag = insert_node_into_index(next_layer_idxid, inode_id, next_father_pt, 
                                                searched_len + zipped_len + LAYER_BITWIDTH, blk_offset);
        if(modify_flag == 1)
        {
            write_change_log(now_idxnd_id, IDXND_OP, now_idxnd, 32+son_arridx*8, 8);
        }
        return 0;
    }
    else
    {
        int64_t level_zipped_part[MAX_ZIPPED_LAYER+1];

        int32_t zipped_layer_num = now_idxnd->zipped_layer;

        for(int32_t i = 1; i <= zipped_layer_num; i++)
        {
            int ziplayer_pos = (zipped_layer_num - i) * LAYER_BITWIDTH;
            int64_t layer_bit = (((1ULL << LAYER_BITWIDTH) - 1) << ziplayer_pos);
            level_zipped_part[i] = ((zipped_part & layer_bit) >> ziplayer_pos);
            // printf("level info: %" PRId32" %" PRId64" \n",i,level_zipped_part[i]);
        }
        
        int32_t allow_zipped_layer = 0;
        for(uint32_t i = 1; i <= zipped_layer_num; i++)
        {
            if(level_zipped_part[i] != 0)
            {
                allow_zipped_layer = i-1;
                break;
            }
        }
        // printf("allow zip: %" PRId64"\n",allow_zipped_layer);

        int64_t new_idx_id = require_index_node_id();
        // printf("new_idx_id: %" PRId64"\n",new_idx_id);
        if(new_idx_id != -1) 
        {
           
            write_create_log(new_idx_id, IDXND_OP);
            void* idx_memaddr = indexid_to_memaddr(inode_id, new_idx_id, CREATE);
            idx_nd_pt new_idx_pt = (idx_nd_pt)idx_memaddr;
            if(new_idx_pt == NULL)
                goto cache_error;
            node_init(new_idx_pt, allow_zipped_layer, NOT_LEAF_NODE);

            
            int new_idxnd_layer = allow_zipped_layer + 1;
            int new_layer_key = level_zipped_part[new_idxnd_layer];
            new_idx_pt->son_blk_id[0] = now_idxnd_id;
            write_change_log(new_idx_id, IDXND_OP, new_idx_pt, 0, ORCH_IDX_SIZE);

            now_idxnd->zipped_layer = zipped_layer_num - allow_zipped_layer - 1;
            write_change_log(now_idxnd_id, IDXND_OP, now_idxnd, 8, 8);


            *father_pt = new_idx_id;
            int64_t* next_father_pt = &(new_idx_pt->son_blk_id[0]);
            int modify_flag = insert_node_into_index(new_idx_id, inode_id, next_father_pt, searched_len, blk_offset);

            // int modify_flag = insert_node_into_index(new_idx_id, inode_id, father, searched_len, blk_offset);
            if(modify_flag == 1)
            {
                write_change_log(new_idx_id, IDXND_OP, new_idx_pt, 32, 8);
            }
            return 1;
        }
        else  
            goto alloc_error;
    }
cache_error:
    printf("root cache create error! -- insert_node_into_index\n");
    exit(0);
alloc_error:
    printf("root create error!\n");
    exit(0);
}


int get_idxson_type(uint64_t* type_info_pt, uint64_t pos)
{
    int64_t foff = pos/64, fpos = pos%64;
    if((type_info_pt[foff]) & (1ULL<<fpos)) 
        return VIR_LEAF_NODE;
    else 
        return SSD_BLOCK;
}


void change_idxson_type(idx_nd_pt target_idxpt, uint64_t pos, int type)
{
    int64_t foff = pos/64, fpos = pos%64;
    if(type == VIR_LEAF_NODE)
        target_idxpt->virnd_flag[foff] = (target_idxpt->virnd_flag[foff] | (1ULL<<fpos));
    else
        target_idxpt->virnd_flag[foff] = (target_idxpt->virnd_flag[foff] & (~(1ULL<<fpos)));

}


idx_nd_pt get_and_add_leafnode_pt(idx_root_pt root_pt, int64_t inode_id, int64_t offset)
{
    index_id_t leaf_node_id = search_leaf_node(root_pt->idx_entry_blkid, inode_id, 0, offset);
    // printf("leaf_node_id %ld\n",leaf_node_id);
    if(leaf_node_id == -1)
    {
        insert_node_into_index(root_pt->idx_entry_blkid, inode_id, &(root_pt->idx_entry_blkid), 0, offset);
        leaf_node_id = search_leaf_node(root_pt->idx_entry_blkid, inode_id, 0, offset);
        // printf("leaf_node_id1 %ld\n",leaf_node_id);
    }
    void* leaf_memaddr = indexid_to_memaddr(inode_id, leaf_node_id, CREATE);
    idx_nd_pt leaf_pt = (idx_nd_pt)leaf_memaddr;
    // printf("leaf_memaddr %ld\n",leaf_memaddr);
    if(leaf_pt == NULL)
        goto cache_error;
    return leaf_pt;
cache_error:
    printf("root cache create error! -- insert_node_into_index\n");
    exit(0);
}


int64_t get_and_add_leafnode_id(idx_root_pt root_pt, int64_t inode_id, int64_t offset)
{
    index_id_t leaf_node_id = search_leaf_node(root_pt->idx_entry_blkid, inode_id, 0, offset);
    if(leaf_node_id == -1)
    {
        insert_node_into_index(root_pt->idx_entry_blkid, inode_id, &(root_pt->idx_entry_blkid), 0, offset);
        leaf_node_id = search_leaf_node(root_pt->idx_entry_blkid, inode_id, 0, offset);
    }
    return leaf_node_id;
}


idx_nd_pt get_leafnode_pt(idx_root_pt root_pt, int64_t inode_id, int64_t offset)
{
    index_id_t leaf_node_id = search_leaf_node(root_pt->idx_entry_blkid, inode_id, 0, offset);
    // printf("leaf_node_id %ld\n",leaf_node_id);
    if(leaf_node_id == -1)
        return NULL;
    void* leaf_memaddr = indexid_to_memaddr(inode_id, leaf_node_id, CREATE);
    //  printf("leaf_memaddr %ld\n",leaf_memaddr);
    idx_nd_pt leaf_pt = (idx_nd_pt)leaf_memaddr;
    if(leaf_pt == NULL)
        goto cache_error;
    return leaf_pt;
cache_error:
    printf("root cache create error! -- insert_node_into_index\n");
    exit(0);
}


int64_t get_leafnode_id(idx_root_pt root_pt, int64_t inode_id, int64_t offset)
{
    index_id_t leaf_node_id = search_leaf_node(root_pt->idx_entry_blkid, inode_id, 0, offset);
    return leaf_node_id;
cache_error:
    printf("root cache create error! -- insert_node_into_index\n");
    exit(0);
}


void insert_strata_node(int64_t inode_id, idx_nd_pt leaf_pt, int64_t leaf_pos)
{
    if(leaf_pos < 0 || leaf_pos >= NODE_SON_CAPACITY)
        goto pos_error;
    
    int son_type = get_idxson_type(leaf_pt->virnd_flag, leaf_pos);
    if(son_type == SSD_BLOCK)
    {
        int64_t virnd_id = require_virindex_node_id();
        if(virnd_id != -1) 
        {

            write_create_log(virnd_id, VIRND_OP);
            void* virnd_memaddr = virnodeid_to_memaddr(inode_id, virnd_id, CREATE);
            vir_nd_pt virnd_mem_pt = (vir_nd_pt)virnd_memaddr;
            if(virnd_mem_pt == NULL)
                goto cache_error;
            
            vir_node_init(virnd_mem_pt, STRATA_NODE);
            virnd_mem_pt->ssd_dev_addr = leaf_pt->son_blk_id[leaf_pos];

            write_change_log(virnd_id, VIRND_OP, virnd_mem_pt, 0, ORCH_VIRND_SIZE);

            leaf_pt->son_blk_id[leaf_pos] = virnd_id;
            change_idxson_type(leaf_pt, leaf_pos, VIR_LEAF_NODE);
        }
        else  
            goto alloc_error;
    }
    else
        goto op_error;
    return;
op_error:
    printf("Can not insert strata node: The offset is error!\n");
    exit(0);
pos_error:
    printf("The offset/pos is out of range!\n");
    exit(0);
alloc_error:
    printf("vir node create error!\n");
    exit(0);
cache_error:
    printf("vir node cache create error! -- insert_strata_node\n");
    exit(0);
}


void delete_all_index_operation(idx_nd_pt now_idx_pt, int64_t inode_id)
{
    if(now_idx_pt->ndtype == LEAF_NODE)
    {
        for(int i = 0; i < NODE_SON_CAPACITY; i++)
        {
            if(now_idx_pt->son_blk_id[i] != EMPTY_BLKID)
            {
                int son_type = get_idxson_type(now_idx_pt->virnd_flag, i);
                if(son_type == SSD_BLOCK)
                    release_ssd_block(now_idx_pt->son_blk_id[i]);
                else if(son_type == VIR_LEAF_NODE)
                {
                    vir_nd_pt vir_idx_pt = virnodeid_to_memaddr(inode_id, now_idx_pt->son_blk_id[i], CREATE);
                    if(vir_idx_pt == NULL)
                        goto cache_error;
                    for(int j = 0; j < VLN_SLOT_SUM; j++)
                    {
                        if(vir_idx_pt->nvm_page_id[j] != EMPTY_BLKID)
                            release_nvm_page(vir_idx_pt->nvm_page_id[j]);
                    }
                    if(vir_idx_pt->ndtype == STRATA_NODE)
                    {
                        for(int j = 0; j < VLN_SLOT_SUM; j++)
                        {
                            if(vir_idx_pt->buf_meta_id[j] != EMPTY_BLKID)
                                release_buffer_metadata(vir_idx_pt->buf_meta_id[j]);
                        }
                        release_ssd_block(vir_idx_pt->ssd_dev_addr);
                    }
                    release_virindex_node(now_idx_pt->son_blk_id[i]);
                }
            }
        }
    }
    else if(now_idx_pt->ndtype == NOT_LEAF_NODE)
    {
        for(int i = 0; i < NODE_SON_CAPACITY; i++)
        {
            if(now_idx_pt->son_blk_id[i] != EMPTY_BLKID)
            {
                int64_t son_idx_id = now_idx_pt->son_blk_id[i];
                idx_nd_pt next_idx_pt = indexid_to_memaddr(inode_id, son_idx_id, CREATE);
                delete_all_index_operation(next_idx_pt, inode_id);
                release_index_node(son_idx_id);
            }
        }
    }
    return;
cache_error:
    printf("root cache create error! -- delete_all_index_node\n");
    exit(0);
}


void delete_part_index_operation(idx_nd_pt idx_pt, int64_t inode_id, int64_t searched_len, int64_t offset)
{
    int32_t zipped_len = idx_pt->zipped_layer * LAYER_BITWIDTH;       ///当层被压缩的位数
    uint64_t zipped_part_bit = (((1ULL << zipped_len) - 1) << (KEY_LEN - searched_len - zipped_len));
    uint64_t zipped_part = ((zipped_part_bit & offset) >> (KEY_LEN - searched_len - zipped_len));
    if(zipped_part != 0)
        goto index_error;
    else
    {
        if(idx_pt->ndtype == LEAF_NODE)
        {
            uint64_t next_layer_bit = (((1ULL << LAYER_BITWIDTH) - 1) << (KEY_LEN - searched_len - zipped_len));
            uint64_t next_layer_key = ((next_layer_bit & offset) >> (KEY_LEN - searched_len - zipped_len));
            for(int i = next_layer_key; i < NODE_SON_CAPACITY; i++)
            {
                int64_t next_layer_idxid = idx_pt->son_blk_id[i];
                void* idxnd_memaddr = indexid_to_memaddr(inode_id, next_layer_idxid, CREATE);
                idx_nd_pt next_idxnd = (idx_nd_pt)idxnd_memaddr;
                if(next_idxnd == NULL)
                    goto cache_error;
                delete_all_index_operation(next_idxnd, inode_id);
                release_index_node(next_layer_idxid);
            }
        }
        else
        {
            uint64_t next_layer_bit = (((1ULL << LAYER_BITWIDTH) - 1) << (KEY_LEN - searched_len - zipped_len));
            uint64_t next_layer_key = ((next_layer_bit & offset) >> (KEY_LEN - searched_len - zipped_len));
            for(int i = next_layer_key; i < NODE_SON_CAPACITY; i++)
            {
                int64_t next_layer_idxid = idx_pt->son_blk_id[i];
                void* idxnd_memaddr = indexid_to_memaddr(inode_id, next_layer_idxid, CREATE);
                idx_nd_pt next_idxnd = (idx_nd_pt)idxnd_memaddr;
                if(next_idxnd == NULL)
                    goto cache_error;
                if(i == next_layer_key)
                {
                    delete_part_index_operation(next_idxnd, inode_id, searched_len + zipped_len + LAYER_BITWIDTH, offset);
                    release_index_node(next_layer_idxid);
                }
                else
                {
                    delete_all_index_operation(next_idxnd, inode_id);
                    release_index_node(next_layer_idxid);
                }
            }
        }
    }
cache_error:
    printf("root cache create error! -- search_leaf_node\n");
    exit(0);
index_error:
    printf("The index is error!\n");
}


root_id_t create_index(int64_t inode_id)
{
    int64_t root_blk_id = require_index_node_id();
    if(root_blk_id != -1)   
    {
        
        write_create_log(root_blk_id, IDXND_OP);
        void* root_blk_memaddr = indexid_to_memaddr(inode_id, root_blk_id, CREATE);
        idx_root_pt root_pt = (idx_root_pt)root_blk_memaddr;
        if(root_pt == NULL)
            goto cache_error;
        

        memset(root_pt, 0x00, sizeof(idx_root_t));
        root_pt->ndtype = IDX_ROOT;
        root_pt->idx_inode_id = inode_id;
        root_pt->idx_entry_blkid = EMPTY_BLKID;
        root_pt->max_blk_offset = -1;
        
        write_change_log(root_blk_id, IDXND_OP, root_pt, 0, 4*sizeof(int64_t));

        return root_blk_id;
    }
    else       
        goto alloc_error;
alloc_error:
    printf("root create error!\n");
    exit(0);
cache_error:
    printf("root cache create error!\n");
    exit(0);
}

int append_ssd_blocks(root_id_t root_id, int64_t inode_id, 
                        int64_t app_blk_num, ssd_blk_id_t ssd_blk_id_arr[])
{

    idx_root_pt root_pt = check_root_id(root_id, inode_id);


    int64_t app_blk_off = root_pt->max_blk_offset + 1;
    if(app_blk_num + app_blk_off >= (1LL << KEY_LEN))
        goto blknum_error;


    int32_t app_cur = 0, need_app_blk_num = app_blk_num;
    while(need_app_blk_num > 0)
    {

        int64_t leaf_node_id = get_and_add_leafnode_id(root_pt, inode_id, app_blk_off);
        idx_nd_pt leaf_pt = (idx_nd_pt)indexid_to_memaddr(inode_id, leaf_node_id, CREATE);
        if(leaf_pt == NULL)
            goto cache_error;
        int64_t leaf_offset = (app_blk_off & ((1LL << LAYER_BITWIDTH) - 1));
        // printf(" %lld %lld %lld\n",need_app_blk_num, leaf_pt, leaf_offset);

        int this_app_blk = 0;
        for(int64_t i = leaf_offset; i < NODE_SON_CAPACITY; i++)
        {
            if(leaf_pt->son_blk_id[i] != EMPTY_BLKID)
                goto index_error;
            leaf_pt->son_blk_id[i] = ssd_blk_id_arr[app_cur];
            // printf("app info:  %" PRId64"  %" PRId64" \n",i,ssd_blk_id_arr[app_cur]);
            app_cur++; app_blk_off++; 
            need_app_blk_num--; this_app_blk++;
            if(need_app_blk_num == 0)
                break;
        }
        
        write_change_log(root_id, IDXND_OP, leaf_pt, (4+leaf_offset)*8, this_app_blk*8);
    }
    root_pt->max_blk_offset += app_blk_num;

    write_change_log(root_id, IDXND_OP, root_pt, 3*sizeof(int64_t), sizeof(int64_t));
    return 0;
blknum_error:
    printf("block num error!\n");
    return -1;
cache_error:
    printf("cache create error! -- append_ssd_blocks\n");
    exit(0);
index_error:
    printf("index error!\n");
    return -1;
}


int append_single_ssd_block(root_id_t root_id, int64_t inode_id, ssd_blk_id_t ssd_blk_id)
{
    ssd_blk_id_t ssd_blk_id_arr[2];
    ssd_blk_id_arr[0] = ssd_blk_id;
    return append_ssd_blocks(root_id, inode_id, 1, ssd_blk_id_arr);
}


int append_nvm_pages(root_id_t root_id, int64_t inode_id, 
                    int32_t app_blk_num, nvm_page_id_t nvm_page_id_arr[])
{
    idx_root_pt root_pt = check_root_id(root_id, inode_id);

    int64_t app_blk_offset = 0;
    if(root_pt->max_blk_offset == -1)
        app_blk_offset = 0;
    else
    {
        app_blk_offset = root_pt->max_blk_offset;
        idx_nd_pt leaf_pt = get_and_add_leafnode_pt(root_pt, inode_id, app_blk_offset);
        int64_t leaf_pos = (app_blk_offset & ((1LL << LAYER_BITWIDTH) - 1));
        if(leaf_pt->son_blk_id[leaf_pos] != EMPTY_BLKID)
        {
            int son_type = get_idxson_type(leaf_pt->virnd_flag, leaf_pos);
            if(son_type == SSD_BLOCK)
                app_blk_offset ++;
        }
    }

    int64_t now_page_cur = 0;
    while(app_blk_num > 0)
    {
        int64_t leaf_node_id = get_and_add_leafnode_id(root_pt, inode_id, app_blk_offset);
        idx_nd_pt leaf_pt = indexid_to_memaddr(inode_id, leaf_node_id, CREATE);
        if(leaf_pt == NULL)
            goto cache_error;
        // printf("leaf pt: %lld\n",leaf_pt);
        int64_t leaf_pos = (app_blk_offset & ((1LL << LAYER_BITWIDTH) - 1));
        if(leaf_pt->son_blk_id[leaf_pos] != EMPTY_BLKID)
        {
            int son_type = get_idxson_type(leaf_pt->virnd_flag, leaf_pos);
            if(son_type == SSD_BLOCK)
                goto index_error;
            else if(son_type == VIR_LEAF_NODE)
            {
                viridx_id_t vir_node_id = leaf_pt->son_blk_id[leaf_pos];
                vir_nd_pt virnd_mem_pt = virnodeid_to_memaddr(inode_id, vir_node_id, CREATE);
                if(virnd_mem_pt == NULL)
                    goto cache_error;
                
                
                if(virnd_mem_pt->ndtype == STRATA_NODE)
                    goto index_error;
                
                
                for(int i = virnd_mem_pt->max_pos; i < VLN_SLOT_SUM; i++)
                {
                    virnd_mem_pt->nvm_page_id[i] = nvm_page_id_arr[now_page_cur];
                    virnd_mem_pt->max_pos++;
                    now_page_cur++;  app_blk_num--;
                    if(app_blk_num <= 0)
                        break;
                }
                
                write_change_log(vir_node_id, VIRND_OP, virnd_mem_pt, 0, 128);
                if(virnd_mem_pt->max_pos == VLN_SLOT_SUM && app_blk_num > 0)
                    app_blk_offset++;
            }
        }
        else if(leaf_pt->son_blk_id[leaf_pos] == EMPTY_BLKID)
        {
            
            int64_t virnd_id = require_virindex_node_id();
            vir_nd_pt virnd_mem_pt = NULL;
            if(virnd_id != -1)
            {
                
                void* virnd_memaddr = virnodeid_to_memaddr(inode_id, virnd_id, CREATE);
                virnd_mem_pt = (vir_nd_pt)virnd_memaddr;
                if(virnd_mem_pt == NULL)
                    goto cache_error;
                
                vir_node_init(virnd_mem_pt, VIR_LEAF_NODE);
            }
            else
                goto alloc_error;
            
            
            int64_t leaf_pos = (app_blk_offset & ((1LL << LAYER_BITWIDTH) - 1));
            leaf_pt->son_blk_id[leaf_pos] = virnd_id;
            change_idxson_type(leaf_pt, leaf_pos, VIR_LEAF_NODE);
            
            write_change_log(leaf_node_id, IDXND_OP, leaf_pt, 16, 8);
            write_change_log(leaf_node_id, IDXND_OP, leaf_pt, 32+leaf_pos*8, 8);

            
            for(int i = virnd_mem_pt->max_pos; i < VLN_SLOT_SUM; i++)
            {
                virnd_mem_pt->nvm_page_id[i] = nvm_page_id_arr[now_page_cur];
                virnd_mem_pt->max_pos++;
                now_page_cur++;  app_blk_num--;
                if(app_blk_num <= 0)
                    break;
            }
            write_change_log(virnd_id, VIRND_OP, virnd_mem_pt, 0, 128);
            if(virnd_mem_pt->max_pos == VLN_SLOT_SUM && app_blk_num > 0)
                app_blk_offset++;
        }
    }
    root_pt->max_blk_offset = app_blk_offset;
    // printf("max_blk_offset: %lld\n",root_pt->max_blk_offset);
    return 0;
alloc_error:
    printf("vir node create error!\n");
    exit(0);
cache_error:
    printf("vir node cache create error! -- insert_node_into_index\n");
    exit(0);
index_error:
    printf("index structure error!\n");
    exit(0);
}


int append_single_nvm_page(root_id_t root_id, int64_t inode_id, nvm_page_id_t nvm_page_id)
{
    ssd_blk_id_t nvm_page_id_arr[2];
    nvm_page_id_arr[0] = nvm_page_id;
    return append_nvm_pages(root_id, inode_id, 1, nvm_page_id_arr);
}


off_info_t query_offset_info(root_id_t root_id, int64_t inode_id, int64_t blk_offset)
{
    off_info_t ret_off_info;
    ret_off_info.ndtype = EMPTY_BLK_TYPE;
    ret_off_info.ssd_dev_addr = EMPTY_BLKID;
    for(int i = 0; i < VLN_SLOT_SUM; i++)
        ret_off_info.nvm_page_id[i] = ret_off_info.buf_meta_id[i] = EMPTY_BLKID;
    ret_off_info.offset_ans = blk_offset;
    
    idx_root_pt root_pt = check_root_id(root_id, inode_id);
    idx_nd_pt leaf_pt = get_leafnode_pt(root_pt, inode_id, blk_offset);
    if(leaf_pt == NULL)
        goto query_error;
    int64_t leaf_pos = (blk_offset & ((1LL << LAYER_BITWIDTH) - 1));
    int son_type = get_idxson_type(leaf_pt->virnd_flag, leaf_pos);

    if(son_type == SSD_BLOCK && leaf_pt->son_blk_id[leaf_pos] != EMPTY_BLKID)
    {
        ret_off_info.ndtype = SSD_BLOCK;
        if(leaf_pt->son_blk_id[leaf_pos] == EMPTY_BLK_TYPE)
            ret_off_info.ssd_dev_addr = -1;
        else
            ret_off_info.ssd_dev_addr = ssdblk_to_devaddr(leaf_pt->son_blk_id[leaf_pos]);
    }
    else if(son_type == VIR_LEAF_NODE && leaf_pt->son_blk_id[leaf_pos] != EMPTY_BLKID)
    {
        viridx_id_t vir_node_id = leaf_pt->son_blk_id[leaf_pos];
        vir_nd_pt virnd_mem_pt = virnodeid_to_memaddr(inode_id, vir_node_id, CREATE);
        if(virnd_mem_pt == NULL)
            goto cache_error;
        

        for(int i = 0; i < VLN_SLOT_SUM; i++)
        {
            if(virnd_mem_pt->nvm_page_id[i] == EMPTY_BLK_TYPE)
                ret_off_info.nvm_page_id[i] = -1;
            else
                ret_off_info.nvm_page_id[i] = nvmpage_to_devaddr(virnd_mem_pt->nvm_page_id[i]);
        }

        if(virnd_mem_pt->ndtype == VIR_LEAF_NODE)
        {
            ret_off_info.ndtype = VIR_LEAF_NODE;
        }
        else if(virnd_mem_pt->ndtype == STRATA_NODE)
        {
            ret_off_info.ndtype = STRATA_NODE;

            if(virnd_mem_pt->ssd_dev_addr == EMPTY_BLK_TYPE)
                ret_off_info.ssd_dev_addr = -1;
            else
                ret_off_info.ssd_dev_addr = ssdblk_to_devaddr(virnd_mem_pt->ssd_dev_addr);

            for(int i = 0; i < VLN_SLOT_SUM; i++)
            {
                if(virnd_mem_pt->buf_meta_id[i] == EMPTY_BLK_TYPE)
                    ret_off_info.buf_meta_id[i] = -1;
                else
                    ret_off_info.buf_meta_id[i] = bufmetaid_to_devaddr(virnd_mem_pt->buf_meta_id[i]);
            }
        }
    }
    else
        goto query_error;
    return ret_off_info;
query_error:
    ret_off_info.ndtype = EMPTY_BLK_TYPE;
    return ret_off_info;
cache_error:
    printf("vir node cache create error! -- query_offset_info\n");
    exit(0);
}


void change_ssd_blk_info(root_id_t root_id, int64_t inode_id, 
                        int64_t blk_offset, int64_t changed_blkid)
{
    if(blk_offset < 0 || blk_offset >= (1LL << KEY_LEN))
        goto offset_error;
    idx_root_pt root_pt = check_root_id(root_id, inode_id);


    int64_t leaf_node_id = get_leafnode_id(root_pt, inode_id, blk_offset);
    if(leaf_node_id == -1)
        goto index_error;
    idx_nd_pt leaf_pt = (idx_nd_pt)indexid_to_memaddr(inode_id, leaf_node_id, CREATE);
    if(leaf_pt == NULL)
        goto cache_error;
    
    int64_t leaf_pos = (blk_offset & ((1LL << LAYER_BITWIDTH) - 1));
    leaf_pt->son_blk_id[leaf_pos] = changed_blkid;

    write_change_log(leaf_node_id, IDXND_OP, leaf_pt, 32+leaf_pos*8, 8);
    return;
offset_error:
    printf("The offset is out of range!\n");
    exit(0);
index_error:
    printf("The offset does not exist!\n");
    exit(0);
cache_error:
    printf("cache create error! -- change_ssd_blk_info\n");
    exit(0);
}


void change_nvm_page_info(root_id_t root_id, int64_t inode_id, 
                        int64_t blk_offset, int32_t pos, int64_t changed_pageid)
{

    if(blk_offset < 0 || blk_offset >= (1LL << KEY_LEN))
        goto offset_error;
    idx_root_pt root_pt = check_root_id(root_id, inode_id);


    idx_nd_pt leaf_pt = get_leafnode_pt(root_pt, inode_id, blk_offset);
    if(leaf_pt == NULL)
        goto index_error;
    

    int64_t leaf_pos = (blk_offset & ((1LL << LAYER_BITWIDTH) - 1));
    

    viridx_id_t vir_node_id = leaf_pt->son_blk_id[leaf_pos];
    vir_nd_pt virnd_mem_pt = virnodeid_to_memaddr(inode_id, vir_node_id, CREATE);
    if(virnd_mem_pt == NULL)
        goto cache_error;
    
    if(pos >= virnd_mem_pt->max_pos || pos < 0)
        goto offset_error;
    virnd_mem_pt->nvm_page_id[pos] = changed_pageid;

    write_change_log(vir_node_id, VIRND_OP, virnd_mem_pt, 24+pos*8, 8);
    return;
offset_error:
    printf("The offset/pos is out of range!\n");
    exit(0);
index_error:
    printf("The offset does not exist!\n");
    exit(0);
cache_error:
    printf("vir node cache create error! -- query_offset_info\n");
    exit(0);
}

void change_virnd_to_ssdblk(root_id_t root_id, int64_t inode_id, int64_t blk_offset, 
                            int64_t changed_blkid)
{

    if(blk_offset < 0 || blk_offset >= (1LL << KEY_LEN))
        goto offset_error;
    idx_root_pt root_pt = check_root_id(root_id, inode_id);


    idx_nd_pt leaf_pt = get_leafnode_pt(root_pt, inode_id, blk_offset);
    if(leaf_pt == NULL)
        goto index_error;
    

    int64_t leaf_pos = (blk_offset & ((1LL << LAYER_BITWIDTH) - 1));
    

    viridx_id_t vir_node_id = leaf_pt->son_blk_id[leaf_pos];
    vir_nd_pt virnd_mem_pt = virnodeid_to_memaddr(inode_id, vir_node_id, CREATE);
    if(virnd_mem_pt == NULL)
        goto cache_error;
    
    if(virnd_mem_pt->ndtype == STRATA_NODE)
        goto type_error;
    
    leaf_pt->son_blk_id[leaf_pos] = changed_blkid;
    change_idxson_type(leaf_pt, leaf_pos, SSD_BLOCK);
    for(int i = 0; i < VLN_SLOT_SUM; i++)
    {
        if(virnd_mem_pt->nvm_page_id[i] != EMPTY_BLK_TYPE)
            release_nvm_page(virnd_mem_pt->nvm_page_id[i]);
    }
    release_virindex_node(vir_node_id);
    return;
offset_error:
    printf("The offset/pos is out of range!\n");
    exit(0);
index_error:
    printf("The offset does not exist!\n");
    exit(0);
cache_error:
    printf("vir node cache create error! -- change_virnd_to_ssdblk\n");
    exit(0);
type_error:
    printf("The index type is error!\n");
    exit(0);
}

int insert_strata_page_and_metabuf(root_id_t root_id, int64_t inode_id, int64_t blk_offset, 
                                int32_t pos, nvm_page_id_t nvm_page_id, bufmeta_id_t buf_id)
{

    if(blk_offset < 0 || blk_offset >= (1LL << KEY_LEN))
        goto offset_error;
    idx_root_pt root_pt = check_root_id(root_id, inode_id);


    int64_t leaf_node_id = get_leafnode_id(root_pt, inode_id, blk_offset);
    if(leaf_node_id == -1)
        goto index_error;
    idx_nd_pt leaf_pt = (idx_nd_pt)indexid_to_memaddr(inode_id, leaf_node_id, CREATE);
    if(leaf_pt == NULL)
        goto cache_error;
    

    int64_t leaf_pos = (blk_offset & ((1LL << LAYER_BITWIDTH) - 1));
    if(leaf_pt->son_blk_id[leaf_pos] == EMPTY_BLKID)
        goto index_error;
    else if(leaf_pt->son_blk_id[leaf_pos] != EMPTY_BLKID)
    {
        int son_type = get_idxson_type(leaf_pt->virnd_flag, leaf_pos);
        if(son_type == SSD_BLOCK)
        {
            insert_strata_node(inode_id, leaf_pt, leaf_pos);
            write_change_log(leaf_node_id, IDXND_OP, leaf_pt, 16, 8);
        }

        viridx_id_t vir_node_id = leaf_pt->son_blk_id[leaf_pos];
        vir_nd_pt virnd_mem_pt = virnodeid_to_memaddr(inode_id, vir_node_id, CREATE);
        if(virnd_mem_pt == NULL)
            goto cache_error;


        if(virnd_mem_pt->ndtype == VIR_LEAF_NODE)
            goto index_error;
        else
        {
            if(pos < 0 || pos >= VLN_SLOT_SUM)
                goto offset_error;
            if(virnd_mem_pt->nvm_page_id[pos] != EMPTY_BLKID || virnd_mem_pt->buf_meta_id[pos] != EMPTY_BLKID)
                goto op_error;
            virnd_mem_pt->nvm_page_id[pos] = nvm_page_id;
            virnd_mem_pt->buf_meta_id[pos] = buf_id;
            write_change_log(vir_node_id, VIRND_OP, virnd_mem_pt, 24+pos*8, 8);
            write_change_log(vir_node_id, VIRND_OP, virnd_mem_pt, 56+pos*8, 8);
        }
    }
    return 0;
offset_error:
    printf("The offset/pos is out of range!\n");
    exit(0);
index_error:
    printf("The offset does not exist!\n");
    exit(0);
op_error:
    printf("Can not insert strata node: The offset is error!\n");
    return -1;
alloc_error:
    printf("vir node create error!\n");
    exit(0);
cache_error:
    printf("cache create error! -- query_offset_info\n");
    exit(0);
}

void delete_part_index(root_id_t root_id, int64_t inode_id, int64_t blk_offset)
{
    idx_root_pt root_pt = check_root_id(root_id, inode_id); 
    idx_nd_pt start_idx_pt = indexid_to_memaddr(inode_id, root_pt->idx_entry_blkid, CREATE);
    delete_part_index_operation(start_idx_pt, inode_id, 0, blk_offset);
    release_index_node(root_pt->idx_entry_blkid);
}

void delete_all_index(root_id_t root_id, int64_t inode_id)
{
    idx_root_pt root_pt = check_root_id(root_id, inode_id); 
    idx_nd_pt start_idx_pt = indexid_to_memaddr(inode_id, root_pt->idx_entry_blkid, CREATE);
    delete_all_index_operation(start_idx_pt, inode_id);
    release_index_node(root_pt->idx_entry_blkid);
}

void lock_range_lock(root_id_t root_id, int64_t inode_id, int64_t blk_start_off, int64_t blk_end_off)
{

    if(blk_start_off < 0 || blk_start_off >= (1LL << KEY_LEN))
        goto offset_error;
    if(blk_end_off < 0 || blk_end_off >= (1LL << KEY_LEN))
        goto offset_error;
    idx_root_pt root_pt = check_root_id(root_id, inode_id);


    int64_t off_pre36 = -1;
    idx_nd_pt leaf_pt = NULL;
    for(int64_t i = blk_start_off; i <= blk_end_off; i++)
    {
        int64_t now_off_pre36 = (i >> LAYER_BITWIDTH);
        if(now_off_pre36 != off_pre36)
        {
            leaf_pt = get_and_add_leafnode_pt(root_pt, inode_id, i);
            if(leaf_pt == NULL)
                goto index_error;
            int64_t leaf_pos = (i & ((1LL << LAYER_BITWIDTH) - 1));
            bitlock_acquire(&(leaf_pt->bit_lock), leaf_pos);
            now_off_pre36 != off_pre36;
        }
        else
        {
            int64_t leaf_pos = (i & ((1LL << LAYER_BITWIDTH) - 1));
            bitlock_acquire(&(leaf_pt->bit_lock), leaf_pos);
        }
    }
    return;
offset_error:
    printf("The offset/pos is out of range!\n");
    exit(0);
index_error:
    printf("The offset does not exist!\n");
    exit(0); 
}

void unlock_range_lock(root_id_t root_id, int64_t inode_id, int64_t blk_start_off, int64_t blk_end_off)
{

    if(blk_start_off < 0 || blk_start_off >= (1LL << KEY_LEN))
        goto offset_error;
    if(blk_end_off < 0 || blk_end_off >= (1LL << KEY_LEN))
        goto offset_error;
    idx_root_pt root_pt = check_root_id(root_id, inode_id);


    int64_t off_pre36 = -1;
    idx_nd_pt leaf_pt = NULL;
    for(int64_t i = blk_start_off; i <= blk_end_off; i++)
    {
        int64_t now_off_pre36 = (i >> LAYER_BITWIDTH);
        if(now_off_pre36 != off_pre36)
        {
            leaf_pt = get_leafnode_pt(root_pt, inode_id, i);
            if(leaf_pt == NULL)
                goto index_error;
            int64_t leaf_pos = (i & ((1LL << LAYER_BITWIDTH) - 1));
            bitlock_release(&(leaf_pt->bit_lock), leaf_pos);
            now_off_pre36 != off_pre36;
        }
        else
        {
            int64_t leaf_pos = (i & ((1LL << LAYER_BITWIDTH) - 1));
            
            bitlock_release(&(leaf_pt->bit_lock), leaf_pos);
        }
    }
    return;
offset_error:
    printf("The offset/pos is out of range!\n");
    exit(0);
index_error:
    printf("The offset does not exist!\n");
    exit(0); 
}


void sync_all_index(idx_nd_pt now_idx_pt, int64_t inode_id)
{
    if(now_idx_pt->ndtype == LEAF_NODE)
    {
        for(int i = 0; i < NODE_SON_CAPACITY; i++)
        {
            if(now_idx_pt->son_blk_id[i] != EMPTY_BLKID)
            {
                int son_type = get_idxson_type(now_idx_pt->virnd_flag, i);
                if(son_type == VIR_LEAF_NODE)
                {
                    sync_virnd_blk(now_idx_pt->son_blk_id[i]);
                }
            }
        }
    }
    else if(now_idx_pt->ndtype == NOT_LEAF_NODE)
    {
        for(int i = 0; i < NODE_SON_CAPACITY; i++)
        {
            if(now_idx_pt->son_blk_id[i] != EMPTY_BLKID)
            {
                int64_t son_idx_id = now_idx_pt->son_blk_id[i];
                idx_nd_pt next_idx_pt = indexid_to_memaddr(inode_id, son_idx_id, CREATE);
                sync_all_index(next_idx_pt, inode_id);
                sync_index_blk(son_idx_id);
            }
        }
    }
    return;
cache_error:
    printf("root cache create error! -- sync_all_index\n");
    exit(0);
}


#ifdef __cplusplus
}
#endif
