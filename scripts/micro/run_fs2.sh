#!/bin/bash
# if [ "$#" -ne 6 ]; then
#     exit 1
# fi
set -x
blocksize=$1
fs=$2
run_id=$3
pmem_dir=$4
worktype=$5
threads=$6

current_dir=$(pwd)
setup_dir=`readlink -f ../configs`
boost_dir=`readlink -f ../../build`
result_dir=`readlink -f ../../results`
fs_results=$result_dir/$fs

if [  $fs  =  orchfs  ]; then
    run_boost=1
elif [  $fs  == spfs ]; then
    run_boost=3
else
    run_boost=0
fi

ulimit -c unlimited

# echo Sleeping for 5 seconds ..
# sleep 5

run_workload()
{

    echo ----------------------- MICROBENCHMARK $fs $worktype $blocksize $threads ---------------------------

    #####
    date

    if [ $run_boost -eq 1 ]; then
        #cd $setup_dir
        #sudo sh xxfs_config.sh
        cd $current_dir
        echo run $fs
        LD_PRELOAD=$boost_dir/libOrchFS.so $boost_dir/paralleltest2-ComFS $fs $blocksize $threads $worktype $pmem_dir 1 >>$fs_results/all/run$run_id
    else
        $boost_dir/paralleltest2 $fs $blocksize $pmem_dir >>$fs_results/all/run$run_id 
    fi

    date
    cd $current_dir

}

#echo $worktype
run_workload