#ifndef SYNTH_COPY
#define SYNTH_COPY

#include "tao.h"
#include "dtypes.h"
#include <chrono>
#include <iostream>
#include <atomic>
#include <cmath>
#include <stdio.h>
#define PSLACK 8  

// Matrix multiplication, tao groupation on written value
class Synth_MatCopy : public AssemblyTask 
{
public: 
#ifdef DVFS
  static float time_table[][XITAO_MAXTHREADS][XITAO_MAXTHREADS];
#else
  static float time_table[][XITAO_MAXTHREADS];
#endif

  Synth_MatCopy(uint32_t _size, int _width, real_t *_A, real_t *_B): AssemblyTask(_width), A(_A), B(_B) {   
    dim_size = _size;
    block_size = dim_size / (_width * PSLACK);
    if(block_size == 0) block_size = 1;
    block_index = 0;
    uint32_t elem_count = dim_size * dim_size;
/*    A = new real_t[elem_count]; 
    memset(A, rand(), elem_count*sizeof(real_t)); 
    B = new real_t[elem_count];
    memset(B, rand(), elem_count*sizeof(real_t)); 
*/
    block_count = dim_size / block_size;

    // input = a;
    // output = b;
    // width = _width;
    // size = _size*_size;
    // jobs = 0;
    // job_lock.lock = false;
  }

  void cleanup(){ 
    //delete[] A;
    //delete[] B;
  }

  // this assembly can work totally asynchronously
  void execute(int threadid) {
    // int slize = pow(width,2);
    // int i;
    // while(1){ 
    //   LOCK_ACQUIRE(job_lock); //Maybe Test-and-set would be better
		// 	i = jobs++;
		// 	LOCK_RELEASE(job_lock)              	
		// 	if (i >= slize) { // no more work to be don
		// 		break;
    //   }
    //   //int virt_id = leader - threadid;
    //   //int i = (virt_id % width) * (size/width); 
    //   //memcpy(&input[i],&output[i], (size/width)*4);
    //   memcpy(&B[i*(size/slize)],&A[i*(size/slize)], (size/slize));
    // }

    while(true) {
      int row_block_id = block_index++;
      if(row_block_id > block_count) return;
      int row_block_start =  row_block_id      * block_size;
      int row_block_end   = (row_block_id + 1) * block_size;
      int end = (dim_size < row_block_end) ? dim_size : row_block_end; 
      for (int i = row_block_start; i < end; ++i) { 
         std::copy(A + (i * dim_size), A + (i * dim_size) + dim_size, B + i * dim_size);
      }
    }
  }

#if defined(Haswell)
  // float get_power(int thread, int real_core_use, int real_use_bywidth) {
  //   float power[2] = {25.24, 4.46};
  //   float total_power = 0;
  //   total_power = power[0] + power[1] * (real_core_use - 1) / real_use_bywidth;
  //   return total_power;
  // }
  float Haswell_Dyna_power(int thread, int width) {
    float dyna_power = 0;
    switch(width){
      case 1:
        if(thread < 10){
          dyna_power = 24.59;
        }
        else{
          dyna_power = 24.67;
        }
        return dyna_power;
        break;
      
      case 2:
        if(thread < 10){
          dyna_power = 33.55;
        }
        else{
          dyna_power = 32.95;
        }
        return dyna_power;
        break;

      case 5:
        if(thread < 10){
          dyna_power = 48.12;
        }
        else{
          dyna_power = 46.69;
        }
        return dyna_power;
        break;

      case 10:
        if(thread < 10){
          dyna_power = 65.25;
        }
        else{
          dyna_power = 64.08;
        }
        return dyna_power;
        break;
    }
  }
#endif

// #if defined(TX2)
//   float get_power(int thread, int real_core_use, int real_use_bywidth) {
//     float denver[2][2] = {1590, 1356, 304, 304};
//     float a57[2][2] = {985, 327, 76, 57};
//     float total_dyna_power = 0;
//     if(thread < 2){ 
//       total_dyna_power = (denver[denver_freq][0] + denver[denver_freq][1] * (real_core_use - 1)) / real_use_bywidth;
//     }else{
//       total_dyna_power = (a57[a57_freq][0] + a57[a57_freq][1] * (real_core_use - 1)) / real_use_bywidth;
//     }
//     return total_dyna_power;
//   }

//   int get_power_ori(int thread, int width) {
//     int denver[2][2] = {2195, 1582, 228, 152};
//     int a57[2][2] = {1061, 554, 76, 57};
//     //int denver[2][2]={1590, 1356, 152, 152};
//     //int a57[2][4] = {985, 378, 301, 302, 75, 76, 76, 0};
//     int denver_static = 76;
//     int a57_static = 152;
//     int total_power = 0;
//     int dyna_power = 0;
//     if(thread < 2){ 
//       total_power = denver[denver_freq][0] + denver[denver_freq][1] * (width-1);
//       //total_power = denver_static + denver[denver_freq][0] + denver[denver_freq][1] * (width-1);
//     }else{
//     /*
//       if(width = 1){
//         total_power = a57[a57_freq][0];
//       }
//       if(width =2 ){
//         total_power = a57[a57_freq][0] + a57[a57_freq][1];
//       }
//       if(width = 4){
//         total_power = a57[a57_freq][0] + a57[a57_freq][1] + a57[a57_freq][2] + a57[a57_freq][3];
//       }
//       */
//       total_power = a57[a57_freq][0] + a57[a57_freq][1] * (width-1);
//     }
//     return total_power;
//   }
// #endif
//#if defined(CRIT_PERF_SCHED)
#ifdef DVFS
  void set_timetable(int env, int threadid, float ticks, int index) {
    time_table[env][index][threadid] = ticks;
  }

  float get_timetable(int env, int threadid, int index) { 
    float time=0;
    time = time_table[env][index][threadid];
    return time;
  }
#else
  void set_timetable(int threadid, float ticks, int index){
    time_table[index][threadid] = ticks;
  }

  float get_timetable(int threadid, int index){ 
    float time=0;
    time = time_table[index][threadid];
    return time;
  }
#endif
private:
  // int size;
  // int width;
  // int jobs;
  // GENERIC_LOCK(job_lock);
  std::atomic<int> block_index; 
  int block_size; 
  int dim_size;
  int block_count;
  real_t *A, *B;
};
#endif
