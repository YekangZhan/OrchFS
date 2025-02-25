# OrchFS [FAST'25] (Version-0.1)


### Structure:

```
root
|---- KernelFS      (OrchFS KernelFS module)
|---- LibFS     (OrchFS LibFS module)
|---- config        (OrchFS config module)
|---- util      (OrchFS other optimization module)
```
Before running OrchFS, it is necessary to ensure that there is a persistent memory (usually Optane NVM) and a block device available. The persistent memory require at least 32GiB of space and need to be configured in dax mode.
Python 3.x is required on the server to run scripts that modify configuration files.

### Building OrchFS:
Assume current directory is a project root directory.

1. Modify configuration file
```
python config_parameter.py [dax] [SSD] [PM_thds] [SSD_thds] [split_size] [fb_flag]
```
- `[dax]`: The device file path of persistent memory. For example, `/dev/dax0.0`.
- `[SSD]`: The device file path of SSD. Recommend using high-speed NVMe SSD. For example, `/dev/nvme1n1`.
- `[PM_thds]`: The maximum number of threads executing PM IOs in parallel IO engine. If you use Optane NVM, it is recommended to set it to 5.
- `[SSD_thd]`: The maximum number of threads executing SSD IOs in parallel IO engine. It is recommended to set this parameter to be greater than 8 and not exceed the number of physical CPU cores on the server.
- `[split_size]`: The splitting granularity of the parallel IO engine. This parameter needs to be a multiple of 32KB. For example, '64k', '256k', '32k'.
- `[fb_flag]`: This parameter can be omitted. If you will test the filebench, this parameter needs to be set to 'fb_mode_on'. Otherwise, this parameter does not need to be set.

Here is an example for simple workload:
```
python config_parameter.py /dev/dax0.0 /dev/nvme1n1 5 16 32k
```

Here is an example for testing filebench:
```
python config_parameter.py /dev/dax0.0 /dev/nvme1n1 5 16 32k fb_mode_on
```
Replace `[dax]` and `[SSD]` with the device files on the server you are using now. Every time the above two types of workloads switch, the script needs to be run.

2. Compile

Compiling requires the use of the cmake tool.
```
mkdir build
cmake ..
make
```

3. Example of Micro-benchmark
```
cd scripts
cd micro
sudo sh run_micro.sh
```
This test is used to test the performance of Sequential write and random write.
During runtime, it is necessary to ensure that all test files are in the directory `/Or`, otherwise these files will not be processed by OrchFS.

4. Example of filebench
```
sh compile_filebench.sh
cd scripts
cd filebench
sudo sh run_filebench.sh
```
This test is used to test the performance of `varmail` workload.

Before running Filebench, we must modify the path in the workload file. Specifically, Find 'set $dir=[path]' and modify it to 'set $dir=/Or'. 
Examples can be found in `./filebench/workloads/varmail.f` 



We will continue to maintain this repository.

TODO: 

- We are preparing more user-friendly test code samples and more automated testing scripts.
- We are enhancing the readability and modularity of the code to make it more developer-friendly.
- We will provide a method on how to write our own test program to test OrchFS.
- Add comments and fix potential bugs.
