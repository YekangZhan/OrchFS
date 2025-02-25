#include "runtime.h"
#include "lib_inode.h"
#include "meta_cache.h"
#include "lib_socket.h"
#include "../util/hashtable.h"
#include "dir_cache.h"
#include "migrate.h"

void init_runtime_info()
{
    orch_rt.log_start_pt = NULL;
    orch_rt.bufmeta_start_pt = NULL;
    for(int i = 0; i < MAX_OPEN_INO_NUM; i++)
    {
        orch_rt.open_inot[i].used_flag = INOT_NOT_USED;
        orch_rt.open_inot[i].inode_pt = NULL;
        orch_rt.open_inot[i].inode_id = -1;
        orch_rt.open_inot[i].fd_link_count = 0;
        pthread_rwlock_init(&(orch_rt.open_inot[i].file_rw_lock), NULL);
    }
    for(int i = 0; i < ORCH_MAX_FD; i++)
    {
        orch_rt.fd_info[i].used_flag = FD_NOT_USED;
        orch_rt.fd_info[i].open_inot_idx = -1;
    }
    orch_rt.mount_pt_inoid = -1;
    pthread_spin_init(&(orch_rt.req_kernel_lock), PTHREAD_PROCESS_SHARED);
    pthread_spin_init(&(orch_rt.inot_lock), PTHREAD_PROCESS_SHARED);
    pthread_spin_init(&(orch_rt.fd_lock), PTHREAD_PROCESS_SHARED);
    orch_rt.now_fd_cur = 0;
    orch_rt.now_inot_cur = 0;
    orch_rt.inoid_to_inotidx = init_hashtable(MAX_OPEN_INO_NUM);

    // 初始化目录缓存
    pthread_rwlock_init(&(orch_rt.dir_cache_lock), NULL);
    orch_rt.dir_to_inoid_cache = init_dir_cache();

    // 初始化迁移的相关信息
    orch_rt.mig_rtinfo_pt = init_migrate_info();
    // fprintf(stderr,"check info %lld\n",orch_rt.mig_rtinfo_pt->migrate_num);

    orch_rt.index_time = 0;
    orch_rt.io_time = 0;
    orch_rt.pm_unit_rwsize = 0;
    orch_rt.pm_page_rwsize = 0;
    orch_rt.wback_time = 0;
    orch_rt.pm_time = 0;
    orch_rt.blk_time = 0;

    orch_rt.used_pm_pages = 0;
    orch_rt.used_pm_units = 0;
    orch_rt.used_ssd_blks = 0;
    orch_rt.used_tree_nodes = 0;
    orch_rt.used_vir_nodes = 0;
    orch_rt.wback_time = 0;
}

void free_runtime_info()
{
    free_hashtable(orch_rt.inoid_to_inotidx);
    free_dir_cache(orch_rt.dir_to_inoid_cache);
    free_migrate_info(orch_rt.mig_rtinfo_pt);
}

void change_current_dir_ino(int64_t root_ino_id, int64_t inode_id)
{
    orch_rt.current_dir_inoid = inode_id;
    orch_rt.mount_pt_inoid = root_ino_id;
}

void file_lock_rdlock(int fd)
{
    int inot_idx = -1;
    // pthread_spin_lock(&(orch_rt.fd_lock));
    if(orch_rt.fd_info[fd].used_flag == FD_USED)
    {
        inot_idx = orch_rt.fd_info[fd].open_inot_idx;
        // pthread_spin_unlock(&(orch_rt.fd_lock));
        if(orch_rt.open_inot[inot_idx].used_flag == INOT_USED && inot_idx >= 0 && inot_idx < MAX_OPEN_INO_NUM)
            pthread_rwlock_rdlock(&(orch_rt.open_inot[inot_idx].file_rw_lock));
        else
            goto file_not_open;
    }
    else
    {
        // pthread_spin_unlock(&(orch_rt.fd_lock));
        goto fd_error;
    }
    return;
fd_error:
    printf("The fd is error! -- lock_rdlock %d\n",fd);
    exit(1);
file_not_open:
    printf("The target file is not open! -- lock_rdlock %d %d\n",fd,inot_idx);
    exit(1);
}

