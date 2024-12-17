#ifndef DIRCACHE_H
#define DIRCACHE_H

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/statfs.h>
#include <sys/vfs.h>
#include <linux/magic.h>
#include <linux/falloc.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <inttypes.h>
#include <time.h>
#include <x86intrin.h>
#include <stdarg.h> 

#define MAX_CACHE_DIR                     (1024*32)

struct dir_cache
{
    struct radix_node* cache_rdtree_pt;
    int now_dir_sum;
};
typedef struct dir_cache dir_cache_t;
typedef dir_cache_t* dir_cache_pt;

/*
All file paths are divided into multiple segments and stored in an array
for example: /home/lkz/femushare/file
After splitted, the path_arr includes the following string.
patharr[0] = 'home'
patharr[1] = 'lkz'
patharr[2] = 'femushare'
patharr[3] = 'file'
For absolute paths, start_dir_inoid represents the inode ID 
where the root directory file is located.
*/

dir_cache_pt init_dir_cache();

/* Insert the direct into cache                   
 */
void insert_dir_cache(dir_cache_pt dir_cache, char* path_arr[], 
                    int path_part_num, int64_t start_dir_inoid, int64_t file_inoid);

/* Delete the direct cache
 */
void delete_dir_cache(dir_cache_pt dir_cache, char* path_arr[], 
                    int path_part_num, int64_t start_dir_inoid);

/* Search the dirent cache
 */
int64_t search_dir_cache(dir_cache_pt dir_cache, char* path_arr[],
                    int path_part_num, int64_t start_dir_inoid);


void free_dir_cache(dir_cache_pt dir_cache);

#endif