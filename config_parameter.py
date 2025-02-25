import sys

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
        

def check_argv(para_list):
    nvm_dev_name = para_list[1]
    ssd_dev_name = para_list[2]
    print(nvm_dev_name, ssd_dev_name)

if __name__ == '__main__':    
    # 配置设备
    change_parameter(sys.argv, 1, False)
    check_argv(sys.argv)