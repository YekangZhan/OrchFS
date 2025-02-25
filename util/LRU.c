#include "LRU.h"
#include "hashtable.h"


LRU_pt create_LRU_module(int64_t max_LRU_item)
{
    LRU_pt ret = malloc(sizeof(LRU_t));
    ret->LRU_item_num = 0;
    ret->max_LRU_item = max_LRU_item;
    ret->key_to_lrund = init_hashtable(max_LRU_item);


    LRU_node_pt new_nd_pt = malloc(sizeof(LRU_node_t));
    new_nd_pt->pre_pt = NULL;
    new_nd_pt->next_pt = NULL;
    new_nd_pt->value = NULL;

    ret->LRU_list_head = new_nd_pt;
    ret->LRU_list_end = new_nd_pt;

    pthread_spin_init(&(ret->lru_lock), PTHREAD_PROCESS_SHARED);
    return ret;
}

void free_LRU_module(LRU_pt free_lru)
{
    pthread_spin_lock(&(free_lru->lru_lock));
    free_hashtable(free_lru->key_to_lrund);
    LRU_node_pt now_pt = free_lru->LRU_list_head;
    while(now_pt != NULL)
    {
        free_lru->LRU_list_head = now_pt->next_pt;
        if(now_pt->value != NULL)
            free(now_pt->value);
        free(now_pt);
        now_pt = free_lru->LRU_list_head;
    }
    free(free_lru);
    pthread_spin_unlock(&(free_lru->lru_lock));
}

void* get_and_eliminate_LRU_node(LRU_pt now_lru)
{

    pthread_spin_lock(&(now_lru->lru_lock));
    // fprintf(stderr,"%lld\n",now_lru->LRU_item_num);
    LRU_node_pt head_pt = now_lru->LRU_list_head;
    if(now_lru->LRU_item_num == 0)
    {
        pthread_spin_unlock(&(now_lru->lru_lock));
        return NULL;
    }
    now_lru->LRU_list_head = now_lru->LRU_list_head->next_pt;
    now_lru->LRU_list_head->pre_pt = NULL;
    void* ret_pt = NULL;
    if(head_pt->value != NULL)
    {
        ret_pt = malloc(head_pt->value_len);
        memcpy(ret_pt, head_pt->value, head_pt->value_len);
        free(head_pt->value);
        delete_kv_from_hashtable(now_lru->key_to_lrund, head_pt->key);
        free(head_pt);
        now_lru->LRU_item_num--;
        // fprintf(stderr,"pp: %lld\n",now_lru->LRU_item_num);
    }
    else
        goto LRU_list_error;
    pthread_spin_unlock(&(now_lru->lru_lock));
    return ret_pt;
LRU_list_error:
    fprintf(stderr,"The structure of LRU list is error! - %"PRId64"\n",now_lru->LRU_item_num);
    exit(0);
}

void* get_eliminate_LRU_node(LRU_pt now_lru)
{

    pthread_spin_lock(&(now_lru->lru_lock));
    LRU_node_pt head_pt = now_lru->LRU_list_head;
    if(now_lru->LRU_item_num == 0)
        return NULL;
    void* value_copy = malloc(head_pt->value_len);
    if(head_pt->value == NULL)
        goto LRU_list_error;
    memcpy(value_copy, head_pt->value, head_pt->value_len);
    pthread_spin_unlock(&(now_lru->lru_lock));
    return value_copy;
LRU_list_error:
    fprintf(stderr,"The structure of LRU list is error!\n");
    exit(0);
}

void eliminate_LRU_node(LRU_pt now_lru)
{
    // pthread_spin_lock(&(now_lru->lru_lock));
    if(now_lru->LRU_item_num == 0)
        return;


    LRU_node_pt head_pt = now_lru->LRU_list_head;
    now_lru->LRU_list_head = head_pt->next_pt;
    now_lru->LRU_list_head->pre_pt = NULL;
    if(head_pt->value != NULL)
        free(head_pt->value);
    delete_kv_from_hashtable(now_lru->key_to_lrund, head_pt->key);
    free(head_pt);

    now_lru->LRU_item_num--;
    // pthread_spin_unlock(&(now_lru->lru_lock));
}


int add_LRU_node(LRU_pt now_lru, int64_t LRU_key, void* value, int64_t value_len)
{
    pthread_spin_lock(&(now_lru->lru_lock));

    // fprintf(stderr,"value_len: %lld\n",value_len);
    void* ans = search_kv_from_hashtable(now_lru->key_to_lrund, LRU_key);
    // fprintf(stderr,"ans: %lld\n",ans);
    if(ans == NULL)
    {

        if(now_lru->LRU_item_num > now_lru->max_LRU_item)
        {
            eliminate_LRU_node(now_lru);
        }

        LRU_node_pt new_lrund_pt = malloc(sizeof(LRU_node_t));
        new_lrund_pt->pre_pt = now_lru->LRU_list_end;
        new_lrund_pt->next_pt = NULL;
        new_lrund_pt->value = NULL;

        now_lru->LRU_list_end->next_pt = new_lrund_pt;
        now_lru->LRU_list_end->key = LRU_key;
        now_lru->LRU_list_end->value_len = value_len;
        now_lru->LRU_list_end->value = malloc(value_len);
        memcpy(now_lru->LRU_list_end->value, value, value_len);


        int64_t* ans_int64 = malloc(sizeof(int64_t));
        ans_int64[0] = (int64_t)now_lru->LRU_list_end;
        add_kv_into_hashtable(now_lru->key_to_lrund, LRU_key, ans_int64, sizeof(int64_t));


        now_lru->LRU_list_end = new_lrund_pt;


        now_lru->LRU_item_num++;

        pthread_spin_unlock(&(now_lru->lru_lock));
        if(now_lru->LRU_item_num >= now_lru->max_LRU_item)
            return LRU_FULL;
        else
            return LRU_NOT_FULL;
    }
    else
    {

        int64_t* ans_int64 = (int64_t*)ans;
        LRU_node_pt now_node_pt = (LRU_node_pt)ans_int64[0];
        LRU_node_pt pre_node_pt = now_node_pt->pre_pt;
        if(pre_node_pt != NULL)
        {
            if(now_node_pt->next_pt != NULL)
            {
                pre_node_pt->next_pt = now_node_pt->next_pt;
                now_node_pt->next_pt->pre_pt = pre_node_pt;
            }
            else
                pre_node_pt->next_pt = now_node_pt->next_pt;
        }
        else
        {
            now_lru->LRU_list_head = now_lru->LRU_list_head->next_pt;
            now_lru->LRU_list_head->pre_pt = NULL;
        }

        now_node_pt->next_pt = now_lru->LRU_list_end;
        now_node_pt->pre_pt = now_lru->LRU_list_end->pre_pt;

        now_lru->LRU_list_end->pre_pt->next_pt = now_node_pt;
        now_lru->LRU_list_end->pre_pt = now_node_pt;

        pthread_spin_unlock(&(now_lru->lru_lock));
        return LRU_NOT_FULL;
    }
}