void file_lock_wrlock(int fd)
{
    int inot_idx = -1;
    // pthread_spin_lock(&(orch_rt.fd_lock));
    if(orch_rt.fd_info[fd].used_flag == FD_USED)
    {
        inot_idx = orch_rt.fd_info[fd].open_inot_idx;
        // pthread_spin_unlock(&(orch_rt.fd_lock));
        if(orch_rt.open_inot[inot_idx].used_flag == INOT_USED && inot_idx >= 0 && inot_idx < MAX_OPEN_INO_NUM)
            pthread_rwlock_wrlock(&(orch_rt.open_inot[inot_idx].file_rw_lock));
        else
            goto file_not_open;
    }
    else
    {
        // pthread_spin_unlock(&(orch_rt.fd_lock));
        goto fd_error;
    }
    return;
fd_error:
    printf("The fd is error! -- lock_wrlock %d\n",fd);
    exit(1);
file_not_open:
    printf("The target file is not open! -- lock_wrlock %d %d\n",fd,inot_idx);
    exit(1);
}

void file_lock_unlock(int fd)
{
    // pthread_spin_lock(&(orch_rt.fd_lock));
    int inot_idx = -1;
    if(orch_rt.fd_info[fd].used_flag == FD_USED)
    {
        // pthread_spin_unlock(&(orch_rt.fd_lock));
        inot_idx = orch_rt.fd_info[fd].open_inot_idx;
        if(orch_rt.open_inot[inot_idx].used_flag == INOT_USED && inot_idx >= 0 && inot_idx < MAX_OPEN_INO_NUM)
            pthread_rwlock_unlock(&(orch_rt.open_inot[inot_idx].file_rw_lock));
        else
            goto file_not_open;
    }
    else
    {
        // pthread_spin_unlock(&(orch_rt.fd_lock));
        goto fd_error;
    }
    return;
fd_error:
    printf("The fd is error! -- lock_unlock %d\n",fd);
    exit(1);
file_not_open:
    printf("The target file is not open! -- lock_unlock %d %d\n",fd,inot_idx);
    exit(1);
}

orch_inode_pt fd_to_inodept(int fd)
{
    // pthread_spin_lock(&(orch_rt.fd_lock));
    int inot_idx = -1;
    if(orch_rt.fd_info[fd].used_flag == FD_USED)
    {
        inot_idx = orch_rt.fd_info[fd].open_inot_idx;
        // pthread_spin_unlock(&(orch_rt.fd_lock));
        if(orch_rt.open_inot[inot_idx].used_flag == INOT_USED && inot_idx >= 0 && inot_idx < MAX_OPEN_INO_NUM)
        {
            return orch_rt.open_inot[inot_idx].inode_pt;
        }
        else
        {
            goto file_not_open;
        }
    }
    else
    {
        // pthread_spin_unlock(&(orch_rt.fd_lock));
        goto fd_error;
    }
    return NULL;
fd_error:
    printf("The fd is error! -- fd_to_inodept\n");
    exit(1);
file_not_open:
    printf("The target file is not open! -- fd_to_inodept %d %d\n",fd,inot_idx);
    exit(1);
}

int64_t fd_to_inodeid(int fd)
{
    // pthread_spin_lock(&(orch_rt.fd_lock));
    int inot_idx = -1;
    if(orch_rt.fd_info[fd].used_flag == FD_USED)
    {
        inot_idx = orch_rt.fd_info[fd].open_inot_idx;
        // pthread_spin_unlock(&(orch_rt.fd_lock));
        // pthread_spin_lock(&(orch_rt.inot_lock));
        if(orch_rt.open_inot[inot_idx].used_flag == INOT_USED && inot_idx >= 0 && inot_idx < MAX_OPEN_INO_NUM)
        {
            // pthread_spin_unlock(&(orch_rt.inot_lock));
            return orch_rt.open_inot[inot_idx].inode_id;
        }
        else
        {
            // pthread_spin_unlock(&(orch_rt.inot_lock));
            goto file_not_open;
        }
    }
    else
    {
        // pthread_spin_unlock(&(orch_rt.fd_lock));
        goto fd_error;
    }
    return -1;
fd_error:
    printf("The fd is error! -- fd_to_inodeid\n");
    exit(1);
file_not_open:
    printf("The target file is not open! -- fd_to_inodeid %d %d\n",fd,inot_idx);
    exit(1);
}

int64_t get_fd_file_offset(int fd)
{
    // pthread_spin_lock(&(orch_rt.fd_lock));
    if(orch_rt.fd_info[fd].used_flag == FD_USED)
    {
        return orch_rt.fd_info[fd].rw_offset;
    }
    else
    {
        // pthread_spin_unlock(&(orch_rt.fd_lock));
        goto fd_error;
    }
    return -1;
fd_error:
    printf("The fd is error! -- get_fd_file_offset\n");
    exit(1);
}

