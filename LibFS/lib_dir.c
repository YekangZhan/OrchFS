#include "lib_dir.h"
#include "index.h"
#include "meta_cache.h"
#include "lib_inode.h"
#include "orchfs.h"
#include "runtime.h"

int64_t all_read_size;

void free_path_arr(char** path_arr, int path_part_num)
{
    for(int i = 0; i < path_part_num; i++)
    {
        if(path_arr[i] != NULL)
            free(path_arr[i]);
    }
    free(path_arr);
    path_arr = NULL;
}

int count_path_part_num(char* pathname)
{
    int len = strlen(pathname);
    int path_part_num = 0;

    int pn_start = 0, pn_end = len-1;
    for(int i = 0; i < len; i++)
    {
        if(pathname[i] != '/')
        {
            pn_start = i; break;
        }
    }
    for(int i = len - 1; i >= 0; i--)
    {
        if(pathname[i] != '/')
        {
            pn_end = i; break;
        }
    }

    int now_filename_len = 1;
    for(int i = pn_start + 1; i <= pn_end + 1; i++)
    {
        if(i == pn_end + 1 || (pathname[i] == '/' && pathname[i-1] != '/'))
        {
            path_part_num++;
        }
    }
    return path_part_num;
}

void split_path(char* pathname, char* path_arr[])
{
    int len = strlen(pathname);
    
    int pn_start = 0, pn_end = len-1;
    for(int i = 0; i < len; i++)
    {
        if(pathname[i] != '/')
        {
            pn_start = i; break;
        }
    }
    for(int i = len - 1; i >= 0; i--)
    {
        if(pathname[i] != '/')
        {
            pn_end = i; break;
        }
    }

    int begin0 = pn_start, end0 = pn_start-1;
    int arr_cur = 0;
    for(int i = pn_start + 1; i <= pn_end + 1; i++)
    {
        if(i == pn_end + 1 || (pathname[i] == '/' && pathname[i-1] != '/'))
        {
            end0 = i-1;
            int filename_len = end0 - begin0 + 1;
            
            path_arr[arr_cur] = malloc(filename_len + 1);
            memcpy(path_arr[arr_cur], pathname + begin0, filename_len);
            path_arr[arr_cur][filename_len] = '\0';
            
            arr_cur++;
            begin0 = i+1;
        }
    }
}

/* Query the directory file and return the inode ID
 */
inode_id_t do_search_path(char* path_arr[], int path_part_num, inode_id_t start_dir_inoid)
{   
    pthread_rwlock_rdlock(&(orch_rt.dir_cache_lock));
    int64_t ret_inoid = search_dir_cache(orch_rt.dir_to_inoid_cache, path_arr,
                            path_part_num, start_dir_inoid);
    pthread_rwlock_unlock(&(orch_rt.dir_cache_lock));
    if(ret_inoid != -1)
        return ret_inoid;
    
    // Traverse parts of each path
    inode_id_t now_dir_inoid = start_dir_inoid;
    for(int i = 0; i < path_part_num ; i++)
    {
        int now_dir_fd = get_unused_fd(now_dir_inoid, O_RDWR);
        if(now_dir_fd == -1)
            goto dir_open_error;
        orch_inode_pt now_ino_pt = fd_to_inodept(now_dir_fd);
        
        // It must be a directory file
        if(now_ino_pt->i_type != DIR_FILE)
            return -1;
        
        int64_t now_dir_fsize = now_ino_pt->i_size;
        orch_dirent_pt dir_file_pt = malloc(now_dir_fsize);
        if(dir_file_pt == NULL)
            goto malloc_error;

        file_lock_rdlock(now_dir_fd);
        read_from_file(now_dir_fd, 0, now_dir_fsize, (void*)dir_file_pt);
        all_read_size += now_dir_fsize;
        file_lock_unlock(now_dir_fd);
        release_fd(now_dir_fd);

        int64_t dir_num = now_dir_fsize / DIRENT_SIZE;
        int find_flag = 0;
        for(int64_t j = 0; j < dir_num; j++)
        {
            if(dir_file_pt[j].d_type == INVALID_T)
                continue;
            int cmp_ans = strcmp(path_arr[i], dir_file_pt[j].d_name);
            
            if(cmp_ans == 0 && dir_file_pt[j].d_type != INVALID_T)
            {
                now_dir_inoid = dir_file_pt[j].d_ino;
                find_flag = 1;
                break;
            }
        }
        free(dir_file_pt);

        // not found
        if(find_flag == 0)
            goto not_found;
    }
    pthread_rwlock_wrlock(&(orch_rt.dir_cache_lock));
    insert_dir_cache(orch_rt.dir_to_inoid_cache, path_arr, path_part_num, 
                            start_dir_inoid, now_dir_inoid);
    pthread_rwlock_unlock(&(orch_rt.dir_cache_lock));
    return now_dir_inoid;
dir_open_error:
    printf("can not get dirent file fd!\n");
    exit(1);
not_found:
    return -1;
malloc_error:
    printf("alloc memory error!\n");
    exit(1);
}


