#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#define DEFAULT_SLOTSUM        (4*1024)
#define MAX_SLOTSUM            (4*1024*1024)
#define DEFAULT                0

#define MAGIC_NUM              0x61C88647

struct static_hashtable
{
    int slot_sum;
    struct orch_list** ht_node_pt;
};
typedef struct static_hashtable hashtable_t;
typedef hashtable_t* hashtable_pt;


hashtable_pt init_hashtable(int slot_sum);


void add_kv_into_hashtable(hashtable_pt ht, int64_t key, void* value, int value_len);

void delete_kv_from_hashtable(hashtable_pt ht, int64_t key);


void* search_kv_from_hashtable(hashtable_pt ht, int64_t search_key);


void change_kv_value(hashtable_pt ht, int64_t key, void* value, int value_len);


void free_hashtable(hashtable_pt ht);

#endif