#!/bin/bash

# for((k=0;k<10;k++))
# do
#     echo "/*----------------------- GRWS Scheduler Begin! Fib -----------------------*/"
#     echo " "
#     export XITAO_LAYOUT_PATH=./ERASE_TACO/ptt_layout_tx2; ./ERASE_TACO/benchmarks/fibonacci/fibonacci 3 1 55 34 > ./ERASE_TACO/debug_results/fib_grws_$k.txt
#     sleep 5
#     echo "/*----------------------The $k th try finish!-----------------------*/"
# done

for((k=0;k<10;k++))
do
    echo "/*----------------------- ERASE Scheduler Begin! Fib -----------------------*/"
    echo " "
    export XITAO_LAYOUT_PATH=./ERASE_TACO/ptt_layout_a1; ./ERASE_TACO/benchmarks/fibonacci/fibonacci 3 1 55 34 > ./ERASE_TACO/debug_results/fib_erase_$k.txt
    sleep 5
    echo "/*----------------------The $k th try finish!-----------------------*/"
done

# for((k=0;k<10;k++))
# do
#     echo "/*----------------------- AEQUITAS Scheduler Begin! Fib -----------------------*/"
#     echo " "
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     export XITAO_LAYOUT_PATH=./AEQUITAS/ptt_layout_test; ./AEQUITAS/benchmarks/fibonacci/fibonacci 3 1 55 34 > ./AEQUITAS/debug_results/fib_aq_$k.txt
#     sleep 5
#     echo "/*----------------------The $k th try finish!-----------------------*/"
# done

# for((k=0;k<10;k++))
# do
#     echo "/*----------------------- STEER Scheduler Begin! Fib -----------------------*/"
#     echo " "
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     export XITAO_LAYOUT_PATH=./EAS/ptt_layout_tx2; ./EAS/benchmarks/fibonacci/fibonacci 1 55 34 > ./EAS/debug_results/fib_steer_$k.txt
#     sleep 5
#     echo "/*----------------------The $k th try finish!-----------------------*/"
# done


# --- Dot Product ---
# for((k=0;k<10;k++))
# do
#     echo "/*----------------------- AEQUITAS Scheduler Begin! Dot Product -----------------------*/"
#     echo " "
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     export XITAO_LAYOUT_PATH=./AEQUITAS/ptt_layout_test; ./AEQUITAS/benchmarks/dotproduct/dotprod 3 1000 40000000 1 200000 > ./AEQUITAS/debug_results/dotprod_20k_AEQUITAS_$k.txt
#     sleep 5
#     echo "/*----------------------The $k th try finish!-----------------------*/"
# done

# for((k=0;k<10;k++))
# do
#     echo "/*----------------------- AEQUITAS Scheduler Begin! Dot Product -----------------------*/"
#     echo " "
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     export XITAO_LAYOUT_PATH=./AEQUITAS/ptt_layout_test; ./AEQUITAS/benchmarks/dotproduct/dotprod 3 1000 40000000 1 1000000 > ./AEQUITAS/debug_results/dotprod_100k_AEQUITAS_$k.txt
#     sleep 5
#     echo "/*----------------------The $k th try finish!-----------------------*/"
# done

# for((k=0;k<10;k++))
# do
#     echo "/*----------------------- STEER Scheduler Begin! Dot Product -----------------------*/"
#     echo " "
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     export XITAO_LAYOUT_PATH=./EAS/ptt_layout_tx2; ./EAS/benchmarks/dotproduct/dotprod 1 1000 40000000 1 200000 > ./EAS/debug_results/dotprod_20k_STEER_$k.txt
#     sleep 5
#     echo "/*----------------------The $k th try finish!-----------------------*/"
# done

# for((k=0;k<10;k++))
# do
#     echo "/*----------------------- STEER Scheduler Begin! Dot Product -----------------------*/"
#     echo " "
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     export XITAO_LAYOUT_PATH=./EAS/ptt_layout_tx2; ./EAS/benchmarks/dotproduct/dotprod 1 1000 40000000 1 1000000 > ./EAS/debug_results/dotprod_100k_STEER_$k.txt
#     sleep 5
#     echo "/*----------------------The $k th try finish!-----------------------*/"
# done