int insert_dirent_entry(char* filename, inode_id_t insert_ino_id, inode_id_t dir_ino_id, int file_type)
{
    int now_dir_fd = get_unused_fd(dir_ino_id, O_RDWR);
    if(now_dir_fd == -1)
        goto dir_open_error;
    orch_inode_pt now_ino_pt = fd_to_inodept(now_dir_fd);

    if(now_ino_pt->i_type != DIR_FILE)
        goto not_dirent;
        
    int64_t now_dir_fsize = now_ino_pt->i_size;
    orch_dirent_pt dir_file_pt = malloc(now_dir_fsize);
    if(dir_file_pt == NULL)
        goto malloc_error;

    // create new direct
    orch_dirent_pt new_dir_pt = malloc(DIRENT_SIZE); 
    new_dir_pt->d_ino = insert_ino_id;
    new_dir_pt->d_off = now_dir_fsize;
    new_dir_pt->d_type = file_type;
    new_dir_pt->d_namelen = strlen(filename);
    strcpy(new_dir_pt->d_name, filename);

    // read direct file
    file_lock_wrlock(now_dir_fd);
    read_from_file(now_dir_fd, 0, now_dir_fsize, (void*)dir_file_pt);

    // check the same file
    int64_t dir_num = now_dir_fsize / DIRENT_SIZE;
    int64_t same_pos = -1;
    for(int64_t j = 0; j < dir_num; j++)
    {
        if(dir_file_pt[j].d_type == INVALID_T)
        {
            if(same_pos == -1)
                same_pos = j;
            continue;
            // break;
        }
        int cmp_ans = strcmp(filename, dir_file_pt[j].d_name);
        if(cmp_ans == 0 && dir_file_pt[j].d_type != INVALID_T)
        {
            printf("samefile: %s %s %d\n",dir_file_pt[j].d_name,filename,dir_file_pt[j].d_type);
            fflush(stdout);
            file_lock_unlock(now_dir_fd);
            goto same_file;
        }
    }

    if(same_pos == -1)
    {
        int64_t new_dir_fsize = now_ino_pt->i_size;
        write_into_file(now_dir_fd, new_dir_fsize, DIRENT_SIZE, new_dir_pt);
        inode_change_file_size(now_ino_pt, new_dir_fsize + DIRENT_SIZE);
        file_lock_unlock(now_dir_fd);
    }
    else if(same_pos >= 0)
    {
        write_into_file(now_dir_fd, same_pos*DIRENT_SIZE, DIRENT_SIZE, new_dir_pt);
        file_lock_unlock(now_dir_fd);
    }
    else
        goto same_pos_error;

    release_fd(now_dir_fd);
    free(dir_file_pt);
    free(new_dir_pt);
    return 0;
dir_open_error:
    printf("can not get dirent file fd!\n");
    exit(1);
same_file:
    printf("can not insert same file!\n");
    return 0;
not_dirent:
    printf("The file is not dirent file\n");
    exit(1);
malloc_error:
    printf("alloc memory error! --111\n");
    exit(1);
same_pos_error:
    printf("can not insert! -- same_pos_error\n");
    exit(1);
}


