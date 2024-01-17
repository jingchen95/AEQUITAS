/*! \file 
@brief Contains the TAOs needed for sparselu
*/
#include "tao.h"
#include <chrono>
#include <iostream>
#include <atomic>

extern "C" {
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
}
// #define min(a,b) ( ((a) < (b)) ? (a) : (b) )

using namespace std;

/*! this TAO will take a set of doubles and add them all together
*/
class LU0 : public AssemblyTask 
{
public: 
  static float time_table[][XITAO_MAXTHREADS];
  //! LU0 TAO constructor. 
  /*!
    \param _in is the input vector for which the elements should be accumulated
    \param _out is the output element holding the summation     
    \param _len is the length of the vector 
    \param width is the number of resources used by this TAO
  */    
  LU0(double *_in, int _len, int width) :
        diag(_in), BSIZE(_len), AssemblyTask(width) 
  {  

  }
  //! Inherited pure virtual function that is called by the runtime to cleanup any resources (if any), held by a TAO. 
  void cleanup() {     
  }

  //! Inherited pure virtual function that is called by the runtime upon executing the TAO. 
  /*!
    \param threadid logical thread id that executes the TAO. For this TAO, we let logical core 0 only do the addition to avoid reduction
  */
  void execute(int threadid)
  {
    // let the leader do all the additions, 
    // otherwise we need to code a reduction here, which becomes too ugly
    //std::cout << "LU0 tid: " << threadid << std::endl;
    if(threadid != leader){
      std::cout << "leader != thread id \n";
    }
    int tid = threadid - leader;
    int i, j, k;
    int steps = (BSIZE+width-1 ) / width;     
    int min = tid * steps;
    int max = std::min(((tid+1)*steps), BSIZE);                                                                    
    //for (k=0; k<BSIZE; k++)
    for(k = min; k < max; k++)
       for (i=k+1; i<BSIZE; i++) {
          diag[i*BSIZE+k] = diag[i*BSIZE+k] / diag[k*BSIZE+k];
          for (j=k+1; j<BSIZE; j++)
             diag[i*BSIZE+j] = diag[i*BSIZE+j] - diag[i*BSIZE+k] * diag[k*BSIZE+j];
      }
  }
  
#if defined(CRIT_PERF_SCHED)
  void set_timetable(int threadid, float ticks, int index){
    time_table[index][threadid] = ticks;
  }

