#include "balloc.h"
#include "type.h"
#include "device.h"
#include "addr_util.h"
#include "../config/config.h"

#ifdef __cplusplus       
extern "C"{
#endif

void init_mem_bmp()
{
	void* sb_addr = valloc(SIZE_4KiB);
	read_data_from_devs(sb_addr, 512, 0);
	// orch_super_blk_pt out = (orch_super_blk_pt)sb_addr;
	orch_super_blk_pt sb_pt = (orch_super_blk_pt)sb_addr;
	for(int i = START_BMP_ID; i <= END_BMP_ID; i++)
	{
		mem_bmp_pt bmp_info = mem_bmp_arr + i; 

		// initialize metadata about bitmap
		bmp_info->dev_start_addr = (sb_pt->bmp_dev_addr_list)[i];
		bmp_info->bmp_alloc_cur = (sb_pt->bmp_alloc_cur_list)[i];
		bmp_info->bmp_alloc_range = (sb_pt->bmp_alloc_range_list)[i];
		bmp_info->bmp_used_num = (sb_pt->bmp_used_num_list)[i];

		if(i == INODE_BMP)
		{
			bmp_info->bmp_capacity = MAX_INODE_NUM / 8;
			bmp_info->blk_area_start = OFFSET_INODE;
			bmp_info->blk_size = ORCH_INODE_SIZE;
		}
		else if(i == PAGE_BMP)
		{
			bmp_info->bmp_capacity = MAX_PAGE_NUM / 8;
			bmp_info->blk_area_start = OFFSET_PAGE;
			bmp_info->blk_size = ORCH_PAGE_SIZE;
		}
		else if(i == BLOCK_BMP)
		{
			bmp_info->bmp_capacity = MAX_BLOCK_NUM / 8;
			bmp_info->blk_area_start = OFFSET_BLOCK;
			bmp_info->blk_size = ORCH_BLOCK_SIZE;
		}
		else if(i == BUFMETA_BMP)
		{
			bmp_info->bmp_capacity = MAX_BUFMETA_NUM / 8;
			bmp_info->blk_area_start = OFFSET_BUFMETA_BMP;
			bmp_info->blk_size = ORCH_BUFMETA_SIZE;
		}
		else if(i == IDX_NODE_BMP)
		{
			bmp_info->bmp_capacity = MAX_INDEX_NUM / 8;
			bmp_info->blk_area_start = OFFSET_INDEX;
			bmp_info->blk_size = ORCH_INODE_SIZE;
		}
		else if(i == VIR_NODE_BMP)
		{
			bmp_info->bmp_capacity = MAX_VIRND_NUM / 8;
			bmp_info->blk_area_start = ORCH_IDX_SIZE;
			bmp_info->blk_size = ORCH_VIRND_SIZE;
		}
		uint64_t valloc_size = bmp_info->bmp_capacity / MEM_PAGE_SIZE * MEM_PAGE_SIZE + MEM_PAGE_SIZE;
		bmp_info->bmp_start_pt = valloc(valloc_size); 

		// read bitmap data
		read_data_from_devs(bmp_info->bmp_start_pt, bmp_info->bmp_capacity, bmp_info->dev_start_addr);
	}
	fflush(stdout);
	free(sb_addr);
}

uint32_t sync_type_mem_bmp(int32_t bmp_type)
{
	mem_bmp_pt bmp_info = mem_bmp_arr+bmp_type;
	// uint8_t* outpt = bmp_info->bmp_start_pt;
	write_data_to_devs(bmp_info->bmp_start_pt, bmp_info->bmp_capacity, bmp_info->dev_start_addr);
	return 0;
}

uint32_t sync_all_mem_bmp()
{
	void* sb_addr = valloc(SIZE_4KiB);
	read_data_from_devs(sb_addr, ORCH_SUPER_BLK_SIZE, OFFSET_SUPER_BLK);
	orch_super_blk_pt sb_pt = (orch_super_blk_pt)sb_addr;
	for(int i = START_BMP_ID; i <= END_BMP_ID; i++)
	{
		sync_type_mem_bmp(i);
		mem_bmp_pt bmp_info = mem_bmp_arr + i;
		(sb_pt->bmp_dev_addr_list)[i] = bmp_info->dev_start_addr;
		(sb_pt->bmp_alloc_cur_list)[i] = bmp_info->bmp_alloc_cur;
		(sb_pt->bmp_alloc_range_list)[i] = bmp_info->bmp_alloc_range;
		(sb_pt->bmp_used_num_list)[i] = bmp_info->bmp_used_num;
	}
	write_data_to_devs(sb_addr, ORCH_SUPER_BLK_SIZE, OFFSET_SUPER_BLK);
	free(sb_addr);
	return 0;
}

uint32_t sync_mem_bmp(int32_t bmp_type, int64_t bit_off, int64_t bit_len)
{
	mem_bmp_pt bmp_info = mem_bmp_arr+bmp_type;
	if(bit_off+bit_len > bmp_info->bmp_capacity*8)
	{
		printf("bitmap copy cross the border!\n");
		return 2;
	}
	uint64_t byte_start = bit_off/8, byte_end = (bit_off+bit_len-1)/8;
	uint64_t cpy_bytes = byte_end-byte_start+1;
	write_data_to_devs(bmp_info->bmp_start_pt+byte_start, cpy_bytes, bmp_info->dev_start_addr+byte_start);
	return 0;
}

void delete_mem_bmp()
{
	for(int i = START_BMP_ID; i <= END_BMP_ID; i++)
	{
		if(mem_bmp_arr[i].bmp_start_pt!=NULL)
			free(mem_bmp_arr[i].bmp_start_pt);
		else
		{
			printf("memory bitmap address error!\n");
			exit(0);
		}
	}
}

/* find free bits, 
 * @bmp_type: The bitmap area used to search for available bits
 * @need_alloc_bit: The number of bit need to allocate
 * @addr_list[]
 * @addr_list_cnt
 * @return      The actual number of allocated bits. 
 */
uint32_t find_free_bit(int32_t bmp_type, uint64_t need_alloc_bit, uint64_t addr_list[], uint32_t addr_list_cnt)
{
	mem_bmp_pt bmp_info = mem_bmp_arr+bmp_type;
	uint32_t alloc_blks = 0;
	for(int64_t i = bmp_info->bmp_alloc_cur; i < bmp_info->bmp_alloc_range; i++)
	{
		uint8_t* now_alloc_pt = bmp_info->bmp_start_pt+i;
		if(*now_alloc_pt != 0xff)
		{
			for(int64_t j = 7; j >= 0; j--)
			{
				if((*now_alloc_pt & (1<<j)) == 0)
				{
					*now_alloc_pt |= (1<<j);
					addr_list[addr_list_cnt+alloc_blks] = i*8 + 7-j;
					alloc_blks++; bmp_info->bmp_used_num++;
				}
				if(alloc_blks == need_alloc_bit)
				{
					bmp_info->bmp_alloc_cur = i;
					goto alloc_end;
				}
			}
		}
	}
	bmp_info->bmp_alloc_cur = bmp_info->bmp_alloc_range;
	goto alloc_end;
alloc_end:
	return alloc_blks;
}

/* do allocate block operation 
 * @bmp_type: The bitmap area used to search for available bits
 * @alloc_blk_num: The number of bit need to allocate
 * @addr_list[]:
 * @return 
 */
uint32_t do_alloc(int32_t bmp_type, uint64_t alloc_blk_num, uint64_t addr_list[])
{
	mem_bmp_pt bmp_info = mem_bmp_arr+bmp_type;
	// allocate block fail
	if(bmp_info->bmp_used_num+alloc_blk_num > bmp_info->bmp_capacity*8)
		goto failed;
	
	// allocate block
	uint32_t now_alloc_blks = 0, addr_list_cur = 0;
	while(alloc_blk_num > 0)
	{
		if(bmp_info->bmp_alloc_cur == bmp_info->bmp_alloc_range)
		{
			if(bmp_info->bmp_alloc_range + MEM_PAGE_SIZE <= bmp_info->bmp_capacity)
			{
				memset(bmp_info->bmp_start_pt + bmp_info->bmp_alloc_range, 0x00, MEM_PAGE_SIZE);
				bmp_info->bmp_alloc_range += MEM_PAGE_SIZE;
			}
			else
			{
				printf("space run out\n");
				bmp_info->bmp_alloc_range = bmp_info->bmp_capacity;
				bmp_info->bmp_alloc_cur = 0;
			}
		}
		now_alloc_blks = find_free_bit(bmp_type, alloc_blk_num, addr_list, addr_list_cur);
		alloc_blk_num -= now_alloc_blks; addr_list_cur += now_alloc_blks;
	}
	return 0;
failed:
	printf("error info: %d %lld %lld %lld\n",bmp_type, bmp_info->bmp_used_num, alloc_blk_num, bmp_info->bmp_capacity);
	printf("bad alloc!\n");
	return 3;
}

uint32_t do_dealloc(int32_t bmp_type, int64_t blk_id)
{
	mem_bmp_pt bmp_info = mem_bmp_arr+bmp_type;
	if(blk_id < 0 || blk_id > bmp_info->bmp_capacity * 8)
		goto failed;
	uint8_t* now_dealloc_pt = bmp_info->bmp_start_pt + blk_id/8;
	uint32_t bit_off = 7 - blk_id%8;
	if((*now_dealloc_pt & (1<<bit_off)) != 0)
	{
		*now_dealloc_pt ^= (1<<bit_off);
		bmp_info->bmp_used_num --;
	}
	else
		goto warning;
	return 0;
failed:
	printf("dealloc error, the address is out of range!\n");
	return 4;
// error:
// 	printf("The address is Non-aligned!\n");
// 	return 5;
warning:
	printf("dealloc warning, the block does not exist! -- %d %"PRId64"\n",bmp_type,blk_id);
	return 6;
}

/* inode */
uint64_t alloc_single_inode(int return_type)
{
	uint64_t ret_addr_list[3] = {0};
	int error_info = do_alloc(INODE_BMP, 1, ret_addr_list);
	if(error_info==0)
	{
		if(return_type == RET_BLK_ID)
			return ret_addr_list[0];
		else if(return_type == RET_BLK_ADDR)
			return INODE_BLKID_TO_DEVADDR(ret_addr_list[0]);
		else
			printf("return type error!\n");
	}
	exit(0);
}
void alloc_inodes(int64_t alloc_blk_num, uint64_t addr_list[], int return_type)
{
	int error_info = do_alloc(INODE_BMP, alloc_blk_num, addr_list);
	if(return_type == RET_BLK_ADDR)
	{
		// ------ balloc check correct ---------
		for(int64_t i = 0; i < alloc_blk_num; i++)
			addr_list[i] = INODE_BLKID_TO_DEVADDR(addr_list[i]);
	}
	if(error_info==3)
		exit(0);
}
void dealloc_inode(uint64_t dealloc_blk_info, int par_type)
{
	int64_t blk_id = 0;
	if(par_type == PAR_BLK_ID)
		blk_id = dealloc_blk_info;
	else if(par_type == PAR_BLK_ADDR)
		blk_id = INODE_DEVADDR_TO_BLKID(dealloc_blk_info);
	int error_info = do_dealloc(INODE_BMP, blk_id);
	if(error_info==4 || error_info==5)
		exit(0);
}

/* simple index node */
uint64_t alloc_single_idx_node(int return_type)
{
	uint64_t ret_addr_list[3] = {0};
	int error_info = do_alloc(IDX_NODE_BMP, 1, ret_addr_list);
	if(error_info==0)
	{
		if(return_type == RET_BLK_ID)
			return ret_addr_list[0];
		else if(return_type == RET_BLK_ADDR)
			return IDX_BLKID_TO_DEVADDR(ret_addr_list[0]);
		else
			printf("return type error!\n");
	}
	exit(0);
}
void alloc_idx_nodes(int64_t alloc_blk_num, uint64_t addr_list[], int return_type)
{
	int error_info = do_alloc(IDX_NODE_BMP, alloc_blk_num, addr_list);
	if(return_type == RET_BLK_ADDR)
	{
		for(int64_t i = 0; i < alloc_blk_num; i++)
			addr_list[i] = IDX_BLKID_TO_DEVADDR(addr_list[i]);
	}
	if(error_info==3)
		exit(0);
}
void dealloc_idx_node(uint64_t dealloc_blk_info, int par_type)
{
	int64_t blk_id = 0;
	if(par_type == PAR_BLK_ID)
		blk_id = dealloc_blk_info;
	else if(par_type == PAR_BLK_ADDR)
		blk_id = IDX_DEVADDR_TO_BLKID(dealloc_blk_info);
	int error_info = do_dealloc(IDX_NODE_BMP, blk_id);
	if(error_info==4 || error_info==5)
		exit(0);
}

/* virtural index node */
uint64_t alloc_single_viridx_node(int return_type)
{
	uint64_t ret_addr_list[3] = {0};
	int error_info = do_alloc(VIR_NODE_BMP, 1, ret_addr_list);
	if(error_info==0)
	{
		if(return_type == RET_BLK_ID)
			return ret_addr_list[0];
		else if(return_type == RET_BLK_ADDR)
			return VIRND_BLKID_TO_DEVADDR(ret_addr_list[0]);
		else
			printf("return type error!\n");
	}
	exit(0);
}
void alloc_viridx_nodes(int64_t alloc_blk_num, uint64_t addr_list[], int return_type)
{
	int error_info = do_alloc(VIR_NODE_BMP, alloc_blk_num, addr_list);
	if(return_type == RET_BLK_ADDR)
	{
		for(int64_t i = 0; i < alloc_blk_num; i++)
			addr_list[i] = VIRND_BLKID_TO_DEVADDR(addr_list[i]);
	}
	if(error_info==3)
		exit(0);
}
void dealloc_viridx_node(uint64_t dealloc_blk_info, int par_type)
{
	int64_t blk_id = 0;
	if(par_type == PAR_BLK_ID)
		blk_id = dealloc_blk_info;
	else if(par_type == PAR_BLK_ADDR)
		blk_id = VIRND_DEVADDR_TO_BLKID(dealloc_blk_info);
	int error_info = do_dealloc(VIR_NODE_BMP, blk_id);
	if(error_info==4 || error_info==5)
		exit(0);
}

/* buffer metadata node */
uint64_t alloc_single_bufmeta_node(int return_type)
{
	uint64_t ret_addr_list[3] = {0};
	int error_info = do_alloc(BUFMETA_BMP, 1, ret_addr_list);
	if(error_info==0)
	{
		if(return_type == RET_BLK_ID)
			return ret_addr_list[0];
		else if(return_type == RET_BLK_ADDR)
			return BUFMETA_BLKID_TO_DEVADDR(ret_addr_list[0]);
		else
			printf("return type error!\n");
	}
	exit(0);
}
void alloc_bufmeta_nodes(int64_t alloc_blk_num, uint64_t addr_list[], int return_type)
{
	int error_info = do_alloc(BUFMETA_BMP, alloc_blk_num, addr_list);
	if(return_type == RET_BLK_ADDR)
	{
		for(int64_t i = 0; i < alloc_blk_num; i++)
			addr_list[i] = BUFMETA_BLKID_TO_DEVADDR(addr_list[i]);
	}
	if(error_info==3)
		exit(0);
}
void dealloc_bufmeta_node(uint64_t dealloc_blk_info, int par_type)
{
	int64_t blk_id = 0;
	if(par_type == PAR_BLK_ID)
		blk_id = dealloc_blk_info;
	else if(par_type == PAR_BLK_ADDR)
		blk_id = BUFMETA_DEVADDR_TO_BLKID(dealloc_blk_info);
	int error_info = do_dealloc(BUFMETA_BMP, blk_id);
	if(error_info==4 || error_info==5)
		exit(0);
}

/* nvm page */
uint64_t alloc_single_nvm_page(int return_type)
{
	uint64_t ret_addr_list[3] = {0};
	int error_info = do_alloc(PAGE_BMP, 1, ret_addr_list);
	if(error_info==0)
	{
		if(return_type == RET_BLK_ID)
			return ret_addr_list[0];
		else if(return_type == RET_BLK_ADDR)
			return PAGE_BLKID_TO_DEVADDR(ret_addr_list[0]);
		else
			printf("return type error!\n");
	}
	exit(0);
}
void alloc_nvm_pages(int64_t alloc_blk_num, uint64_t addr_list[], int return_type)
{
	int error_info = do_alloc(PAGE_BMP, alloc_blk_num, addr_list);
	if(return_type == RET_BLK_ADDR)
	{
		for(int64_t i = 0; i < alloc_blk_num; i++)
			addr_list[i] = PAGE_BLKID_TO_DEVADDR(addr_list[i]);
	}
	if(error_info==3)
		exit(0);
}
void dealloc_nvm_page(uint64_t dealloc_blk_info, int par_type)
{
	int64_t blk_id = 0;
	if(par_type == PAR_BLK_ID)
		blk_id = dealloc_blk_info;
	else if(par_type == PAR_BLK_ADDR)
		blk_id = PAGE_DEVADDR_TO_BLKID(dealloc_blk_info);
	int error_info = do_dealloc(PAGE_BMP, blk_id);
	if(error_info==4 || error_info==5)
		exit(0);
}

/* ssd block */
uint64_t alloc_single_ssd_block(int return_type)
{
	uint64_t ret_addr_list[3] = {0};
	int error_info = do_alloc(BLOCK_BMP, 1, ret_addr_list);
	if(error_info==0)
	{
		if(return_type == RET_BLK_ID)
			return ret_addr_list[0];
		else if(return_type == RET_BLK_ADDR)
			return BLOCK_BLKID_TO_DEVADDR(ret_addr_list[0]);
		else
			printf("return type error!\n");
	}
	exit(0);
}
void alloc_ssd_blocks(int64_t alloc_blk_num, uint64_t addr_list[], int return_type)
{
	int error_info = do_alloc(BLOCK_BMP, alloc_blk_num, addr_list);
	if(return_type == RET_BLK_ADDR)
	{
		for(int64_t i = 0; i < alloc_blk_num; i++)
			addr_list[i] = BLOCK_BLKID_TO_DEVADDR(addr_list[i]);
	}
	if(error_info==3)
		exit(0);
}
void dealloc_ssd_block(uint64_t dealloc_blk_info, int par_type)
{
	int64_t blk_id = 0;
	if(par_type == PAR_BLK_ID)
		blk_id = dealloc_blk_info;
	else if(par_type == PAR_BLK_ADDR)
		blk_id = BLOCK_DEVADDR_TO_BLKID(dealloc_blk_info);
	int error_info = do_dealloc(BLOCK_BMP, blk_id);
	if(error_info==4 || error_info==5)
		exit(0);
}

#ifdef __cplusplus
}
#endif