inode_id_t do_path_to_inode(char* pathname, inode_id_t start_dir_ino, int create_flag)
{
    int path_part_num = count_path_part_num(pathname);

    char** path_arr = malloc(path_part_num * sizeof(char*));
    for(int i = 0; i < path_part_num; i++)
        path_arr[i] = NULL;
    split_path(pathname, path_arr);
    
    pthread_rwlock_rdlock(&(orch_rt.dir_cache_lock));
    int64_t ret_inoid = search_dir_cache(orch_rt.dir_to_inoid_cache, path_arr,
                            path_part_num, start_dir_ino);
    pthread_rwlock_unlock(&(orch_rt.dir_cache_lock));
    if(ret_inoid != -1)
        return ret_inoid;

    inode_id_t target_inoid = do_search_path(path_arr, path_part_num-1, start_dir_ino);
    if(target_inoid == -1)
    {
        fprintf(stderr,"cannot search path! %s\n",pathname);
        free_path_arr(path_arr, path_part_num);
        return -1;
    }
    else
    {
        if(create_flag == NOT_CREATE_PATH)
        {
            // printf("not create!\n");
            int now_dir_fd = get_unused_fd(target_inoid, O_RDWR);
            if(now_dir_fd == -1)
                goto dir_open_error;
            orch_inode_pt now_ino_pt = fd_to_inodept(now_dir_fd);
            
            // Allocate space for directory files
            int64_t now_dir_fsize = now_ino_pt->i_size;
            orch_dirent_pt dir_file_pt = malloc(now_dir_fsize);
            if(dir_file_pt == NULL)
                goto malloc_error;

            file_lock_rdlock(now_dir_fd);
            read_from_file(now_dir_fd, 0, now_dir_fsize, (void*)dir_file_pt);
            all_read_size += now_dir_fsize;
            file_lock_unlock(now_dir_fd);
            release_fd(now_dir_fd);

            int64_t dir_num = now_dir_fsize / DIRENT_SIZE;
            int find_flag = 0, ino_ans = 0;
            for(int64_t j = 0; j < dir_num; j++)
            {
                if(dir_file_pt[j].d_type == INVALID_T)
                    continue;
                int cmp_ans = strcmp(path_arr[path_part_num-1], dir_file_pt[j].d_name);
                
                if(cmp_ans == 0 && dir_file_pt[j].d_type != INVALID_T)
                {
                    ino_ans = dir_file_pt[j].d_ino;
                    find_flag = 1;
                    break;
                }
            }
            free(dir_file_pt);
            if(find_flag == 0)
            {
                // printf("cannot search path in last level! %" PRId64 " %s\n",dir_num,path_arr[path_part_num-1]);
                // for(int64_t j = 0; j < dir_num; j++)
                //     printf("%s    ---    ",dir_file_pt[j].d_name);
                // printf("\n");
                // fflush(stdout);
                free_path_arr(path_arr, path_part_num);
                return -1;
            }
            else
            {
                free_path_arr(path_arr, path_part_num);
                return ino_ans;
            }
        }
        else
        {
            inode_id_t new_ino_id = inode_create(SIMPLE_FILE);
            // printf("new_ino_id1: %lld\n",new_ino_id);
            insert_dirent_entry(path_arr[path_part_num-1], new_ino_id, target_inoid, SIMPLE_FILE_T);

            pthread_rwlock_wrlock(&(orch_rt.dir_cache_lock));
            insert_dir_cache(orch_rt.dir_to_inoid_cache, path_arr, path_part_num, 
                                    start_dir_ino, new_ino_id);
            pthread_rwlock_unlock(&(orch_rt.dir_cache_lock));

            free_path_arr(path_arr, path_part_num);
            return new_ino_id;
        }
    }
    return -1;
dir_open_error:
    printf("can not get dirent file fd!\n");
    exit(1);
malloc_error:
    printf("alloc memory error! -222\n");
    exit(1);
}

/* 
Initialize a directory file
 */