# for((k=0;k<10;k++))
# do
#     echo "/*----------------------- JOSS Scheduler Begin! Dot Product -----------------------*/"
#     echo " "
#     echo 1 >/sys/kernel/debug/bpmp/debug/clk/emc/mrq_rate_locked
#     echo 1866000000 > /sys/kernel/debug/bpmp/debug/clk/emc/rate
#     echo 1 > /sys/kernel/debug/bpmp/debug/clk/emc/state
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     sleep 5
#     export XITAO_LAYOUT_PATH=./Paper3/ptt_layout_tx2; ./Paper3/benchmarks/dotproduct/dotprod 1 1000 40000000 1 200000 > ./Paper3/debug_results/dotprod_20k_JOSS_$k.txt
#     sleep 5
#     echo "/*----------------------The $k th try finish!-----------------------*/"
# done

# for((k=0;k<10;k++))
# do
#     echo "/*----------------------- JOSS Scheduler Begin! Dot Product -----------------------*/"
#     echo " "
#     echo 1 >/sys/kernel/debug/bpmp/debug/clk/emc/mrq_rate_locked
#     echo 1866000000 > /sys/kernel/debug/bpmp/debug/clk/emc/rate
#     echo 1 > /sys/kernel/debug/bpmp/debug/clk/emc/state
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     export XITAO_LAYOUT_PATH=./Paper3/ptt_layout_tx2; ./Paper3/benchmarks/dotproduct/dotprod 1 1000 40000000 1 1000000 > ./Paper3/debug_results/dotprod_100k_JOSS_$k.txt
#     sleep 5
#     echo "/*----------------------The $k th try finish!-----------------------*/"
# done


###### HEAT ######
# for((k=0;k<10;k++))
# do
#     echo "/*----------------------- AEQUITAS Scheduler Begin! 2D Heat- small  -----------------------*/"
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     ./benchmarks/heat/heat-tao ./benchmarks/heat/small.dat > ./debug_results/heat_small_aq_${k}.txt 
#     sleep 5
#     echo "/*----------------------The $k th try finish!-----------------------*/"
# done

# for((k=0;k<10;k++))
# do
#     echo "/*----------------------- AEQUITAS Scheduler Begin! 2D Heat- big  -----------------------*/"
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     ./benchmarks/heat/heat-tao ./benchmarks/heat/big.dat > ./debug_results/heat_big_aq_${k}.txt 
#     sleep 5
#     echo "/*----------------------The $k th try finish!-----------------------*/"
# done

# for((k=0;k<10;k++))
# do
#     echo "/*----------------------- AEQUITAS Scheduler Begin! 2D Heat- huge  -----------------------*/"
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     ./benchmarks/heat/heat-tao ./benchmarks/heat/huge.dat > ./debug_results/heat_huge_aq_${k}.txt 
#     sleep 5
#     echo "/*----------------------The $k th try finish!-----------------------*/"
# done

###### Sparse LU ######
# for((j=0;j<10;j++))
# do
#   echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#   echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#   echo "/*---------------------------------------------------------------*/"
#   echo "AEQUITAS Scheduler Begin the $j execution! "
#   echo "Sparse LU - 32 blocks, 256 block length:"
#   ./benchmarks/sparselu/sparselu 3 32 256 > AEQUITAS_32_256_$j.txt
#   sleep 5
#   echo "AEQUITAS End! "
#   echo "/*---------------------------------------------------------------*/"
#   echo ""
#   # chown nvidia AEQUITAS_64_256_$j.txt
# done

# for((j=0;j<10;j++))
# do
#   echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#   echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#   echo "/*---------------------------------------------------------------*/"
#   echo "AEQUITAS Scheduler Begin the $j execution! "
#   echo "Sparse LU - 32 blocks, 512 block length:"
#   ./benchmarks/sparselu/sparselu 3 32 512 > AEQUITAS_32_512_$j.txt
#   sleep 5
#   echo "AEQUITAS End! "
#   echo "/*---------------------------------------------------------------*/"
#   echo ""
#   # chown nvidia AEQUITAS_64_256_$j.txt
# done

