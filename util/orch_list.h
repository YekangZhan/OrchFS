#ifndef ORCHLIST_H
#define ORCHLIST_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

struct list_node
{
    int64_t key;                                      
    void* node_data;                          
    struct list_node* next_pt;                   
};
typedef struct list_node list_node_t;
typedef list_node_t* list_node_pt;

struct orch_list
{
    int64_t list_len;                                
    list_node_pt list_head;                          
    list_node_pt list_end;                          
};
typedef struct orch_list orch_list_t;
typedef orch_list_t* orch_list_pt;



orch_list_pt init_list();


void* search_list_data(orch_list_pt target_list, int64_t search_key);


int insert_into_list(orch_list_pt target_list, int64_t insert_key, void* insert_data, int64_t data_len);


int insert_into_list_head(orch_list_pt target_list, int64_t insert_key, void* insert_data, int64_t data_len);


int change_list_node(orch_list_pt target_list, int64_t change_key, void* change_data, int64_t data_len);


int delete_list_node(orch_list_pt target_list, int64_t delete_key);


void free_list(orch_list_pt target_list);

#endif