void dir_file_init(inode_id_t dir_ino, inode_id_t father_dir_ino)
{
    int now_dir_fd = get_unused_fd(dir_ino, O_RDWR);
    if(now_dir_fd == -1)
        goto dir_open_error;
    orch_inode_pt now_ino_pt = fd_to_inodept(now_dir_fd);
    if(now_ino_pt->i_type != DIR_FILE)
        goto file_type_error;
   
    // add "." and ".."
    orch_dirent_pt new_dir_pt = malloc(DIRENT_SIZE * 2); 

    char this_fname[10] = {0};
    this_fname[0] = '.'; this_fname[1] = 0;
    new_dir_pt[0].d_ino = dir_ino;
    new_dir_pt[0].d_off = 0;
    new_dir_pt[0].d_type = DIRENT_FILE_T;
    new_dir_pt[0].d_namelen = 1;
    strcpy(new_dir_pt[0].d_name, this_fname);
    
    this_fname[0] = '.';  this_fname[1] = '.'; this_fname[2] = 0;
    new_dir_pt[1].d_ino = father_dir_ino;
    new_dir_pt[1].d_off = DIRENT_SIZE;
    new_dir_pt[1].d_type = DIRENT_FILE_T;
    new_dir_pt[1].d_namelen = 2;
    strcpy(new_dir_pt[1].d_name, this_fname);

    file_lock_wrlock(now_dir_fd);
    write_into_file(now_dir_fd, 0, DIRENT_SIZE*2, new_dir_pt);
    inode_change_file_size(now_ino_pt, DIRENT_SIZE*2);
    change_fd_file_offset(now_dir_fd, DIRENT_SIZE*2);
    file_lock_unlock(now_dir_fd);

    release_fd(now_dir_fd);
    return;
file_type_error:
    printf("The file type is not dirent file!\n");
    exit(1);
dir_open_error:
    printf("can not get dirent file fd!\n");
    exit(1);
}

/* Determine if the target directory file is empty
 */
int is_emtpy_dir(inode_id_t dir_inode)
{
    int dir_fd = get_unused_fd(dir_inode, O_RDWR);
    if(dir_fd == -1)
        goto dir_open_error;
    orch_inode_pt now_ino_pt = fd_to_inodept(dir_fd);
    if(now_ino_pt->i_type != DIR_FILE)
        goto file_type_error;
    
    // Read directory files
    int64_t dir_fsize = now_ino_pt->i_size;
    orch_dirent_pt dir_file_pt = malloc(dir_fsize); 
    file_lock_rdlock(dir_fd);
    read_from_file(dir_fd, 0, dir_fsize, (void*)dir_file_pt);
    all_read_size += dir_fsize;
    file_lock_unlock(dir_fd);
    release_fd(dir_fd);

    int64_t dir_num = dir_fsize / DIRENT_SIZE;
    int active_dir = 0;
    for(int64_t j = 0; j < dir_num; j++)
    {
        if(dir_file_pt[j].d_type != INVALID_T)
        {
            active_dir++;
        }
    }
    free(dir_file_pt);
    if(active_dir == 2)
        return EMPTY_DIR;
    else if(active_dir > 2)
        return NOT_EMPTY_DIR;
    else
        goto dir_file_error;
dir_file_error:
    release_fd(dir_fd);
    printf("The dir file data is error! --- is_emtpy_dir\n");
    return -1;
file_type_error:
    release_fd(dir_fd);
    printf("The file type is not dirent file! --- is_emtpy_dir\n");
    return -1;
dir_open_error:
    release_fd(dir_fd);
    printf("can not get dirent file fd! --- is_emtpy_dir\n");
    return -1;
}

