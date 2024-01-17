#ifndef _CONFIG_H
#define _CONFIG_H

/* Open debug option*/
//#define DEBUG

/* Power Model for Hebbe in Synthetic DAG Benchmarks */
// #define Haswell
// #define NUMSOCKETS 2
// #define COREPERSOCKET 10

/* Power Model for TX2 in Synthetic DAG Benchmarks */

//#define DVFS_Before
//#define DVFS_final
//#define DVFS_1
//#define Denver_Change
//#define A57_Change

#define TX2
//#define CATA
// #define DVFS
#define TASKTYPES 2 // MM+CP
// #define FREQLEVELS 2
#define NUMSOCKETS 2
// #define EAS_PTT_TRAIN 2
/* Frequency setting: 0=>MAX, 1=>MIN */
#define A57 0
#define DENVER 0

#define STEAL_ATTEMPTS 1

/* Idle sleep try */
#define IDLE_SLEEP 100
/*Sleep time bound settings (nanoseconds)*/
#define SLEEP_LOWERBOUND 1000000
#define SLEEP_UPPERBOUND 64000000

/* Schedulers with sleep for idle work stealing loop */
#define SLEEP

/* Random working stealing with sleep */
#define RWSS_SLEEP

/* Performance-oriented with sleep */
//#define FCAS_SLEEP
// #define CRI_COST
//#define CRI_PERF

/* Energy aware scheduler */
//#define EAS_SLEEP
//#define EAS_NoCriticality

// #define ACCURACY_TEST

/* Enable or disable work stealing */
#define WORK_STEALING

/* Accumulte the total exec time this thread complete */
#define EXECTIME

/* Accumulte the number of task this thread complete */
//#define NUMTASKS
#define NUMTASKS_MIX

/* Accumulte the PTT visiting time */
// #define OVERHEAD_PTT

// #define ONLYCRITICAL

// #define PARA_TEST

//#define NEED_BARRIER

#define GOTAO_THREAD_BASE 0
#define GOTAO_NO_AFFINITY (1.0)
#define TASK_POOL 100
#define TAO_STA 1
#define XITAO_MAXTHREADS 6
#define L1_W   1
#define L2_W   2
#define L3_W   6
////#define L4_W   12
////#define L5_W   48
#define TOPOLOGY { L1_W, L2_W}
#define GOTAO_HW_CONTEXTS 1

//Defines for hetero environment if Weight or Crit_Hetero scheduling
//#define LITTLE_INDEX 0
//#define LITTLE_NTHREADS 4
//#define BIG_INDEX 4
//#define BIG_NTHREADS 4

//#define KMEANS
#endif
