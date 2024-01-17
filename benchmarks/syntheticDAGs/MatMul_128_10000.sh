#!/usr/bin/env bash
#SBATCH -A SNIC2019-3-293 -p hebbe
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH -t 0-05:00:00
#SBATCH --error=job.%J.err 
#SBATCH --output=MatMul_128_10000.txt

export XITAO_LAYOUT_PATH=../../ptt_layout

## MatCopy
for i in {2..22..1}
do
    echo "Parallelism = $i: "
    echo "************************* RWSS-No Sleep ***********************"
    for k in 1 2
    do
        ./MM_RWSS_NoS 0 128 0 0 1 10000 0 0 $i 
        sleep 5
    done
    echo "************************* RWSS-with Sleep *********************"
    for k in 1 2
    do
        ./MM_RWSS_S 0 128 0 0 1 10000 0 0 $i 
        sleep 5
    done
    echo "************************* FCAS-No Sleep *********************"
    for k in 1 2
    do
        ./MM_FCAS_NoS 0 128 0 0 1 10000 0 0 $i 
        sleep 5
    done
    echo "************************* FCAS-with Sleep *********************"
    for k in 1 2
    do
        ./MM_FCAS_S 0 128 0 0 1 10000 0 0 $i 
        sleep 5
    done
    echo "******************* EAS-No Sleep-No Criticality ***************"
    for k in 1 2
    do
        ./MM_EAS_NoC_NoS 0 128 0 0 1 10000 0 0 $i 
        sleep 5
    done
    echo "******************* EAS-with Sleep-No Criticality *************"
    for k in 1 2
    do
        ./MM_EAS_NoC_S 0 128 0 0 1 10000 0 0 $i
        sleep 5
    done
    echo "***************** EAS-No Sleep-with Criticality (Perf) ***************"
    for k in 1 2
    do
        ./MM_EAS_C_NoS_Perf 0 128 0 0 1 10000 0 0 $i 
        sleep 5
    done
    echo "***************** EAS-with Sleep-with Criticality (Perf) *************"
    for k in 1 2
    do
        ./MM_EAS_C_S_Perf 0 128 0 0 1 10000 0 0 $i 
        sleep 5
    done
    echo "***************** EAS-No Sleep-with Criticality (Cost) ***************"
    for k in 1 2
    do
        ./MM_EAS_C_NoS_Cost 0 128 0 0 1 10000 0 0 $i 
        sleep 5
    done
    echo "***************** EAS-with Sleep-with Criticality (Cost) *************"
    for k in 1 2
    do
        ./MM_EAS_C_S_Cost 0 128 0 0 1 10000 0 0 $i 
        sleep 5
    done
    echo "******************* PTT - EAS-No Sleep-No Criticality ***************"
    for k in 1 2
    do
        ./MM_EAS_NoC_NoS_PTT 0 128 0 0 1 10000 0 0 $i 
        sleep 5
    done
    echo "******************* PTT - EAS-with Sleep-No Criticality *************"
    for k in 1 2
    do
        ./MM_EAS_NoC_S_PTT 0 128 0 0 1 10000 0 0 $i
        sleep 5
    done
    echo "***************** PTT - EAS-No Sleep-with Criticality (Perf) ***************"
    for k in 1 2
    do
        ./MM_EAS_C_NoS_Perf_PTT 0 128 0 0 1 10000 0 0 $i 
        sleep 5
    done
    echo "***************** PTT - EAS-with Sleep-with Criticality (Perf) *************"
    for k in 1 2
    do
        ./MM_EAS_C_S_Perf_PTT 0 128 0 0 1 10000 0 0 $i 
        sleep 5
    done
    echo "***************** PTT - EAS-No Sleep-with Criticality (Cost) ***************"
    for k in 1 2
    do
        ./MM_EAS_C_NoS_Cost_PTT 0 128 0 0 1 10000 0 0 $i 
        sleep 5
    done
    echo "***************** PTT - EAS-with Sleep-with Criticality (Cost) *************"
    for k in 1 2
    do
        ./MM_EAS_C_S_Cost_PTT 0 128 0 0 1 10000 0 0 $i 
        sleep 5
    done
    echo "*************************************************************"
done
##&
##../uarch-configure/rapl-read/rapl-plot