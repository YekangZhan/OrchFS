#ifndef LIBDIR_H
#define LIBDIR_H

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

/* The directory module of libFS is mainly responsible for 
adding, deleting, modifying, and querying directory entries*/

#define CREATE_PATH              1
#define NOT_CREATE_PATH          0

#define ORCH_MAX_NAME            230
#define DIRENT_SIZE              256

#define PATH_EXIST               1
#define PATH_NOT_EXIST           0

#define INVALID_T                0x00
#define DIRENT_FILE_T            0x01
#define SIMPLE_FILE_T            0x02

#define EMPTY_DIR                1
#define NOT_EMPTY_DIR            0

#define inode_id_t               int64_t

struct orch_dirent
{
    inode_id_t d_ino;                   //8B
    __off64_t d_off;                    //8B
    unsigned short int d_namelen;       //2B
    unsigned char d_type;               //1B
    char d_name[ORCH_MAX_NAME + 1];
};
typedef struct orch_dirent orch_dirent_t;
typedef orch_dirent_t* orch_dirent_pt;

extern int64_t all_read_size;

/* Create a path (cannot be recursively created)
 */
int create_dirent(char* pathname);

/* Find the inode number from the Absolute path
 * @pathname                    The path that needs to be searched
 * @create_flag                 If the file does not exist, do you want to create it
 */
inode_id_t path_to_inode(char* pathname, int create_flag);

/* Find the inode number from the relative path
 */
inode_id_t path_to_inode_fromdir(inode_id_t start_ino_id, char* pathname, int create_flag);

/* Delete a path (must be an empty directory)
 */
int delete_dirent(char* pathname, int ftype);

/* Determine if the path exists
 * @pathname                    The path that needs to be checked
 */
int is_path_exist(char* pathname);

/* File renaming
 * @target_dir                  The path where the file name needs to be changed is located
 * @target_file_name            Target file name
 * @changed_file_name           The changed file name
 */
void file_rename(char* target_dir, char* target_fname, char* changed_fname);



void dir_file_init(inode_id_t dir_ino, inode_id_t father_dir_ino);

#endif