#include "lib_inode.h"
#include "meta_cache.h"
#include "libspace.h"
#include "lib_log.h"
#include "orchfs.h"
#include "runtime.h"
#include "index.h"

#include "../config/log_config.h"
#include "../config/config.h"


void inode_file_resize(orch_inode_pt inode_pt, fsize_t new_file_size)
{
	
}

void inode_change_file_size(orch_inode_pt inode_pt, fsize_t new_file_size)
{
	pthread_spin_lock(&(inode_pt->i_lock));
	inode_pt->i_size = new_file_size;
	orch_time_stamp(&inode_pt->i_ctim);
	orch_time_stamp(&inode_pt->i_mtim);
	pthread_spin_unlock(&(inode_pt->i_lock));
	// TODO: Log record change information
}

void delete_inode(ino_id_t delete_ino_id)
{
	orch_inode_pt new_ino_pt = inodeid_to_memaddr(delete_ino_id);

	delete_all_index(new_ino_pt->i_idxroot, delete_ino_id);

	delete_file_metadata_cache(delete_ino_id);

	release_inode(delete_ino_id);

	write_delete_log(delete_ino_id, INODE_OP);
}

void sync_inode_and_index(ino_id_t sync_ino_id)
{
	sync_inode(sync_ino_id);
	orch_inode_pt new_ino_pt = inodeid_to_memaddr(sync_ino_id);
	sync_index_blk(new_ino_pt->i_idxroot);
	idx_root_pt sync_root_pt = indexid_to_memaddr(sync_ino_id, new_ino_pt->i_idxroot, CREATE);
	// if(sync_ino_id == 0)
	// 	printf("sync_root_pt  %lld %lld %lld\n",new_ino_pt->i_idxroot,
	// 		sync_root_pt, sync_root_pt->idx_entry_blkid);
	sync_index_blk(sync_root_pt->idx_entry_blkid);
	idx_nd_pt sync_idx_pt = indexid_to_memaddr(sync_ino_id, sync_root_pt->idx_entry_blkid, CREATE);
	sync_all_index(sync_idx_pt, sync_ino_id);
}

ino_id_t inode_create(ftype_t i_type)
{
	// Create a new inode and obtain its ID
	ino_id_t new_ino_id = require_inode_id();

	write_create_log(new_ino_id, INODE_OP);
	create_file_metadata_cache(new_ino_id);
	// fprintf(stderr,"create end!\n");

	orch_inode_pt new_ino_pt = inodeid_to_memaddr(new_ino_id);
	// fprintf(stderr,"new_ino_pt %lld\n",new_ino_pt);

	new_ino_pt->i_size = 0;
	new_ino_pt->i_idxroot = create_index(new_ino_id);
	new_ino_pt->i_nlink = 0;
	new_ino_pt->i_uid =	0;
	new_ino_pt->i_gid =	0;
	new_ino_pt->i_type = i_type;
	new_ino_pt->i_mode = S_IRWXU | S_IRWXG | S_IRWXO;
	orch_time_stamp(&new_ino_pt->i_atim);
	orch_time_stamp(&new_ino_pt->i_ctim);
	orch_time_stamp(&new_ino_pt->i_mtim);
	pthread_spin_init(&(new_ino_pt->i_lock), PTHREAD_PROCESS_SHARED);

	write_change_log(new_ino_id, INODE_OP, new_ino_pt, 0, ORCH_INODE_SIZE);

	close_file_metadata_cache(new_ino_id);
	return new_ino_id;
}