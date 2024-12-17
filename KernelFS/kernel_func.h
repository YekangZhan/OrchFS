#ifndef KERNEL_FUNC_H
#define KERNEL_FUNC_H

#include <stdint.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>


void mkfs();


void init_kernelFS();


void close_kernelFS();


int call_alloc_func(int64_t func_type, void* para_space);


int call_dealloc_func(int64_t func_type, void* para_space);


int call_log_func(int64_t func_type, void* para_space);


int call_register_func(int64_t func_type, void* para_space);


#endif