# for((j=0;j<10;j++))
# do
#   echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#   echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#   echo "/*---------------------------------------------------------------*/"
#   echo "AEQUITAS Scheduler Begin the $j execution! "
#   echo "Sparse LU - 32 blocks, 512 block length:"
#   ./benchmarks/sparselu/sparselu 3 64 256 > AEQUITAS_64_256_$j.txt
#   sleep 5
#   echo "AEQUITAS End! "
#   echo "/*---------------------------------------------------------------*/"
#   echo ""
#   # chown nvidia AEQUITAS_64_256_$j.txt
# done

# for((j=0;j<10;j++))
# do
#   echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#   echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#   echo "/*---------------------------------------------------------------*/"
#   echo "AEQUITAS Scheduler Begin the $j execution! "
#   echo "Sparse LU - 64 blocks, 512 block length:"
#   ./benchmarks/sparselu/sparselu 3 64 512 > AEQUITAS_64_512_$j.txt
#   sleep 5
#   echo "AEQUITAS End! "
#   echo "/*---------------------------------------------------------------*/"
#   echo ""
#   # chown nvidia AEQUITAS_64_256_$j.txt
# done

###### Synthetic Benchmarks ######
# parallelism="4 8 16"

# for dop in $parallelism
# do
#   for((k=0;k<5;k++))
#   do
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     echo "/*----------------------- MM - 256 - 10K  - p${dop} - $k iteration starts! -----------------------*/" >> Process.txt
#     ./benchmarks/syntheticDAGs/synbench 3 256 0 0 1 10000 0 0 $dop >> ./debug_results/MM_256_AQ_p${dop}.txt   
#     sleep 5
#   done
# done

# for dop in $parallelism
# do
#   for((k=0;k<5;k++))
#   do
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     echo "/*----------------------- MM - 512 - 2K  - p${dop} - $k iteration starts! -----------------------*/" >> Process.txt
#     ./benchmarks/syntheticDAGs/synbench 3 512 0 0 1 2000 0 0 $dop >> ./debug_results/MM_512_AQ_p${dop}.txt   
#   done
# done

# for dop in $parallelism
# do
#   for((k=0;k<10;k++))
#   do
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     echo "/*----------------------- CP - 1024 - 50K  - p${dop} - $k iteration starts! -----------------------*/" >> Process.txt
#     ./benchmarks/syntheticDAGs/synbench 3 0 1024 0 1 0 50000 0 $dop >> ./debug_results/CP_1024_AQ_p${dop}.txt   
#   done
# done

# for dop in $parallelism
# do
#   for((k=0;k<10;k++))
#   do
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     echo "/*----------------------- CP - 2048 - 50K  - p${dop} - $k iteration starts! -----------------------*/" >> Process.txt
#     ./benchmarks/syntheticDAGs/synbench 3 0 2048 0 1 0 50000 0 $dop >> ./debug_results/CP_2048_AQ_p${dop}.txt   
#   done
# done

# for dop in $parallelism
# do
#   for((k=0;k<10;k++))
#   do
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     echo "/*----------------------- CP - 4096 - 20K  - p${dop} - $k iteration starts! -----------------------*/" >> Process.txt
#     ./benchmarks/syntheticDAGs/synbench 3 0 4096 0 1 0 20000 0 $dop >> ./debug_results/CP_4096_AQ_p${dop}.txt   
#   done
# done

# for dop in $parallelism
# do
#   for((k=0;k<10;k++))
#   do
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     echo "/*----------------------- CP - 8192 - 10K  - p${dop} - $k iteration starts! -----------------------*/" >> Process.txt
#     ./benchmarks/syntheticDAGs/synbench 3 0 8192 0 1 0 10000 0 $dop >> ./debug_results/CP_8192_AQ_p${dop}.txt   
#   done
# done

