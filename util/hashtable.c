#include "hashtable.h"
#include "orch_list.h"

int64_t get_solt_id(hashtable_pt ht, int64_t key)
{
    int64_t pre_num = (key>>32), suf_num = ((key<<32)>>32);
    int64_t mul = (1LL<<32), slots = ht->slot_sum;
    int64_t ret_slot = ((pre_num % slots * mul) % slots + suf_num % slots) % slots;
    if(ret_slot < 0 || ret_slot >= slots)
        return -1;
    return ret_slot;
}

hashtable_pt init_hashtable(int slot_sum)
{
    if(slot_sum < DEFAULT && slot_sum > MAX_SLOTSUM)
    {
        printf("slot sum error!\n");
        return NULL;
    }
    else
    {
        hashtable_pt ret_htp = malloc(sizeof(hashtable_t));
        if(slot_sum == DEFAULT)
            ret_htp->slot_sum = DEFAULT_SLOTSUM;
        else
            ret_htp->slot_sum = slot_sum;
        ret_htp->ht_node_pt = malloc(sizeof(struct orch_list) * ret_htp->slot_sum);
        for(int i = 0; i < ret_htp->slot_sum; i++)
        {
            ret_htp->ht_node_pt[i] = init_list();
            if(ret_htp->ht_node_pt[i] == NULL)
                return NULL;
        }
        return ret_htp;
    }
}

void add_kv_into_hashtable(hashtable_pt ht, int64_t key, void* value, int value_len)
{
    if(ht == NULL)
        goto error2;
    int slot_id = get_solt_id(ht, key);
    if(slot_id == -1)
        goto error1;
    insert_into_list(ht->ht_node_pt[slot_id], key, value, value_len);
    return;
error1:
    printf("The key is error -- add!\n");
    exit(0);
error2:
    printf("The hashtable is error! -- add\n");
    exit(0);
}

void delete_kv_from_hashtable(hashtable_pt ht, int64_t delete_key)
{   
    if(ht == NULL)
        goto error2;
    int slot_id = get_solt_id(ht, delete_key);
    if(slot_id == -1)
        goto error1;
    int ret_info = delete_list_node(ht->ht_node_pt[slot_id], delete_key);
    if(ret_info == -1)
        goto warning;
    else
        return;
error1:
    printf("The key is error -- delete!\n");
    exit(0);
error2:
    printf("The hashtable is error! -- delete\n");
    exit(0);
warning:
    printf("The key you want to delete does not exist!\n");
}

void* search_kv_from_hashtable(hashtable_pt ht, int64_t search_key)
{
    if(ht == NULL)
        goto error2;
    int slot_id = get_solt_id(ht, search_key);
    if(slot_id == -1)
        goto error1;
    void* ret_pt = search_list_data(ht->ht_node_pt[slot_id], search_key);
    return ret_pt;
error1:
    printf("The key is error -- search!\n");
    exit(0);
error2:
    printf("The hashtable is error!\n");
    exit(0);
}

void change_kv_value(hashtable_pt ht, int64_t key, void* value, int value_len)
{
    if(ht == NULL)
        goto error2;
    int slot_id = get_solt_id(ht, key);
    if(slot_id == -1)
        goto error1;
    int ret_info = change_list_node(ht->ht_node_pt[slot_id], key, value, value_len);
    if(ret_info == -1)
        goto warning;
    return;
error1:
    printf("The key is error -- change!\n");
    exit(0);
error2:
    printf("The hashtable is error! -- change\n");
    exit(0);
warning:
    printf("The key you want to delete does not exist!\n");
}

void free_hashtable(hashtable_pt ht)
{
    if(ht == NULL)
        return;
    for(int i = 0; i < ht->slot_sum; i++)
    {
        free_list(ht->ht_node_pt[i]);
    }
    return;
}