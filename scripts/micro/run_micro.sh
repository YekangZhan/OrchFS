#!/bin/bash

current_dir=$(pwd)
setup_dir=`readlink -f ../configs`
source_dir=`readlink -f ../../build`


run_micro_orchfs()
{
    fs=orchfs
    for run in 1
    do
        for blksize in 64k #32k 64k 128k 256k 512k
        do
            for worktype in RANDWRITE SEQWRITE
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


echo "-- OrchFS --"
gcc microtest.c -o microtest
# cd $current_dir
run_micro_orchfs

# /home/ubuntu/OrchFS-v0.2/build/libOrchFS.so
