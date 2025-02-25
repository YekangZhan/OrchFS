#!/bin/bash

current_dir=$(pwd)
setup_dir=`readlink -f ../configs`
source_dir=`readlink -f ../../build`


run_micro_ext4dax()
{
    fs=ext4dax
    sudo rm -rf /mnt/ext4daxtest/*
    sudo umount /mnt/ext4daxtest
    sudo mkfs.ext4 -F -b 4096 /dev/pmem1
    sudo mount -o dax /dev/pmem1 /mnt/ext4daxtest
    for run in 1
    do
        for blksize in 1k 2k 4k 8k 16k 32k 64k 128k 256k 512k
        do
            rm -rf /mnt/ext4daxtest/*
            sudo taskset -c 0-27,56-83 ./run_fs2.sh $blksize $fs $run /mnt/ext4daxtest
            sleep 5
        done
    done
    sudo umount /mnt/ext4daxtest
}

run_micro_novarelaxed()
{
    fs=novarelaxed
    sudo rm -rf /mnt/novatest/*
    sudo umount /mnt/novatest
    sudo rmmod nova
    sudo modprobe nova inplace_data_updates=1
    sudo mount -t NOVA -o init /dev/pmem1 /mnt/novatest
    for run in 1
    do
        for blksize in 1k 4k 8k 16k 32k 64k 128k 256k 512k
        do
            rm -rf /mnt/novatest/*
            sudo taskset -c 0-27,56-83 ./run_fs2.sh $blksize $fs $run /mnt/novatest
            sleep 5
        done
    done
    sudo umount /mnt/novatest
}

run_micro_f2fs()
{
    fs=f2fs
    sudo rm -rf /mnt/f2fstest/*
    sudo umount /mnt/f2fstest
    sudo mkfs.f2fs -f -l f2fs /dev/nvme0n1
    sudo mount -t f2fs /dev/nvme0n1 /mnt/f2fstest
    for run in 1
    do
        for blksize in 1k 4k 8k 16k 32k 64k 128k 256k 512k
        do
            rm -rf /mnt/f2fstest/*
            sudo taskset -c 0-27,56-83 ./run_fs2.sh $blksize $fs $run /mnt/f2fstest
            sleep 5
        done
    done
    sudo umount /mnt/f2fstest
}

run_micro_ext4()
{
    fs=ext4
    sudo rm -rf /mnt/ext4test/*
    sudo umount /mnt/ext4test
    sudo mkfs.ext4 -F /dev/nvme0n1
    sudo mount -t ext4 /dev/nvme0n1 /mnt/ext4test
    for run in 1
    do
        for blksize in 1k 4k 8k 16k 32k 64k 128k 256k 512k
        do
            rm -rf /mnt/ext4test/*
            sudo taskset -c 0-27,56-83 ./run_fs2.sh $blksize $fs $run /mnt/ext4test
            sleep 5
        done
    done
    sudo umount /mnt/ext4test
}


run_micro_p2cache()
{
    fs=p2cache
    for run in 1
    do
        for blksize in 1k 4k 8k 16k 32k 64k 128k 256k 512k
        do
            sudo rm -rf /mnt/p2cachetest/*
            sudo umount /mnt/p2cachetest
            # sudo mkfs.ext4 -F /dev/nvme6n1
            # sudo mount -t ext4 /dev/nvme6n1 /mnt/p2cachetest
            sudo mount -t p2cache -o init /dev/pmem1 /mnt/p2cachetest
            sudo taskset -c 0-27,56-83 ./run_fs2.sh $blksize $fs $run /mnt/p2cachetest
            sudo umount /mnt/p2cachetest
            sudo umount /mnt/p2cachetest
            sleep 5
        done
    done
}

run_micro_spfs_f2fs()
{
    fs=spfs_f2fs
    for run in 1
    do
        for blksize in 1k 4k 8k 16k 32k 64k 128k 256k 512k
        do
            sudo rm -rf /mnt/spfstest/*
            sudo umount /mnt/spfstest
            sudo umount /mnt/spfstest
            sudo mkfs.f2fs -f -l f2fs /dev/nvme0n1
            sudo mount -t f2fs -o sync /dev/nvme0n1 /mnt/spfstest/
            sudo mount -t spfs -o pmem=/dev/pmem1,format,consistency=meta /mnt/spfstest/ /mnt/spfstest/
            sudo taskset -c 0-27,56-83 ./run_fs2.sh $blksize $fs $run /mnt/spfstest
            sudo umount /mnt/spfstest
            sudo umount /mnt/spfstest
            sleep 5
        done
    done
}

run_micro_spfs_ext4()
{
    fs=spfs_ext4
    for run in 1
    do
        for blksize in 1k 4k 8k 16k 32k 64k 128k 256k 512k
        do
            sudo rm -rf /mnt/spfstest/*
            sudo umount /mnt/spfstest
            sudo umount /mnt/spfstest
            sudo mkfs.ext4 -F /dev/nvme0n1
            sudo mount -o sync -t ext4 /dev/nvme0n1 /mnt/spfstest/
            sudo mount -t spfs -o pmem=/dev/pmem1,format,consistency=meta /mnt/spfstest/ /mnt/spfstest/
            sudo taskset -c 0-27,56-83 ./run_fs2.sh $blksize $fs $run /mnt/spfstest
            sudo umount /mnt/spfstest
            sudo umount /mnt/spfstest
            sleep 5
        done
    done
}

run_micro_madfs()
{
    fs=madfs
    for run in 1
    do
        for blksize in 1k 4k 8k 16k 32k 64k 128k 256k 512k
        do
            rm -rf /mnt/madfstest/*
            sudo umount /mnt/madfstest
            sudo mkfs.ext4 -F -b 4096 /dev/pmem1
            sudo mount -o dax /dev/pmem1 /mnt/madfstest
            sudo taskset -c 0-27,56-83 ./run_fs2.sh $blksize $fs $run /mnt/madfstest
            sudo umount /mnt/madfstest
            sleep 5
        done
    done
}

run_micro_xxfs()
{
    fs=xxfs
    for run in 2-time-para
    do
        for blksize in 64k #32k 64k 128k 256k 512k #1k 4k 16k 32k 64k 128k 256k 512k
        do
            for worktype in RANDWRITE #SEQWRITE #RANDREAD #RANDWRITE #SEQREAD SEQWRITE RANDREAD
            do 
                for threads in 8
                do
                    # cd $current_dir
                    # sudo taskset -c 0-27,56-83 ./run_fs.sh $worktype $blksize $threads $fs $run /xx
                    # # sudo taskset -c 18-35,90-107 ./run_fs.sh $worktype $blksize $threads $fs $run /xx
                    ./microtest $blksize $fs $run /Or $worktype $threads
                    sleep 10
                done
            done
        done
    done
}


# echo "-- NOVA Relaxed --"
# cd $current_dir
# run_micro_novarelaxed

# echo "-- EXT4 DAX --"
# cd $current_dir
# run_micro_ext4dax

# echo "-- SPFS + EXT4 --"
# cd $current_dir
# run_micro_spfs_ext4

# echo "-- SPFS + F2FS --"
# cd $current_dir
# run_micro_spfs_f2fs

# echo "-- F2FS --"
# cd $current_dir
# run_micro_f2fs

# echo "-- EXT4 --"
# cd $current_dir
# run_micro_ext4

# echo "-- P2Cache --"
# cd $current_dir
# run_micro_p2cache

# echo "-- ctFS --"
# # cd $setup_dir
# # sudo sh ctfs_config.sh
# cd $current_dir
# run_micro_ctfs

# echo "-- MadFS --"
# cd $current_dir
# run_micro_madfs

echo "-- XXFS --"
gcc microtest.c -o microtest
# cd $current_dir
run_micro_xxfs

# /home/ubuntu/OrchFS-v0.2/build/libOrchFS.so