int create_dirent(char* pathname)
{
    // printf("create path name: %s\n",pathname);
    // fflush(stdout);
    if(strlen(pathname) == 0)
        return -1;
    int path_part_num = count_path_part_num(pathname);
    char** path_arr = malloc(path_part_num * sizeof(char*));
    for(int i = 0; i < path_part_num; i++)
        path_arr[i] = NULL;
    split_path(pathname, path_arr);

    inode_id_t start_dir_ino = -1;
    if(pathname[0] == '/')
        start_dir_ino = orch_rt.mount_pt_inoid;
    else
        start_dir_ino = orch_rt.current_dir_inoid;

    inode_id_t target_inoid = do_search_path(path_arr, path_part_num-1, start_dir_ino);
    if(target_inoid == -1)
    {
        printf("can not create dirent file! --- pre dir not exist\n");
        free_path_arr(path_arr, path_part_num);
        return -1;
    }
    else
    {
        // Check for duplicate files
        int now_dir_fd = get_unused_fd(target_inoid, O_RDWR);
        if(now_dir_fd == -1)
            goto dir_open_error;
        
        orch_inode_pt now_ino_pt = fd_to_inodept(now_dir_fd);
        int64_t now_dir_fsize = now_ino_pt->i_size;
        orch_dirent_pt dir_file_pt = malloc(now_dir_fsize); 


        file_lock_rdlock(now_dir_fd);
        read_from_file(now_dir_fd, 0, now_dir_fsize, (void*)dir_file_pt);
        all_read_size += now_dir_fsize;
        file_lock_unlock(now_dir_fd);
        release_fd(now_dir_fd);

        // Traverse each directory
        int64_t dir_num = now_dir_fsize / DIRENT_SIZE;
        for(int64_t j = 0; j < dir_num; j++)
        {
            if(dir_file_pt[j].d_type == INVALID_T)
                continue;
            int cmp_ans = strcmp(path_arr[path_part_num-1], dir_file_pt[j].d_name);
            // printf("cmp_ans: %d %s\n",cmp_ans,dir_file_pt[j].d_name);
            if(cmp_ans == 0 && dir_file_pt[j].d_type != INVALID_T)
            {
                inode_id_t ino_next = dir_file_pt[j].d_ino;
                
                int next_dir_fd = get_unused_fd(ino_next, O_RDWR);
                orch_inode_pt next_ino_pt = fd_to_inodept(next_dir_fd);
                if(next_ino_pt->i_type == DIR_FILE)
                {
                    release_fd(next_dir_fd);
                    free(dir_file_pt);
                    free_path_arr(path_arr, path_part_num);
                    goto dir_file_exist;
                }
                else
                {
                    release_fd(next_dir_fd);
                    free(dir_file_pt);
                    free_path_arr(path_arr, path_part_num);
                    goto file_exist;
                }
            }
        }
        free(dir_file_pt);

        inode_id_t new_ino_id = inode_create(DIR_FILE);
        
        // Initialize this directory file
        dir_file_init(new_ino_id, target_inoid);

        insert_dirent_entry(path_arr[path_part_num-1], new_ino_id, target_inoid, DIRENT_FILE_T);
        free_path_arr(path_arr, path_part_num);
        return 0;
    }
    return -1;
file_exist:
    fprintf(stderr,"can not create dirent file, file exist! -- create_dirent\n");
    return -1;
dir_file_exist:
    // fprintf(stderr,"dir file exist! -- create_dirent\n");
    // return -1;
    return 0;
not_found:
    fprintf(stderr,"can not create dirent file! --- pre dir not exist\n");
    return -1;
dir_open_error:
    fprintf(stderr,"can not get dirent file fd!\n");
    exit(1);
}

inode_id_t path_to_inode(char* pathname, int create_flag)
{
    inode_id_t ret_inoid = -1;
    
    // Check if the path is legal
    if(strlen(pathname) == 0)
        goto dir_name_error;
    
    // Absolute path
    if(pathname[0] == '/')
    {
        ret_inoid = do_path_to_inode(pathname, orch_rt.mount_pt_inoid, create_flag);
    }
    // Relative path
    else if(pathname[0] != '/')
    {
        ret_inoid = do_path_to_inode(pathname, orch_rt.current_dir_inoid, create_flag);
    }
    return ret_inoid;
dir_name_error:
    printf("The path name is error!\n");
    exit(1);
}