void change_fd_file_offset(int fd, int64_t changed_offset)
{
    assert(changed_offset >= 0);
    // pthread_spin_lock(&(orch_rt.fd_lock));
    if(orch_rt.fd_info[fd].used_flag == FD_USED)
    {
        orch_rt.fd_info[fd].rw_offset = changed_offset;
        // int inot_idx = orch_rt.fd_info[fd].open_inot_idx;
        // // pthread_spin_unlock(&(orch_rt.fd_lock));
        // pthread_spin_lock(&(orch_rt.inot_lock));
        // if(orch_rt.open_inot[inot_idx].used_flag == INOT_USED)
        // {
        //     orch_rt.open_inot[inot_idx].rw_offset = changed_offset;
        //     pthread_spin_unlock(&(orch_rt.inot_lock));
        //     return;
        // }
        // else
        // {
        //     pthread_spin_unlock(&(orch_rt.inot_lock));
        //     goto file_not_open;
        // }
    }
    else
    {
        // pthread_spin_unlock(&(orch_rt.fd_lock));
        goto fd_error;
    }
    return;
fd_error:
    printf("The fd is error! -- change_fd_file_offset\n");
    exit(1);
// file_not_open:
//     printf("The target file is not open! -- change_fd_file_offset\n");
//     exit(1);
}

int inode_open(int64_t inode_id, int mode)
{
    int ino_open_flag = 0, inot_idx = 0;
    int64_t* idxid_pt = (int64_t*)search_kv_from_hashtable(orch_rt.inoid_to_inotidx, inode_id);
    if(idxid_pt != NULL)
    {
        ino_open_flag = 1; inot_idx = (*idxid_pt);
        orch_rt.open_inot[inot_idx].fd_link_count += 1;
    }
    if(ino_open_flag == 0)
    {
        for(int i = 0; i < MAX_OPEN_INO_NUM; i++)
        {
            int now_ino_cur = (orch_rt.now_inot_cur+i) % MAX_OPEN_INO_NUM;
            if(orch_rt.open_inot[now_ino_cur].used_flag == INOT_NOT_USED)
            {
                create_file_metadata_cache(inode_id);
                // fprintf(stderr,"create end!\n");
                orch_rt.open_inot[now_ino_cur].inode_pt = inodeid_to_memaddr(inode_id);
                // fprintf(stderr,"open begin: %lld %d %lld\n",inode_id,i,orch_rt.open_inot[now_ino_cur].inode_pt);
                pthread_spin_init(&(orch_rt.open_inot[now_ino_cur].inode_pt->i_lock), PTHREAD_PROCESS_SHARED);
                // fprintf(stderr,"create end2!\n");
                if(orch_rt.open_inot[now_ino_cur].inode_pt == NULL)
                    goto inode_id_error;
                // printf("itype: %lld %lld\n",inode_id, orch_rt.open_inot[i].inode_pt->i_type);
                orch_rt.open_inot[now_ino_cur].used_flag = INOT_USED;
                orch_rt.open_inot[now_ino_cur].inode_id = inode_id;
                // orch_rt.open_inot[i].rw_offset = 0;
                orch_rt.open_inot[now_ino_cur].open_mode = mode;
                orch_rt.open_inot[now_ino_cur].fd_link_count = 1;
                pthread_rwlock_init(&(orch_rt.open_inot[now_ino_cur].file_rw_lock), NULL);
                orch_rt.now_inot_cur = (orch_rt.now_inot_cur+1)%MAX_OPEN_INO_NUM;

                int64_t* value_idxid = malloc(sizeof(int64_t));
                (*value_idxid) = now_ino_cur;
                add_kv_into_hashtable(orch_rt.inoid_to_inotidx, inode_id, value_idxid, sizeof(int64_t));
                free(value_idxid);
                return now_ino_cur;
            }
        }
        goto open_inode_num_error;
    }
    else if(ino_open_flag == 1)
    {
        return inot_idx;
    }
inode_id_error:
    printf("The inode id id error! --- open\n");
    exit(1);
open_inode_num_error:
    printf("The open inode is too many\n");
    exit(1);
}


