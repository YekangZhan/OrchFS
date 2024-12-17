#include "orch_array.h"

orch_arr_pt create_array(int64_t item_size)
{
    if(item_size <= 0)
        goto size_error;
    orch_arr_pt ret_arr = malloc(sizeof(orch_arr_t));
    if(ret_arr == NULL)
        goto alloc_error;
    ret_arr->arr_item_num = 0;
    ret_arr->arr_item_capacity = INIT_CAPACITY;
    ret_arr->arr_item_size = item_size;
    ret_arr->arr_item_sp = malloc(ret_arr->arr_item_size * ret_arr->arr_item_capacity);
    if(ret_arr->arr_item_sp == NULL)
        goto alloc_error;
    return ret_arr;
size_error:
    printf("The item size is error!\n");
    return NULL;
alloc_error:
    printf("alloc error! --- create_array\n");
    exit(1);
}

void expand_arr_space(orch_arr_pt cap, int64_t expand_size)
{
    if(expand_size <= cap->arr_item_capacity)
        return;
    void* new_sp = malloc(cap->arr_item_capacity * expand_size);
    if(new_sp == NULL)
        goto alloc_error;
    memcpy(new_sp, cap->arr_item_sp, cap->arr_item_capacity * cap->arr_item_size);
    free(cap->arr_item_sp);
    cap->arr_item_sp = new_sp;
    cap->arr_item_capacity = expand_size;
    return;
alloc_error:
    printf("alloc error! --- expand_arr_space\n");
    exit(1);
}

void append_into_array(orch_arr_pt cap, void* value)
{
    // expand capacity
    if(cap->arr_item_num + 1 > cap->arr_item_capacity)
        expand_arr_space(cap, cap->arr_item_capacity*2);


    int64_t value_sp_size = malloc_usable_size(value);
    if(value_sp_size < cap->arr_item_size)
        goto value_error;
    

    int64_t app_target_addr = (int64_t)cap->arr_item_sp + cap->arr_item_size*cap->arr_item_num;
    void* app_target_pt = (void*)app_target_addr;
    memcpy(app_target_pt, value, cap->arr_item_size);
    cap->arr_item_num++;
    return;
value_error:
    printf("value size error! --- append_into_array\n");
    exit(1);
}

int64_t search_from_array(orch_arr_pt cap, void* value)
{
    int64_t value_sp_size = malloc_usable_size(value);
    if(value_sp_size < cap->arr_item_size)
        goto value_error;
    
    for(int64_t i = 0; i < cap->arr_item_num; i++)
    {
        int64_t search_target_addr = (int64_t)cap->arr_item_sp + cap->arr_item_size*i;
        void* search_target_pt = (void*)search_target_addr;
        int ret = memcmp(value, search_target_pt, cap->arr_item_size);
        if(ret == 0)
            return i;
    }
    return -1;
value_error:
    printf("value size error! --- search_from_array\n");
    exit(1);
}

void change_arr_value(orch_arr_pt cap, void* value, int64_t pos)
{
    int64_t value_sp_size = malloc_usable_size(value);
    if(value_sp_size < cap->arr_item_size)
        goto value_error;
    

    int64_t change_target_addr = (int64_t)cap->arr_item_sp + cap->arr_item_size*pos;
    void* change_target_pt = (void*)change_target_addr;
    memcpy(change_target_pt, value, cap->arr_item_size);
    return;
value_error:
    printf("value size error! --- change_from_array\n");
    exit(1);
}

void free_array(orch_arr_pt cap)
{
    free(cap->arr_item_sp);
    free(cap);
    cap = NULL;
}