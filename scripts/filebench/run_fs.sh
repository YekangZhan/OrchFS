#!/bin/bash

if [ "$#" -ne 4 ]; then
    echo "Usage: sudo ./run_fs.sh <fs> <run_id>"
    exit 1
fi

set -x

fs=$1
run_id=$2
worktype=$3
pmem_dir=$4

current_dir=$(pwd)
filebench_dir=`readlink -f ../../filebench`
setup_dir=`readlink -f ../configs`
workload_dir=$filebench_dir/workloads

boost_dir=`readlink -f ../../build`
result_dir=`readlink -f ../../results`
fs_results=$result_dir/$fs

if [ "$fs" == "orchfs" ]; then
    run_boost=1
elif [ "$fs" == "ctfs" ]; then
    run_boost=2
else
    run_boost=0
fi

ulimit -c unlimited

# echo Sleeping for 5 seconds ..
# sleep 5

# /home/ubuntu/OrchFS-v0.2/build/libOrchFS.so
run_varmail()
{

    echo ----------------------- FILEBENCH varmail ---------------------------
    mkdir -p $fs_results/varmail
    rm $fs_results/varmail/run$run_id
    date
    if [ $run_boost -eq 1 ]; then
        # cd $setup_dir
        # sudo sh xxfs_config.sh
        cd $current_dir
        LD_PRELOAD=$boost_dir/libOrchFS.so $filebench_dir/filebench -f $workload_dir/varmail.f 2>&1 | tee $fs_results/varmail/run$run_id
    else
        $filebench_dir/filebench -f $workload_dir/varmail.f 2>&1 | tee $fs_results/varmail/run$run_id >./info.txt
    fi
    date
    cd $current_dir
    echo Sleeping for 5 seconds . .
    sleep 5
}


run_fileserver()
{

    echo ----------------------- FILEBENCH fileserver ---------------------------
    mkdir -p $fs_results/fileserver
    rm $fs_results/fileserver/run$run_id
    date
    if [ $run_boost -eq 1 ]; then
        # cd $setup_dir
        # sudo sh xxfs_config.sh
        cd $current_dir
        LD_PRELOAD=$boost_dir/libOrchFS.so $filebench_dir/filebench -f $workload_dir/fileserver.f 2>&1 | tee $fs_results/fileserver/run$run_id
    else
        $filebench_dir/filebench -f $workload_dir/fileserver.f 2>&1 | tee $fs_results/fileserver/run$run_id >./info.txt
    fi
    date
    cd $current_dir
    echo Sleeping for 5 seconds . .
    sleep 5
}

run_webserver()
{
    echo ----------------------- FILEBENCH webserver ---------------------------
    mkdir -p $fs_results/webserver
    rm $fs_results/webserver/run$run_id
    date
    if [ $run_boost -eq 1 ]; then
        # cd $setup_dir
        # sudo sh xxfs_config.sh
        cd $current_dir
        LD_PRELOAD=$boost_dir/libOrchFS.so $filebench_dir/filebench -f $workload_dir/webserver.f 2>&1 | tee $fs_results/webserver/run$run_id
    else
        $filebench_dir/filebench -f $workload_dir/webserver.f 2>&1 | tee $fs_results/webserver/run$run_id
    fi
    date
    cd $current_dir
    echo Sleeping for 5 seconds . .
    sleep 5
}

run_webproxy()
{
    echo ----------------------- FILEBENCH webproxy ---------------------------
    mkdir -p $fs_results/webproxy
    rm $fs_results/webproxy/run$run_id
    date
    if [ $run_boost -eq 1 ]; then
        # cd $setup_dir
        # sudo sh xxfs_config.sh
        cd $current_dir
        LD_PRELOAD=$boost_dir/libOrchFS.so $filebench_dir/filebench -f $workload_dir/webproxy.f 2>&1 | tee $fs_results/webproxy/run$run_id
    else
        $filebench_dir/filebench -f $workload_dir/webproxy.f 2>&1 | tee $fs_results/webproxy/run$run_id
    fi
    date
    cd $current_dir
    echo Sleeping for 5 seconds . .
    sleep 5
}

case "$worktype" in
    varmail)
        run_varmail
        ;;
    fileserver)
        run_fileserver
        ;;
    webserver)
        run_webserver
        ;;
    webproxy)
        run_webproxy
        ;;
    *)
        exit 1
esac
#run_workload

cd $current_dir
