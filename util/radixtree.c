#include "radixtree.h"

radix_node_pt create_new_radix_node()
{
    radix_node_pt ret_pt = malloc(sizeof(radix_node_t));
    ret_pt->value_len = 0;
    ret_pt->dirty_flag = NOT_DIRTY;
    ret_pt->value = NULL;
    for(int i = 0; i < RDTREE_SON_NUM; i++)
        ret_pt->next_level_pt[i] = NULL;
    return ret_pt;
}

radix_node_pt init_radix_tree()
{
    return create_new_radix_node();
}

void insert_kv_into_rdtree(radix_node_pt root, void* key, int64_t key_len, 
                            void* value, int64_t value_len)
{
    char* key_char = key;
    radix_node_pt now_rdpt = root;
    void* value_sp = malloc(value_len);
    memcpy(value_sp, value, value_len);
    for(int i = 0; i < key_len; i++)
    {
        int now_key_info = (int)(key_char[i]);
        if(now_rdpt->next_level_pt[now_key_info] == NULL)
        {
            now_rdpt->next_level_pt[now_key_info] = create_new_radix_node();
        }
        now_rdpt = now_rdpt->next_level_pt[now_key_info];
    }
    now_rdpt->dirty_flag = NOT_DIRTY;
    now_rdpt->value_len = value_len;
    now_rdpt->value = value_sp;
}

void* search_from_rdtree(radix_node_pt root, void* key, int64_t key_len)
{
    char* key_char = key;
    radix_node_pt now_rdpt = root;
    for(int i = 0; i < key_len; i++)
    {
        int now_key_info = (int)(key_char[i]);
        if(now_rdpt->next_level_pt[now_key_info] == NULL)
        {
            goto not_find;
        }
        now_rdpt = now_rdpt->next_level_pt[now_key_info];
    }
    if(now_rdpt->dirty_flag == NOT_DIRTY)
        return now_rdpt->value;
    else
        return NULL;
not_find:
    return NULL;
}

void delete_kv_from_rdtree(radix_node_pt root, void* key, int64_t key_len)
{
    char* key_char = key;
    radix_node_pt now_rdpt = root;
    for(int i = 0; i < key_len; i++)
    {
        int now_key_info = (int)(key_char[i]);
        if(now_rdpt->next_level_pt[now_key_info] == NULL)
        {
            goto not_find;
        }
        now_rdpt = now_rdpt->next_level_pt[now_key_info];
    }
    if(now_rdpt->dirty_flag == NOT_DIRTY)
    {
        now_rdpt->dirty_flag = DIRTY;
        free(now_rdpt->value);
        now_rdpt->value = NULL;
        now_rdpt->value_len = 0;
    }
    return;
not_find:
    // printf("can not delete the key-value pair! -- delete_kv_from_rdtree\n");
    return;
}

void change_kv_in_rdtree(radix_node_pt root, void* key, int64_t key_len, 
                        void* value, int64_t value_len)
{
    char* key_char = key;
    radix_node_pt now_rdpt = root;
    for(int i = 0; i < key_len; i++)
    {
        int now_key_info = (int)(key_char[i]);
        if(now_rdpt->next_level_pt[now_key_info] == NULL)
        {
            goto not_find;
        }
        now_rdpt = now_rdpt->next_level_pt[now_key_info];
    }
    if(now_rdpt->dirty_flag == NOT_DIRTY)
    {
        void* new_value_sp = malloc(value_len);
        memcpy(new_value_sp, value, value_len);
        free(now_rdpt->value);
        now_rdpt->value = new_value_sp;
        now_rdpt->value_len = value_len;
    }
    else
        goto not_find;
    return;
not_find:
    printf("can not change the key-value pair! -- change_kv_in_rdtree\n");
    return;
}

void free_radix_tree(radix_node_pt rdtree_root)
{
    radix_node_pt now_rdpt = rdtree_root;
    for(int i = 0; i < RDTREE_SON_NUM; i++)
    {
        if(now_rdpt->next_level_pt[i] != NULL)
        {
            free_radix_tree(now_rdpt->next_level_pt[i]);
        }
    }
    if(now_rdpt->value != NULL)
        free(now_rdpt->value);
    free(now_rdpt);
}