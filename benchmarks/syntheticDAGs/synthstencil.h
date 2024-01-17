#ifndef SYNTH_STENCIL
#define SYNTH_STENCIL

#include "tao.h"
#include "dtypes.h"

#include <chrono>
#include <iostream>
#include <atomic>
#include <cmath>
#include <vector>
//#define PSLACK 4 
#define SPSLACK 4

// Matrix multiplication, tao groupation on written value
class Synth_MatStencil : public AssemblyTask 
{
public: 
#ifdef DVFS
  static float time_table[][XITAO_MAXTHREADS][XITAO_MAXTHREADS];
#else
  static float time_table[][XITAO_MAXTHREADS];
#endif

  Synth_MatStencil(uint32_t _size, int _width, real_t *_A, real_t *_B) : AssemblyTask(_width), A(_A), B(_B) {   
    dim_size = _size;
    block_size = dim_size / (_width * SPSLACK);
    //block_size = dim_size / SPSLACK;
    if(block_size == 0) block_size = 1;
    block_index = 0;
    uint32_t elem_count = dim_size * dim_size;
/*    A = new real_t[elem_count];
    B = new real_t[elem_count];
    */
    block_count = dim_size / block_size;
  }

  void cleanup() { 
    //delete[] A;
    //delete[] B;

  }

  void execute(int threadid) {
    while(true) {
      int row_block_id = block_index++;
      if(row_block_id > block_count) return;
      int row_block_start =  row_block_id      * block_size;
      int row_block_end   = (row_block_id + 1) * block_size;
      int end = (dim_size < row_block_end) ? dim_size : row_block_end; 
      if (row_block_start == 0) row_block_start = 1;
      if (end == dim_size)      end = dim_size - 1;
      for (int i = row_block_start; i < end; ++i) { 
        for (int j = 1; j < dim_size-1; j++) {
             B[i*dim_size + j] = A[i*dim_size + j] + k * (
             A[(i-1)*dim_size + j] +
             A[(i+1)*dim_size + j] +
             A[i*dim_size + j-1] +
             A[i*dim_size + j+1] +
             (-4)*A[i*dim_size + j] );
        }
      }
    }
  }

// NEED UPDATE EXPERIMENTAL RESULTS. HERE IS MEMORY-BOUND
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
//     //float denver[2][2] = {2430, 2195, 228, 228};
//     //float a57[2][2] = {1141, 758, 76, 76};
//     float denver[2][2] = {2125, 1962, 190, 190};
//     float a57[2][2] = {1139, 756, 76, 76};
//     float total_power = 0;
//     if(thread < 2){
//       total_power = (denver[denver_freq][0] + denver[denver_freq][1] *  (real_core_use - 1)) / real_use_bywidth;
//     }else{
//       total_power = (a57[a57_freq][0] + a57[a57_freq][1] * (real_core_use - 1)) / real_use_bywidth;
//     }
//     return total_power;
//   }

//   int get_power_ori(int thread, int width) {
//     int denver[2][2] = {2430, 2195, 228, 228};
//     int a57[2][2] = {1141, 758, 76, 76};
//     int denver_static = 76;
//     int a57_static = 152;
//     int total_power = 0;
//     int dyna_power = 0;
//     if(thread < 2){
//       dyna_power = denver[denver_freq][0] + denver[denver_freq][1] * (width-1);
//       //total_power = denver_static + denver[denver_freq][0] + denver[denver_freq][1] * (width-1);
//     }else{
//       dyna_power = a57[a57_freq][0] + a57[a57_freq][1] * (width-1);
//       //total_power = a57_static + a57[a57_freq][0] + a57[a57_freq][1] * (width-1);
//     }
//     //return total_power;
//     return dyna_power;
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
  const real_t k = 0.001;
  std::atomic<int> block_index; 
  int dim_size;
  int block_count;
  int block_size;
  real_t* A, *B;
  //std::vector<std::vector<real_t> > A, B;
};

#endif
