#include "kindex.h"
#include "balloc.h"
#include "device.h"
#include "addr_util.h"

#include "../config/config.h"
#include "../config/log_config.h"



void change_idxson_type(ker_idx_nd_pt target_idxpt, uint64_t pos, int type)
{
    int64_t foff = pos/64, fpos = pos%64;
    if(type == VIR_LEAF_NODE)
        target_idxpt->virnd_flag[foff] = (target_idxpt->virnd_flag[foff] | (1ULL<<fpos));
    else
        target_idxpt->virnd_flag[foff] = (target_idxpt->virnd_flag[foff] & (~(1ULL<<fpos)));
    /* 此处修改了节点信息，需要记录日志 */
}


void node_init(ker_idx_nd_pt idxnd_spt, int64_t init_zipped_layer, int32_t ntype)
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



void vir_node_init(ker_vir_nd_pt virnd_spt, int32_t ntype)
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


void root_init(ker_idx_root_pt root_spt, int64_t inode_id)
{
    root_spt->ndtype = IDX_ROOT;
    root_spt->idx_inode_id = inode_id;
    root_spt->idx_entry_blkid = EMPTY_BLKID;
    root_spt->max_blk_offset = -1;
}

root_id_t create_index_with_page(int64_t inode_id, nvm_page_id_t page_id)
{
    int64_t root_blk_id = alloc_single_idx_node(PAR_BLK_ID);
    int64_t idx_blk_id = alloc_single_idx_node(PAR_BLK_ID);
    int64_t virnd_blk_id = alloc_single_viridx_node(PAR_BLK_ID);
    if(root_blk_id < 0 || idx_blk_id < 0 || virnd_blk_id < 0)
        goto alloc_error;
    else
    {
        void* root_memaddr = malloc(ORCH_IDX_SIZE);
        void* idx_memaddr = malloc(ORCH_IDX_SIZE);
        void* virnd_memaddr = malloc(ORCH_VIRND_SIZE);
        if(root_memaddr == NULL || idx_memaddr == NULL || virnd_memaddr == NULL)
            goto alloc_error;
        ker_idx_root_pt root_pt = (ker_idx_root_pt)root_memaddr;
        ker_idx_nd_pt idxnd_pt = (ker_idx_nd_pt)idx_memaddr;
        ker_vir_nd_pt virnd_pt = (ker_vir_nd_pt)virnd_memaddr;


        root_init(root_pt, inode_id);
        root_pt->idx_entry_blkid = idx_blk_id;
        root_pt->max_blk_offset = 0;


        node_init(idxnd_pt, MAX_ZIPPED_LAYER, LEAF_NODE);
        idxnd_pt->son_blk_id[0] = virnd_blk_id;
        change_idxson_type(idxnd_pt, 0, VIR_LEAF_NODE);


        vir_node_init(virnd_pt, VIR_LEAF_NODE);
        virnd_pt->nvm_page_id[0] = page_id;
        virnd_pt->max_pos = 1;


        int64_t root_addr = IDX_BLKID_TO_DEVADDR(root_blk_id);
        write_data_to_devs(root_pt, ORCH_IDX_SIZE, root_addr);

        int64_t idx_addr = IDX_BLKID_TO_DEVADDR(idx_blk_id);
        write_data_to_devs(idxnd_pt, ORCH_IDX_SIZE, idx_addr);
        
        int64_t virnd_addr = VIRND_BLKID_TO_DEVADDR(virnd_blk_id);
        write_data_to_devs(virnd_pt, ORCH_VIRND_SIZE, virnd_addr);

        free(root_memaddr);
        free(idx_memaddr);
        free(virnd_memaddr);
        return root_blk_id;
    }
alloc_error:
    printf("root create error!\n");
    exit(0);
cache_error:
    printf("root cache create error!\n");
    exit(0);
}


