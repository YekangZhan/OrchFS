#include "orch_list.h"

orch_list_pt init_list()
{
    orch_list_pt new_list = malloc(sizeof(orch_list_t));
    if(new_list == NULL)
        return NULL;
    new_list->list_len = 0;
    new_list->list_head = NULL;
    new_list->list_end = NULL;
    return new_list;
}

void* search_list_data(orch_list_pt target_list, int64_t search_key)
{
    list_node_pt iter_pt = target_list->list_head;
    for(int64_t i = 0; i < target_list->list_len; i++)
    {
        if(iter_pt == NULL)
        {
            // printf("%lld %lld %lld\n",i,target_list->list_len,search_key);
            goto error;
        }
        // printf("%lld %lld %lld %lld\n",target_list,i,iter_pt->key,search_key);
        if(iter_pt->key == search_key)
            return iter_pt->node_data;
        iter_pt = iter_pt->next_pt;
    }
    return NULL;
error:
    printf("list structure error -- search_list_data!\n");
    exit(0);
}


int insert_into_list(orch_list_pt target_list, int64_t insert_key, void* insert_data, int64_t data_len)
{
    // printf("insert %lld\n",target_list);
    list_node_pt add_node_pt = malloc(sizeof(list_node_t));
    if(add_node_pt == NULL)
        goto error;
    add_node_pt->key = insert_key;
    add_node_pt->next_pt = NULL;
    add_node_pt->node_data = malloc(data_len);
    if(add_node_pt->node_data == NULL)
        goto error;
    memcpy(add_node_pt->node_data, insert_data, data_len);

    // if(insert_key == 88761253)
    //     printf("ppp: %lld\n", target_list->list_len);
    if(target_list->list_head == NULL)
    {
        target_list->list_head = add_node_pt;
        target_list->list_end = add_node_pt;
    }
    else
    {
        target_list->list_end->next_pt = add_node_pt;
        target_list->list_end = add_node_pt;
    }
    target_list->list_len++;
    // if(insert_key == 88761253)
    //     printf("ppp2: %lld\n", target_list->list_len);
    return 0;
error:
    printf("alloc memory error -- insert_into_list!\n");
    return -1;
}   

int insert_into_list_head(orch_list_pt target_list, int64_t insert_key, void* insert_data, int64_t data_len)
{
    // printf("insert %lld\n",target_list);
    list_node_pt add_node_pt = malloc(sizeof(list_node_t));
    if(add_node_pt == NULL)
        goto error;
    add_node_pt->key = insert_key;
    add_node_pt->next_pt = NULL;
    add_node_pt->node_data = malloc(data_len);
    if(add_node_pt->node_data == NULL)
        goto error;
    memcpy(add_node_pt->node_data, insert_data, data_len);

    if(target_list->list_head == NULL)
    {
        target_list->list_head = add_node_pt;
        target_list->list_end = add_node_pt;
    }
    else
    {
        add_node_pt->next_pt = target_list->list_head;
        target_list->list_head = add_node_pt->next_pt;
    }
    target_list->list_len++;
    return 0;
error:
    printf("alloc memory error -- insert_into_list_head!\n");
    return -1;
}

int change_list_node(orch_list_pt target_list, int64_t change_key, void* change_data, int64_t data_len)
{
    list_node_pt iter_pt = target_list->list_head;
    for(int64_t i = 0; i < target_list->list_len; i++)
    {
        if(iter_pt == NULL)
            goto error;
        if(iter_pt->key == change_key)
        {
            free(iter_pt->node_data);
            iter_pt->node_data = malloc(data_len);
            if(iter_pt->node_data == NULL)
                goto error1;
            memcpy(iter_pt->node_data, change_data, data_len);
            return 0;
        }
        iter_pt = iter_pt->next_pt;
    }
    return -1;
error:
    printf("list structure error -- change_list_node!\n");
    exit(0);
error1:
    printf("alloc memory error -- change_list_node!\n");
    exit(0);
}

int delete_list_node(orch_list_pt target_list, int64_t delete_key)
{
    // printf("delete %lld\n",target_list);
    list_node_pt iter_pt = target_list->list_head;
    list_node_pt pre_pt = NULL;
    for(int64_t i = 0; i < target_list->list_len; i++)
    {
        if(iter_pt == NULL)
            goto error;
        if(iter_pt->key == delete_key)
        {
            list_node_pt trans_pt = iter_pt;
            if(pre_pt == NULL)
                target_list->list_head = iter_pt->next_pt;
            else
                pre_pt->next_pt = iter_pt->next_pt;
            if(iter_pt == target_list->list_end)
                target_list->list_end = pre_pt;
            free(trans_pt->node_data);
            free(trans_pt);
            target_list->list_len--;
            return 0;
        }
        pre_pt = iter_pt;
        iter_pt = iter_pt->next_pt;
    }
    return -1;
error:
    printf("list structure error -- delete_list_node!\n");
    exit(0);
}

void free_list(orch_list_pt target_list)
{
    if(target_list == NULL)
        return;
    list_node_pt iter_pt = target_list->list_head;
    for(int64_t i = 0; i < target_list->list_len; i++)
    {
        if(iter_pt == NULL)
            goto error;
        list_node_pt trans_pt = iter_pt;
        iter_pt = iter_pt->next_pt;
        free(trans_pt->node_data);
        free(trans_pt);
    }
    return;
error:
    printf("list structure error -- free_list!\n");
    exit(0);
}