# for dop in $parallelism
# do
#   for((k=0;k<10;k++))
#   do
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     echo "/*----------------------- ST - 512 - 50K  - p${dop} - $k iteration starts! -----------------------*/" >> Process.txt
#     ./benchmarks/syntheticDAGs/synbench 3 0 0 512 1 0 0 50000 $dop >> ./debug_results/ST_512_AQ_p${dop}.txt   
#   done
# done

# for dop in $parallelism
# do
#   for((k=0;k<10;k++))
#   do
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     echo "/*----------------------- ST - 1024 - 50K  - p${dop} - $k iteration starts! -----------------------*/" >> Process.txt
#     ./benchmarks/syntheticDAGs/synbench 3 0 0 1024 1 0 0 50000 $dop >> ./debug_results/ST_1024_AQ_p${dop}.txt   
#   done
# done

# for dop in $parallelism
# do
#   for((k=0;k<10;k++))
#   do
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#     sleep 5
#     echo "/*----------------------- ST - 2048 - 50K  - p${dop} - $k iteration starts! -----------------------*/" >> Process.txt
#     ./benchmarks/syntheticDAGs/synbench 3 0 0 2048 1 0 0 50000 $dop >> ./debug_results/ST_2048_AQ_p${dop}.txt   
#   done
# done


# ff=1
# gg=4
# for((j=$ff;j>0;j--))
# do
#   for((i=$gg;i<=4;i+=2))
#   do
#     echo "MatMul - 64*64 - 50000 - Parallelism = $i:"
#     ./benchmarks/syntheticDAGs/synbench 3 64 1024 1280 1 50000 0 0 $i
#     echo "MatMul - 256 * 256 - 10000 - Parallelism = $i:"
#     ./benchmarks/syntheticDAGs/synbench 3 64 0 0 1 50000 0 0 $i > test.txt
#     echo "COPY - 2048 * 2048 - 50000 - Parallelism = $i:"
#     ./benchmarks/syntheticDAGs/synbench 3 0 2048 0 1 0 50000 0 $i > test.txt
#     echo "Stencil - 1024 * 1024 - 50000 - Parallelism = $i:"
#     ./benchmarks/syntheticDAGs/synbench 3 0 0 1024 1 0 0 50000 $i > test.txt
#     echo "AEQUITAS End! "
#     echo "/*----------------------------------------------*/"
#     # sleep 5
#     echo ""
#     echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#     echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
#   done
# done



###### fibonacci ######
# for((j=0;j<10;j++))
# do
#   echo "/*---------------------------------------------------------------*/"
#   echo "AEQUITAS Scheduler Begin the $j execution! "
#   ./benchmarks/fibonacci/fibonacci 3 1 55 34 > Fibonacci_AEQUITAS_${j}.txt
#   sleep 5
#   echo "AEQUITAS End! "
#   echo "/*---------------------------------------------------------------*/"
#   echo ""
#   chown nvidia Fibonacci_AEQUITAS_${j}.txt
#   echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#   echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
# done

###### 2D Heat ######
# for((j=10;j<20;j++))
# do
#   echo "/*---------------------------------------------------------------*/"
#   echo "AEQUITAS Scheduler Begin the $j execution! "
#   # ./benchmarks/heat/heat-tao ./benchmarks/heat/small.dat > heat_aequitas_${j}.txt
#   ./benchmarks/heat/heat-tao ./benchmarks/heat/big.dat > heat_aequitas_${j}.txt
#   sleep 5
#   echo "AEQUITAS End! "
#   echo "/*---------------------------------------------------------------*/"
#   echo ""
#   chown nvidia heat_aequitas_${j}.txt
#   echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#   echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
# done

###### Dot Product ######
# for((j=0;j<20;j++))
# do
#   echo "/*---------------------------------------------------------------*/"
#   echo "AEQUITAS Scheduler Begin the $j execution! "
#   ./benchmarks/dotproduct/dotprod 3 100 64000000 1 320000 > dotprod_${k}.txt
#   sleep 5
#   echo "AEQUITAS End! "
#   echo "/*---------------------------------------------------------------*/"
#   echo ""
#   chown nvidia dotprod_${k}.txt
#   echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed
#   echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
# done




