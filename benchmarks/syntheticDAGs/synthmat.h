#ifndef SYNTH_MUL
#define SYNTH_MUL

#include "tao.h"
#include "dtypes.h"

#include <chrono>
#include <iostream>
#include <atomic>
#include <cmath>
#include <stdio.h>
#include <string.h>
#define PSLACK 8  

// Matrix multiplication, tao groupation on written value
class Synth_MatMul : public AssemblyTask 
{
public: 
#ifdef DVFS
  static float time_table[][XITAO_MAXTHREADS][XITAO_MAXTHREADS];
#else
  static float time_table[][XITAO_MAXTHREADS];
#endif

  Synth_MatMul( uint32_t _size, int _width, real_t *_A, real_t *_B, real_t *_C): AssemblyTask(_width), A(_A), B_Trans(_B), C(_C) {   
    dim_size = _size;
    block_size = dim_size / (_width * PSLACK);
    if(block_size == 0) block_size = 1;
    block_index = 0;
    uint32_t elem_count = dim_size * dim_size;
    block_count = dim_size / block_size;
  }

  void cleanup() { 
  //  delete[] A;
  //  delete[] B_Trans;
  //  delete[] C;
  }

  // this assembly can work totally asynchronously
  void execute(int threadid) {
/*
#ifdef DEBUG
    LOCK_ACQUIRE(output_lck);
    std::cout << "dim_size = " << dim_size << ", block_size = " << block_size << ", block_count = " << block_count << std::endl;
    LOCK_RELEASE(output_lck);
#endif
*/
    while(true) {
//      std::cout << "row_block_id = " << row_block_id << std::endl;
      int row_block_id = block_index++;
/*
#ifdef DEBUG
      LOCK_ACQUIRE(output_lck);
      std::cout << "row_block_id = " << row_block_id << std::endl;
      LOCK_RELEASE(output_lck);
#endif
*/
      if(row_block_id > block_count) return;
      // assume B is transposed, so that you can utilize the performance of transposed matmul 
      for (int i = row_block_id * block_size; i < dim_size && i < ((row_block_id + 1 ) * block_size); ++i) { 
/*
#ifdef DEBUG
      LOCK_ACQUIRE(output_lck);
      std::cout << "i = " << i << std::endl;
      LOCK_RELEASE(output_lck);
#endif
*/
        for (int j = 0; j < dim_size; j++) {
          real_t res  = 0;
          for (int k = 0; k < dim_size; k++) {
            res += A[i*dim_size+k]*B_Trans[j*dim_size+k];
          }
          C[i*dim_size+j] = res;
        }
      }
    }
  }

#if defined(Haswell)
  // float get_power(int thread, int real_core_use, int real_use_bywidth) {
  //   float power[2] = {22.73, 4.48};
  //   float total_power = 0;
  //   total_power = power[0] + power[1] * (real_core_use - 1) / real_use_bywidth;
  //   return total_power;
  // }
  float Haswell_Dyna_power(int thread, int width) {
    float dyna_power = 0;
    switch(width){
      case 1:
        if(thread < 10){
          dyna_power = 19.89;
        }
        else{
          dyna_power = 20.21;
        }
        return dyna_power;
        break;
      
      case 2:
        if(thread < 10){
          dyna_power = 26.11;
        }
        else{
          dyna_power = 26.1;
        }
        return dyna_power;
        break;

      case 5:
        if(thread < 10){
          dyna_power = 35.2;
        }
        else{
          dyna_power = 35.01;
        }
        return dyna_power;
        break;

      case 10:
        if(thread < 10){
          dyna_power = 57.64;
        }
        else{
          dyna_power = 56.53;
        }
        return dyna_power;
        break;
    }
  }
#endif

// #if defined(TX2)
//   float get_power(int thread, int real_core_use, int real_use_bywidth) {
//     float compute_denver[2][2] = {2046, 2046, 228, 152};
//     float compute_a57[2][2] = {988, 809, 76, 76};
//     float total_power = 0;
//     if(thread < 2){
//       total_power = (compute_denver[denver_freq][0] + compute_denver[denver_freq][1] * (real_core_use - 1)) / real_use_bywidth;
//     }else{
//       total_power = (compute_a57[a57_freq][0] + compute_a57[a57_freq][1] * (real_core_use - 1)) / real_use_bywidth;
//     }
//     return total_power;
//   }

//   int get_power_ori(int thread, int width) {
//     int compute_denver[2][2] = {2046, 2046, 228, 152};
//     int compute_a57[2][2] = {988, 809, 76, 76};
// 		int compute_denver_static = 76;
//     int compute_a57_static = 152;
//     int total_power = 0;
//     int dyna_power = 0;
//     if(thread < 2){
//       total_power = compute_denver[denver_freq][0] + compute_denver[denver_freq][1] * (width-1);
//     }else{
//       total_power = compute_a57[a57_freq][0] + compute_a57[a57_freq][1] * (width-1);
//     }
//     return total_power;
//   }
// #endif

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
  void set_timetable(int threadid, float ticks, int index) {
    time_table[index][threadid] = ticks;
  }

  float get_timetable(int threadid, int index) { 
    float time=0;
    time = time_table[index][threadid];
    return time;
  }
#endif
private:
  std::atomic<int> block_index; 
  int dim_size;
  int block_count;
  int block_size;
  real_t* A, *B_Trans, *C;
};

#endif