inode_id_t path_to_inode_fromdir(inode_id_t start_ino_id, char* pathname, int create_flag)
{
    inode_id_t ret_inoid = -1;
    if(strlen(pathname) == 0)
        goto dir_name_error;
    if(pathname[0] != '/')
    {
        ret_inoid = do_path_to_inode(pathname, start_ino_id, create_flag);
    }
    else
        goto dir_name_error;
    return ret_inoid;
dir_name_error:
    printf("The path name is error!\n");
    exit(1);
}

int is_path_exist(char* pathname)
{
    if(strlen(pathname) == 0)
        return PATH_NOT_EXIST;
    inode_id_t ret_ino_id = path_to_inode(pathname, NOT_CREATE_PATH);
    if(ret_ino_id == -1)
        return PATH_NOT_EXIST;
    else
        return PATH_EXIST;
}

int delete_dirent(char* pathname, int ftype)
{
    if(strlen(pathname) == 0)
        return -1;
    
    // split the directory
    int path_part_num = count_path_part_num(pathname);
    char** path_arr = malloc(path_part_num * sizeof(char*));
    for(int i = 0; i < path_part_num; i++)
        path_arr[i] = NULL;
    split_path(pathname, path_arr);

    inode_id_t start_dir_ino = -1;
    if(pathname[0] == '/')
        start_dir_ino = orch_rt.mount_pt_inoid;
    else
        start_dir_ino = orch_rt.current_dir_inoid;

    inode_id_t target_inoid = do_search_path(path_arr, path_part_num-1, start_dir_ino);
    if(target_inoid == -1)
    {
        printf("can not delete dirent file! --- pre dir not exist\n");
        free_path_arr(path_arr, path_part_num);
        return -1;
    }
    else
    {
        int now_dir_fd = get_unused_fd(target_inoid, O_RDWR);
        if(now_dir_fd == -1)
            goto dir_open_error;
        orch_inode_pt now_ino_pt = fd_to_inodept(now_dir_fd);
        int64_t now_dir_fsize = now_ino_pt->i_size;
        orch_dirent_pt dir_file_pt = malloc(now_dir_fsize); 

        file_lock_rdlock(now_dir_fd);
        read_from_file(now_dir_fd, 0, now_dir_fsize, (void*)dir_file_pt);
        all_read_size += now_dir_fsize;
        file_lock_unlock(now_dir_fd);

        /* Get the number of directory entries and traverse 
            each directory to find the directory file to be deleted*/
        int64_t dir_num = now_dir_fsize / DIRENT_SIZE;
        int delete_flag = 0;
        
        for(int64_t j = 0; j < dir_num; j++)
        {
            if(dir_file_pt[j].d_type == INVALID_T)
                continue;
            int cmp_ans = strcmp(path_arr[path_part_num-1], dir_file_pt[j].d_name);
            if(cmp_ans == 0 && dir_file_pt[j].d_type != INVALID_T)
            {
                inode_id_t ino_next = dir_file_pt[j].d_ino;
                int next_dir_fd = get_unused_fd(ino_next, O_RDWR);
                orch_inode_pt next_ino_pt = fd_to_inodept(next_dir_fd);
                if(next_ino_pt->i_type == ftype && next_ino_pt->i_type == DIR_FILE)
                {
                    int empty_flag = is_emtpy_dir(ino_next);
                    if(empty_flag == EMPTY_DIR)
                    {
                        dir_file_pt[j].d_type = INVALID_T;
                        file_lock_rdlock(now_dir_fd);
                        write_into_file(now_dir_fd, DIRENT_SIZE*j, DIRENT_SIZE, dir_file_pt + j);
                        file_lock_unlock(now_dir_fd);
                        release_fd(now_dir_fd);
                        release_fd(next_dir_fd);

                        pthread_rwlock_wrlock(&(orch_rt.dir_cache_lock));
                        delete_dir_cache(orch_rt.dir_to_inoid_cache, path_arr, 
                                    path_part_num, start_dir_ino);
                        pthread_rwlock_unlock(&(orch_rt.dir_cache_lock));

                        free(dir_file_pt);
                        delete_flag = 1;
                        free_path_arr(path_arr, path_part_num);
                        return ino_next;
                    }
                    else
                    {
                        release_fd(now_dir_fd);
                        release_fd(next_dir_fd);
                        free(dir_file_pt);
                        free_path_arr(path_arr, path_part_num);
                        goto cannot_rm_file;
                    }
                }
                else if(next_ino_pt->i_type == ftype && next_ino_pt->i_type == SIMPLE_FILE)
                {
                    dir_file_pt[j].d_type = INVALID_T;
                    file_lock_rdlock(now_dir_fd);
                    write_into_file(now_dir_fd, DIRENT_SIZE*j, DIRENT_SIZE, dir_file_pt + j);
                    file_lock_unlock(now_dir_fd);
                    release_fd(now_dir_fd);
                    release_fd(next_dir_fd);
                    
                    pthread_rwlock_wrlock(&(orch_rt.dir_cache_lock));
                    delete_dir_cache(orch_rt.dir_to_inoid_cache, path_arr, 
                                    path_part_num, start_dir_ino);
                    pthread_rwlock_unlock(&(orch_rt.dir_cache_lock));

                    free(dir_file_pt);
                    free_path_arr(path_arr, path_part_num);
                    return ino_next;
                }
            }
        }
        if(delete_flag == 0)
        {
            release_fd(now_dir_fd);
            free(dir_file_pt);
            free_path_arr(path_arr, path_part_num);
            goto not_found;
        }
    }
    return -1;
file_type_error:
    printf("%s is not dirent file!\n",pathname);
    return -1;
cannot_rm_file:
    printf("can not delete non empty dir file! --- delete_dirent\n");
    return -1;
not_found:
    printf("can not delete target dirent file! --- delete_dirent\n");
    return -1;
dir_open_error:
    printf("can not get dirent file fd!\n");
    exit(1);
}