void inode_close(int64_t inode_id)
{
    int64_t* idxid_pt = (int64_t*)search_kv_from_hashtable(orch_rt.inoid_to_inotidx, inode_id);
    if(idxid_pt == NULL)
        goto inode_id_error;
    int64_t idxid = (*idxid_pt);
    if(orch_rt.open_inot[idxid].used_flag == INOT_USED)
    {

        pthread_rwlock_destroy(&(orch_rt.open_inot[idxid].file_rw_lock));
        orch_rt.open_inot[idxid].used_flag = INOT_NOT_USED;
        orch_rt.open_inot[idxid].inode_pt = NULL;
        orch_rt.open_inot[idxid].fd_link_count = 0;
        close_file_metadata_cache(inode_id);
        delete_kv_from_hashtable(orch_rt.inoid_to_inotidx, inode_id);
        // printf("close begin: %lld %d %d\n",inode_id,i,orch_rt.open_inot[i-1].used_flag);
        return;
    }
    goto inode_id_error;
inode_id_error:
    printf("The inode id id error! --- close\n");
    exit(1);
}

int64_t get_unused_fd(int64_t inode_id, int mode)
{
    // fprintf(stderr,"get_unused_fd: %" PRId64 " \n",inode_id);
    pthread_spin_lock(&(orch_rt.fd_lock));
    for(int i = 0; i < ORCH_MAX_FD; i++)
    {
        int now_fd_cur = (orch_rt.now_fd_cur+i)%ORCH_MAX_FD;
        if(orch_rt.fd_info[now_fd_cur].used_flag == FD_NOT_USED)
        {
            pthread_spin_lock(&(orch_rt.inot_lock));
            int64_t inot_idx = inode_open(inode_id, mode);
            pthread_spin_unlock(&(orch_rt.inot_lock));

            orch_rt.fd_info[now_fd_cur].used_flag = FD_USED;
            orch_rt.fd_info[now_fd_cur].rw_offset = 0;
            orch_rt.fd_info[now_fd_cur].flags = 0;
            orch_rt.fd_info[now_fd_cur].open_inot_idx = inot_idx;
            pthread_spin_unlock(&(orch_rt.fd_lock));
            // fprintf(stderr,"unlock: %d\n",i);
            orch_rt.now_fd_cur = (now_fd_cur+1)%ORCH_MAX_FD;
            return now_fd_cur;
        }
    }
    pthread_spin_unlock(&(orch_rt.fd_lock));
    goto open_file_num_error;
open_file_num_error:
    printf("The open file is too much\n");
    fflush(stdout);
    assert(1==0);
    return -1;
}

void release_fd(int fd)
{
    if(fd < 0 || fd >= ORCH_MAX_FD)
        goto fd_error;
    // printf("release fd: %d\n",fd);
    pthread_spin_lock(&(orch_rt.fd_lock));
    // fprintf(stderr,"relock: %d\n",fd);
    if(orch_rt.fd_info[fd].used_flag == FD_USED)
    {
        orch_rt.fd_info[fd].used_flag = FD_NOT_USED;
        int inot_idx = orch_rt.fd_info[fd].open_inot_idx;
        
        pthread_spin_lock(&(orch_rt.inot_lock));
        // fprintf(stderr,"ino relock: %d\n",fd);
        orch_rt.open_inot[inot_idx].fd_link_count -= 1;
        // 同步数据
#ifdef FILEBENCH
        if(lib_register_pid == 0)
        {
            // printf("sync inoid %lld\n",inode_id);
            sync_inode_and_index(orch_rt.open_inot[inot_idx].inode_id);
        }
#endif
        if(orch_rt.open_inot[inot_idx].fd_link_count == 0)
        {
            // pthread_spin_unlock(&(orch_rt.inot_lock));
            inode_close(orch_rt.open_inot[inot_idx].inode_id);
        }
        // fprintf(stderr,"ino reunlock: %d\n",fd);
        pthread_spin_unlock(&(orch_rt.inot_lock));
        orch_rt.fd_info[fd].open_inot_idx = -1;
    }
    else if(orch_rt.fd_info[fd].used_flag == FD_NOT_USED)
        goto can_not_release;
    pthread_spin_unlock(&(orch_rt.fd_lock));
    // fprintf(stderr,"reunlock: %d\n",fd);
    return;
fd_error:
    pthread_spin_unlock(&(orch_rt.fd_lock));
    printf("The file discripter value is error!\n");
    exit(1);
can_not_release:
    pthread_spin_unlock(&(orch_rt.fd_lock));
    printf("The file discripter can not release!\n");
    return;
}