#!/bin/bash

current_dir=$(pwd)
setup_dir=`readlink -f ../configs`
source_dir=`readlink -f ../../build`

run_filebench_orchfs()
{
    fs=orchfs
    for run in 8-30
    do
        for worktype in varmail
        do
            sudo ./fbtest $fs $run $worktype /Or
            sleep 15
        done
    done
}

# echo 0 > /proc/sys/kernel/randomize_va_space

echo "-- OrchFS --"
gcc fbtest.c -o fbtest
# cd $setup_dir
cd $current_dir
run_filebench_orchfs


