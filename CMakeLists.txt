# CMake 最低版本号要求
cmake_minimum_required (VERSION 3.0)

set(ProjectName OrchFS)

# 项目信息
project(${ProjectName})


add_definitions(-O3 -fPIC -mclwb -mclflushopt -Wall -pthread -mavx512f)
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED True)



set(${ProjectName}_LIBFSsrc ./LibFS/index.c ./LibFS/io_thdpool.c ./LibFS/lib_dir.c ./LibFS/lib_func.c
./LibFS/lib_inode.c ./LibFS/lib_log.c ./LibFS/lib_rwfunc.c ./LibFS/lib_socket.c ./LibFS/libspace.c ./LibFS/meta_cache.c 
./LibFS/req_kernel.c ./LibFS/runtime.c ./LibFS/ffile.c ./LibFS/dir_cache.c ./LibFS/lib_shm.c ./LibFS/migrate.c ./KernelFS/device.c 
./util/hashtable.c ./util/radixtree.c ./util/orch_list.c ./util/orch_array.c ./util/LRU.c)

ADD_LIBRARY(${ProjectName}_LIBFS ${${ProjectName}_LIBFSsrc})
ADD_LIBRARY(${ProjectName} SHARED ./LibFS/orchfs_wrapper.c)                        #./src/glibc/ffile.c

TARGET_LINK_LIBRARIES(${ProjectName} ${ProjectName}_LIBFS pmem pthread dl)
target_include_directories(${ProjectName} PUBLIC LibFS)


#-------------------------------------------------------------------------------------------------------------------------------------

set(${ProjectName}_KERFSsrc ./KernelFS/balloc.c ./KernelFS/device.c ./KernelFS/kernel_func.c ./KernelFS/kernel_recv.c 
./KernelFS/kernel_send.c ./KernelFS/log.c ./KernelFS/thdpool.c ./KernelFS/kindex.c ./KernelFS/close_kfs.c ./KernelFS/execio_thdpool.c
./KernelFS/ker_shm.c ./util/hashtable.c ./util/orch_list.c ./util/orch_array.c) 

ADD_LIBRARY(${ProjectName}_KERFS ${${ProjectName}_KERFSsrc})
TARGET_LINK_LIBRARIES(${ProjectName}_KERFS pmem pthread) #库
target_include_directories(${ProjectName}_KERFS PUBLIC KernelFS)


add_executable(mkfs KernelFS/mkfs.c)
TARGET_LINK_LIBRARIES(mkfs ${ProjectName}_KERFS)
add_custom_target(run_mkfs
DEPENDS mkfs
COMMAND mkfs
)

add_executable(kfs_main KernelFS/kernel_main.c)
TARGET_LINK_LIBRARIES(kfs_main ${ProjectName}_KERFS)
add_custom_target(run_kfs_main
DEPENDS kfs_main
COMMAND kfs_main
)

add_executable(close_kfs KernelFS/close_kfs.c)
TARGET_LINK_LIBRARIES(close_kfs ${ProjectName}_KERFS)
add_custom_target(run_close_kfs
DEPENDS close_kfs
COMMAND close_kfs
)
