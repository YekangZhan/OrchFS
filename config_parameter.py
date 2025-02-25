import sys
import os

def change_parameter(para_list, split_blk, fb_flag):
    # 获取参数
    nvm_dev_name = para_list[1]
    ssd_dev_name = para_list[2]
    max_nvm_threads = para_list[3]
    max_ssd_threads = para_list[4]

    # 读取文件
    with open("./config/config_template.h", "r") as file:
        config_lines = file.readlines()
    # print(config_lines)

    # 表达式匹配后替换
    line_num = len(config_lines)
    new_config = []
    for i in range(0, line_num):
        line = config_lines[i]
        if "ORCH_DEV_NVM_PATH" in line:
            newline = "#define ORCH_DEV_NVM_PATH     \""+nvm_dev_name+"\"\n"
            config_lines[i] = newline
        elif "ORCH_DEV_SSD_PATH" in line:
            newline = "#define ORCH_DEV_SSD_PATH     \""+ssd_dev_name+"\"\n"
            config_lines[i] = newline
        elif "ORCH_CONFIG_NVMTHD" in line:
            newline = "#define ORCH_CONFIG_NVMTHD     "+max_nvm_threads+"\n"
            config_lines[i] = newline
        elif "ORCH_CONFIG_SSDTHD" in line:
            newline = "#define ORCH_CONFIG_SSDTHD     "+max_ssd_threads+"\n"
            config_lines[i] = newline
        elif "ORCH_MAX_SPLIT_BLK" in line:
            newline = "#define ORCH_MAX_SPLIT_BLK     "+str(split_blk)+"\n"
            config_lines[i] = newline
    
    if fb_flag == True:
        for i in range(0, line_num):
            line = config_lines[i]
            if "// #define FILEBENCH" in line:
                newline = "#define FILEBENCH\n"
                config_lines[i] = newline
    
    # 写回
    with open("./config/config.h", 'w') as file:
        file.writelines(config_lines)
        

def print_argv(para_list):
    print("Byte addressing device file path:",para_list[1])
    print("Block addressing device file path:",para_list[2])
    print("Maximum number of write threads for Byte addressing device:",para_list[3])
    print("Maximum number of write threads for Block addressing device:",para_list[4])

def get_split_blks(size_str):
    split_blk = 1
    size_unit = size_str[-1]
    size_num = size_str[:-1]
    if size_unit != 'k' and size_unit != 'K':
        print("error: Format error in the split granularity grid of parallel IO engine")
        print("Enable default split granularity of 32KB")
        return split_blk
    if size_num.isdigit() == False:
        print("error: Format error in the split granularity grid of parallel IO engine")
        print("Enable default split granularity of 32KB")
        return split_blk
    if int(size_num) <= 0 or int(size_num) % 32 != 0:
        print("error: The splitting granularity must be a multiple of 32KB")
        print("Enable default split granularity of 32KB")
        return split_blk
    
    split_blk = int(size_num) // 32
    return split_blk

def check_filebench(fb_flag):
    if fb_flag == 'fb_mode_on':
        return True
    else:
        return False

def check_dev_file_exist(nvm_dev_file, ssd_dev_file):
    if os.path.exists(nvm_dev_file) and os.path.exists(ssd_dev_file):
        return True
    else:
        print(nvm_dev_file, "or", ssd_dev_file, "do not exist")
        return False

def check_threads(max_nvm_threads, max_ssd_threads):
    if max_nvm_threads.isdigit() == False or max_ssd_threads.isdigit() == False:
        print("The number of threads is error")
        return False
    if int(max_nvm_threads) <= 0 or int(max_ssd_threads) <= 0:
        print("The number of threads cannot be negative")
        return False
    return True

if __name__ == '__main__': 
    # 输出基本参数
    print_argv(sys.argv)

    # 检查设备文件
    ret = check_dev_file_exist(sys.argv[1], sys.argv[2])
    if ret == False:
        sys.exit()
    
    # 检查线程数
    ret = check_threads(sys.argv[3], sys.argv[4])
    if ret == False:
        sys.exit()

    # 检查拆分粒度
    split_blk = get_split_blks(sys.argv[5])
    print("split granularity:", split_blk*32, "KB")

    # 检查是否启用filebench标志
    num_arg = len(sys.argv) - 1
    fb_flag = False
    if num_arg > 6:
        fb_flag = check_filebench(sys.argv[6])
        if fb_flag == True:
            print("Filebench mode on\n")
    else:
        print("Filebench mode off\n")

    # 改变参数
    change_parameter(sys.argv, split_blk, fb_flag)