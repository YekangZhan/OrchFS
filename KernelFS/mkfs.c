#include "type.h"
#include "kernel_func.h"
#include "balloc.h"
#include "device.h"
#include "log.h"
#include "thdpool.h"
#include "addr_util.h"
#include "kernel_socket.h"
#include "kindex.h"
#include "../config/protocol.h"
#include "../config/socket_config.h"
#include "../config/config.h"

// #define KERNEL_FUNC_DEBUG

int64_t kernel_inode_create(int i_type)
{

	int64_t new_ino_id = alloc_single_inode(PAR_BLK_ID);


	orch_kfs_inode_pt new_ino_pt = malloc(ORCH_INODE_SIZE);


	new_ino_pt->i_size = 0;
	new_ino_pt->i_idxroot = -1;
	new_ino_pt->i_nlink = 0;
	new_ino_pt->i_uid =	0;
	new_ino_pt->i_gid =	0;
	new_ino_pt->i_type = i_type;
	new_ino_pt->i_mode = S_IRWXU | S_IRWXG | S_IRWXO;
	korch_time_stamp(&new_ino_pt->i_atim);
	korch_time_stamp(&new_ino_pt->i_ctim);
	korch_time_stamp(&new_ino_pt->i_mtim);


    int64_t inode_addr = INODE_BLKID_TO_DEVADDR(new_ino_id);
    write_data_to_devs(new_ino_pt, ORCH_INODE_SIZE, inode_addr);
    // fprintf(stderr,"new_ino_id: %lld %lld\n",new_ino_id,inode_addr);
    free(new_ino_pt);
	return new_ino_id;
}

void sync_inode(int64_t ino_id, void* changed_ino_pt)
{
    int64_t inode_addr = INODE_BLKID_TO_DEVADDR(ino_id);
    // printf("inode addr: %lld\n",inode_addr);
    write_data_to_devs(changed_ino_pt, ORCH_INODE_SIZE, inode_addr);
}

void init_root_dir(int64_t dir_ino, int64_t father_dir_ino)
{

    void* ino_sp_pt = malloc(ORCH_INODE_SIZE);
    orch_kfs_inode_pt ino_pt = (orch_kfs_inode_pt)ino_sp_pt;
    int64_t dir_ino_addr = INODE_BLKID_TO_DEVADDR(dir_ino);
    read_data_from_devs(ino_pt, ORCH_INODE_SIZE, dir_ino_addr);

    int64_t nvm_page_id = alloc_single_nvm_page(PAR_BLK_ID);
    int64_t page_dev_addr = PAGE_BLKID_TO_DEVADDR(nvm_page_id);
    

    orch_kfs_dirent_pt new_dir_pt = malloc(KDIRENT_SIZE * 2); 

    char this_fname[10] = {0};
    this_fname[0] = '.'; this_fname[1] = 0;
    new_dir_pt[0].d_ino = dir_ino;
    new_dir_pt[0].d_off = 0;
    new_dir_pt[0].d_type = KDIRENT_FILE_T;
    new_dir_pt[0].d_namelen = 1;
    strcpy(new_dir_pt[0].d_name, this_fname);
    
    this_fname[0] = '.';  this_fname[1] = '.'; this_fname[2] = 0;
    new_dir_pt[1].d_ino = father_dir_ino;
    new_dir_pt[1].d_off = KDIRENT_SIZE;
    new_dir_pt[1].d_type = KDIRENT_FILE_T;
    new_dir_pt[1].d_namelen = 2;
    strcpy(new_dir_pt[1].d_name, this_fname);

    write_data_to_devs(new_dir_pt, KDIRENT_SIZE*2, page_dev_addr);


    ino_pt->i_size = KDIRENT_SIZE*2;
    ino_pt->i_idxroot = create_index_with_page(dir_ino, nvm_page_id);
    // printf("update type: %lld\n",ino_pt->i_type);
    sync_inode(dir_ino, ino_pt);

    free(new_dir_pt);
    free(ino_pt);
    return;
file_type_error:
    printf("The file type is not dirent file!\n");
    exit(1);
dir_open_error:
    printf("can not get dirent file fd!\n");
    exit(1);
}

void init_super_block()
{

    void* init_buf = valloc(ORCH_SUPER_BLK_SIZE);
    memset(init_buf, 0x00, ORCH_SUPER_BLK_SIZE);
    orch_super_blk_pt sb_pt = init_buf;
    for(int i = 0; i < 3; i++)
        sb_pt->magic_num[i] = ORCH_MAGIC_NUM;
    for(int i = START_BMP_ID; i <= END_BMP_ID; i++)
    {
        if(i == INODE_BMP)
            sb_pt->bmp_dev_addr_list[i] = OFFSET_INODE_BMP;
        else if(i == PAGE_BMP)
            sb_pt->bmp_dev_addr_list[i] = OFFSET_PAGE_BMP;
        else if(i == BLOCK_BMP)
            sb_pt->bmp_dev_addr_list[i] = OFFSET_BLOCK_BMP;
        else if(i == IDX_NODE_BMP)
            sb_pt->bmp_dev_addr_list[i] = OFFSET_IDX_BMP;
        else if(i == VIR_NODE_BMP)
            sb_pt->bmp_dev_addr_list[i] = OFFSET_VIRND_BMP;
        else if(i == BUFMETA_BMP)
            sb_pt->bmp_dev_addr_list[i] = OFFSET_BUFMETA_BMP;
    }
    write_data_to_devs(init_buf, ORCH_SUPER_BLK_SIZE, OFFSET_SUPER_BLK);


    init_mem_bmp();
    sb_pt->root_inode = kernel_inode_create(KER_DIR_FILE);
    init_root_dir(sb_pt->root_inode, sb_pt->root_inode);
    write_data_to_devs(init_buf, ORCH_SUPER_BLK_SIZE, OFFSET_SUPER_BLK);
    sync_all_mem_bmp();


    free(init_buf);
}

void mkfs()
{
	device_init();
    init_super_block();
    device_close();
}

int main()
{
    mkfs();
    fprintf(stderr,"mkfs end!\n");
    fflush(stdout);
}