  float get_timetable(int threadid, int index){ 
    float time=0;
    time = time_table[index][threadid];
    return time;
  }
#endif

#if defined(Haswell)
  float get_power_Haswell(int thread, int width) {
    float dyna_power = 0;
    switch(width){
      case 0:
        if(thread < 10){
          dyna_power = 19.72;
        }
        else{
          dyna_power = 17.29;
        }
        return dyna_power;
        break;

      case 1:
        if(thread < 10){
          dyna_power = 22.25;
        }
        else{
          dyna_power = 21.47;
        }
        return dyna_power;
        break;
      
      case 2:
        if(thread < 10){
          dyna_power = 27.96;
        }
        else{
          dyna_power = 27.41;
        }
        return dyna_power;
        break;

      case 5:
        if(thread < 10){
          dyna_power = 36.55;
        }
        else{
          dyna_power = 36.27;
        }
        return dyna_power;
        break;

      case 10:
        if(thread < 10){
          dyna_power = 60.8;
        }
        else{
          dyna_power = 59.66;
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
// #endif
  double *diag;  /*!< TAO implementation specific double vector that holds the input to be accumulated */
  int BSIZE;     /*!< TAO implementation specific integer that holds the number of elements */
};


/*! this TAO will take a set of doubles and add them all together
*/
class FWD : public AssemblyTask 
{
public: 
  static float time_table[][XITAO_MAXTHREADS];
  //! FWD TAO constructor. 
  /*!
    \param _in is the input vector for which the elements should be accumulated
    \param _out is the output element holding the summation     
    \param _len is the length of the vector 
    \param width is the number of resources used by this TAO
  */    
  FWD(double *_in, double *_out, int _len, int width) :
        diag(_in), col(_out), BSIZE(_len), AssemblyTask(width) 
  {  

  }
  //! Inherited pure virtual function that is called by the runtime to cleanup any resources (if any), held by a TAO. 
  void cleanup() {     
  }

  //! Inherited pure virtual function that is called by the runtime upon executing the TAO. 
  /*!
    \param threadid logical thread id that executes the TAO. For this TAO, we let logical core 0 only do the addition to avoid reduction
  */
  void execute(int threadid)
  {
    // let the leader do all the additions, 
    // otherwise we need to code a reduction here, which becomes too ugly
    //std::cout << "FWD tid: " << threadid << std::endl;
    // if(threadid != leader) return;
    int tid = threadid - leader;
    int i, j, k;
    int steps = (BSIZE+width-1 ) / width;     
    int min = tid * steps;
    int max = std::min(((tid+1)*steps), BSIZE);                                                                    
    //for (k=0; k<BSIZE; k++)
    for(k = min; k < max; k++)
      for (i=k+1; i<BSIZE; i++)
        for (j=0; j<BSIZE; j++)
             col[i*BSIZE+j] = col[i*BSIZE+j] - diag[i*BSIZE+k]*col[k*BSIZE+j];
  }

#if defined(CRIT_PERF_SCHED)
  void set_timetable(int threadid, float ticks, int index){
    time_table[index][threadid] = ticks;
  }

  float get_timetable(int threadid, int index){ 
    float time=0;
    time = time_table[index][threadid];
    return time;
  }
#endif

#if defined(Haswell)
  float get_power_Haswell(int thread, int width) {
    float dyna_power = 0;
    switch(width){
      case 0:
        if(thread < 10){
          dyna_power = 19.72;
        }
        else{
          dyna_power = 17.29;
        }
        return dyna_power;
        break;

      case 1:
        if(thread < 10){
          dyna_power = 22.25;
        }
        else{
          dyna_power = 21.47;
        }
        return dyna_power;
        break;
      
      case 2:
        if(thread < 10){
          dyna_power = 27.96;
        }
        else{
          dyna_power = 27.41;
        }
        return dyna_power;
        break;

      case 5:
        if(thread < 10){
          dyna_power = 36.55;
        }
        else{
          dyna_power = 36.27;
        }
        return dyna_power;
        break;

      case 10:
        if(thread < 10){
          dyna_power = 60.8;
        }
        else{
          dyna_power = 59.66;
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
// #endif
  double *diag;  /*!< TAO implementation specific double vector that holds the input to be accumulated */
  double *col; /*!< TAO implementation specific double point to the summation*/
  int BSIZE;     /*!< TAO implementation specific integer that holds the number of elements */
};

/*! this TAO will take a set of doubles and add them all together
*/
class BDIV : public AssemblyTask 
{
public:
  static float time_table[][XITAO_MAXTHREADS];
  //! BDIV TAO constructor. 
  /*!
    \param _in is the input vector for which the elements should be accumulated
    \param _out is the output element holding the summation     
    \param _len is the length of the vector 
    \param width is the number of resources used by this TAO
  */    
  BDIV(double *_in, double *_out, int _len, int width) :
        diag(_in), row(_out), BSIZE(_len), AssemblyTask(width) 
  {  

  }
  //! Inherited pure virtual function that is called by the runtime to cleanup any resources (if any), held by a TAO. 
  void cleanup() {     
  }

  //! Inherited pure virtual function that is called by the runtime upon executing the TAO. 
  /*!
    \param threadid logical thread id that executes the TAO. For this TAO, we let logical core 0 only do the addition to avoid reduction
  */
  void execute(int threadid)
  {
    // let the leader do all the additions, 
    // otherwise we need to code a reduction here, which becomes too ugly
    //std::cout << "BDIV tid: " << threadid << std::endl;
    int tid = threadid - leader;
    int i, j, k;
    int steps = (BSIZE+width-1)/width;     
    int min = tid * steps;
    int max = std::min(((tid+1)*steps), BSIZE);                                                                    
    for(i = min; i < max; i++){
       for (k=0; k<BSIZE; k++) {
          row[i*BSIZE+k] = row[i*BSIZE+k] / diag[k*BSIZE+k];
          for (j=k+1; j<BSIZE; j++)
             row[i*BSIZE+j] = row[i*BSIZE+j] - row[i*BSIZE+k]*diag[k*BSIZE+j];
       }
    }
  }

#if defined(CRIT_PERF_SCHED)
  void set_timetable(int threadid, float ticks, int index){
    time_table[index][threadid] = ticks;
  }

  float get_timetable(int threadid, int index){ 
    float time=0;
    time = time_table[index][threadid];
    return time;
  }
#endif

#if defined(Haswell)
  float get_power_Haswell(int thread, int width) {
    float dyna_power = 0;
    switch(width){
      case 0:
        if(thread < 10){
          dyna_power = 19.72;
        }
        else{
          dyna_power = 17.29;
        }
        return dyna_power;
        break;

      case 1:
        if(thread < 10){
          dyna_power = 22.25;
        }
        else{
          dyna_power = 21.47;
        }
        return dyna_power;
        break;
      
      case 2:
        if(thread < 10){
          dyna_power = 27.96;
        }
        else{
          dyna_power = 27.41;
        }
        return dyna_power;
        break;

      case 5:
        if(thread < 10){
          dyna_power = 36.55;
        }
        else{
          dyna_power = 36.27;
        }
        return dyna_power;
        break;

      case 10:
        if(thread < 10){
          dyna_power = 60.8;
        }
        else{
          dyna_power = 59.66;
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
// #endif
  double *diag; /*!< TAO implementation specific double point to the summation*/
  double *row;  /*!< TAO implementation specific double vector that holds the input to be accumulated */
  int BSIZE;     /*!< TAO implementation specific integer that holds the number of elements */
};

/*! this TAO will take a set of doubles and add them all together
*/
class BMOD : public AssemblyTask 
{
public: 
  static float time_table[][XITAO_MAXTHREADS];
  //! BMOD TAO constructor. 
  /*!
    \param _in is the input vector for which the elements should be accumulated
    \param _out is the output element holding the summation     
    \param _len is the length of the vector 
    \param width is the number of resources used by this TAO
  */    
  BMOD(double *_in1, double *_in2,  double *_out, int _len, int width) :
        row(_in1), col(_in2), inner(_out), BSIZE(_len), AssemblyTask(width) 
  {  

  }
  //! Inherited pure virtual function that is called by the runtime to cleanup any resources (if any), held by a TAO. 
  void cleanup() {     
  }
  //! Inherited pure virtual function that is called by the runtime upon executing the TAO. 
  /*!
    \param threadid logical thread id that executes the TAO. For this TAO, we let logical core 0 only do the addition to avoid reduction
  */
  void execute(int threadid)
  {
    // let the leader do all the additions, 
    // otherwise we need to code a reduction here, which becomes too ugly
    //std::cout << "BMOD tid: " << threadid << std::endl;
    // if(threadid != leader) return;
    int tid = threadid - leader;
    int i, j, k;
    int steps = (BSIZE+width-1 ) / width;     
    int min = tid * steps;
    int max = std::min(((tid+1)*steps), BSIZE);                                                                    
    for(i = min; i < max; i++)
       for (k=0; k<BSIZE; k++) 
          for (j=0; j<BSIZE; j++)
             inner[i*BSIZE+j] = inner[i*BSIZE+j] - row[i*BSIZE+k]*col[k*BSIZE+j];
  }

#if defined(CRIT_PERF_SCHED)
  void set_timetable(int threadid, float ticks, int index){
    time_table[index][threadid] = ticks;
  }

  float get_timetable(int threadid, int index){ 
    float time=0;
    time = time_table[index][threadid];
    return time;
  }
#endif

#if defined(Haswell)
  float get_power_Haswell(int thread, int width) {
    float dyna_power = 0;
    switch(width){
      case 0:
        if(thread < 10){
          dyna_power = 19.72;
        }
        else{
          dyna_power = 17.29;
        }
        return dyna_power;
        break;

      case 1:
        if(thread < 10){
          dyna_power = 22.25;
        }
        else{
          dyna_power = 21.47;
        }
        return dyna_power;
        break;
      
      case 2:
        if(thread < 10){
          dyna_power = 27.96;
        }
        else{
          dyna_power = 27.41;
        }
        return dyna_power;
        break;

      case 5:
        if(thread < 10){
          dyna_power = 36.55;
        }
        else{
          dyna_power = 36.27;
        }
        return dyna_power;
        break;

      case 10:
        if(thread < 10){
          dyna_power = 60.8;
        }
        else{
          dyna_power = 59.66;
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
// #endif
  double *row;  /*!< TAO implementation specific double vector that holds the input to be accumulated */
  double *col;  /*!< TAO implementation specific double vector that holds the input to be accumulated */
  double *inner; /*!< TAO implementation specific double point to the summation*/
  int BSIZE;     /*!< TAO implementation specific integer that holds the number of elements */
};