void file_rename(char* target_dir, char* target_fname, char* changed_fname)
{
    // Find the directory file that needs to be modified
    inode_id_t target_ino_id = path_to_inode(target_dir, NOT_CREATE_PATH);
    if(target_ino_id == -1)
        goto can_not_rename;
    
    int now_dir_fd = get_unused_fd(target_ino_id, O_RDWR);
    if(now_dir_fd == -1)
        goto dir_open_error;
    orch_inode_pt now_ino_pt = fd_to_inodept(now_dir_fd);

    // Verify that the file must be a directory file
    if(now_ino_pt->i_type != DIR_FILE)
        goto not_dirent;
        
    int64_t now_dir_fsize = now_ino_pt->i_size;
    orch_dirent_pt dir_file_pt = malloc(now_dir_fsize);
    if(dir_file_pt == NULL)
        goto malloc_error;

    // read the direct file
    file_lock_rdlock(now_dir_fd);
    read_from_file(now_dir_fd, 0, now_dir_fsize, (void*)dir_file_pt);
    all_read_size += now_dir_fsize;
    file_lock_unlock(now_dir_fd);

    int64_t dir_num = now_dir_fsize / DIRENT_SIZE;
    for(int64_t j = 0; j < dir_num; j++)
    {
        if(dir_file_pt[j].d_type == INVALID_T)
            continue;
        int cmp_ans = strcmp(target_fname, dir_file_pt[j].d_name);
        
        if(cmp_ans == 0 && dir_file_pt[j].d_type != INVALID_T)
        {
            dir_file_pt[j].d_namelen = strlen(changed_fname);
            memcpy(dir_file_pt[j].d_name, changed_fname, dir_file_pt[j].d_namelen+1);
            file_lock_wrlock(now_dir_fd);
            
            write_into_file(now_dir_fd, DIRENT_SIZE*j, DIRENT_SIZE, dir_file_pt + j);
            file_lock_unlock(now_dir_fd);
            break;
        }
    }

    release_fd(now_dir_fd);
    free(dir_file_pt);
    return;
not_dirent:
    printf("The target_dir is not dirent file\n");
    exit(1);
can_not_rename:
    printf("can not rename --- target_dir not found!\n");
    exit(1);
malloc_error:
    printf("alloc memory error!\n");
    exit(1);
dir_open_error:
    printf("can not get dirent file fd!\n");
    exit(1);
}
