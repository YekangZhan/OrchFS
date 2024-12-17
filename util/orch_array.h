#ifndef orch_ARR_H
#define orch_ARR_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#define INIT_CAPACITY              8

struct orch_array_t
{
    int64_t arr_item_num;                          
    int64_t arr_item_capacity;                  
    int64_t arr_item_size;                      
    void* arr_item_sp;                          
};
typedef struct orch_array_t orch_arr_t;
typedef orch_arr_t* orch_arr_pt;


orch_arr_pt create_array(int64_t item_size);


void expand_array_space(orch_arr_pt cap, int64_t expand_size);


void append_into_array(orch_arr_pt cap, void* value);


int64_t search_from_array(orch_arr_pt cap, void* value);


void change_arr_value(orch_arr_pt cap, void* value, int64_t pos);


void free_array(orch_arr_pt cap);

#endif