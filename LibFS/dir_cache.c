#include "dir_cache.h"
#include "../util/radixtree.h"

int count_path_all_len(char* path_arr[], int path_part_num)
{
    int ret_len = 0;
    for(int i = 0; i < path_part_num; i++)
    {
        // printf("ret_len  %d %s\n",ret_len,path_arr[i] );
        int len = strlen(path_arr[i]);
        // printf("len %d\n",len);
        ret_len += len;
    }
    return ret_len;
}

dir_cache_pt init_dir_cache()
{
    dir_cache_pt ret_dir_cache = malloc(sizeof(dir_cache_t));
    ret_dir_cache->now_dir_sum = 0;
    ret_dir_cache->cache_rdtree_pt = init_radix_tree();
    return ret_dir_cache;
}

void insert_dir_cache(dir_cache_pt dir_cache, char* path_arr[], 
                    int path_part_num, int64_t start_dir_inoid, int64_t file_inoid)
{
    int all_path_len = count_path_all_len(path_arr, path_part_num) + sizeof(int64_t);
    void* key_sp = malloc(all_path_len);
    int64_t* key_sp_int64 = key_sp;
    key_sp_int64[0] = start_dir_inoid;
    void* now_sp_pt = key_sp + sizeof(int64_t);
    for(int i = 0; i < path_part_num; i++)
    {
        int len = strlen(path_arr[i]);
        memcpy(now_sp_pt, path_arr[i], len);
        now_sp_pt += len;
    }

    int64_t* value = malloc(sizeof(int64_t));
    value[0] = file_inoid;

    insert_kv_into_rdtree(dir_cache->cache_rdtree_pt, key_sp, all_path_len, value, sizeof(int64_t));
    dir_cache->now_dir_sum++;
    free(value);
    return;
}

void delete_dir_cache(dir_cache_pt dir_cache, char* path_arr[], 
                    int path_part_num, int64_t start_dir_inoid)
{
    int all_path_len = count_path_all_len(path_arr, path_part_num) + sizeof(int64_t);
    void* key_sp = malloc(all_path_len);
    int64_t* key_sp_int64 = key_sp;
    key_sp_int64[0] = start_dir_inoid;
    void* now_sp_pt = key_sp + sizeof(int64_t);
    for(int i = 0; i < path_part_num; i++)
    {
        int len = strlen(path_arr[i]);
        memcpy(now_sp_pt, path_arr[i], len);
        now_sp_pt += len;
    }

    delete_kv_from_rdtree(dir_cache->cache_rdtree_pt, key_sp, all_path_len);
    dir_cache->now_dir_sum--;
}

int64_t search_dir_cache(dir_cache_pt dir_cache, char* path_arr[], 
                    int path_part_num, int64_t start_dir_inoid)
{
    int all_path_len = count_path_all_len(path_arr, path_part_num) + sizeof(int64_t);
    void* key_sp = malloc(all_path_len);
    int64_t* key_sp_int64 = key_sp;
    key_sp_int64[0] = start_dir_inoid;
    void* now_sp_pt = key_sp + sizeof(int64_t);
    for(int i = 0; i < path_part_num; i++)
    {
        int len = strlen(path_arr[i]);
        memcpy(now_sp_pt, path_arr[i], len);
        now_sp_pt += len;
    }

    int64_t* value = search_from_rdtree(dir_cache->cache_rdtree_pt, key_sp, all_path_len);
    if(value == NULL)
        return -1;
    else
        return value[0];
}

void free_dir_cache(dir_cache_pt dir_cache)
{
    dir_cache->now_dir_sum = 0;
    free_radix_tree(dir_cache->cache_rdtree_pt);
}