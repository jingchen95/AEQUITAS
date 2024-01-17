#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <vector>
#include <chrono>
#include <sys/ioctl.h>
#include <cstring>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <fstream>
#include "functions.h"
#include "xitao.h"
#define CHUNK 20000
//#define CHUNK 10000

#ifdef PERF_COUNTERS
struct read_format {
  uint64_t nr;
  struct {
    uint64_t value;
    uint64_t id;
  } values[];
};
std::ofstream pmc;
#endif

//x = A*b
void spmv(int nrows,int* iA,int* jA, double* A, double* b, double* x)
{
    double sum;
    int istart,iend;

    for(int row = 0; row < nrows; row++){
        istart = iA[row];
        iend   = iA[row+1];
        sum = 0.0;
        for(int i = istart; i < iend; i++){
            sum += b[jA[i]] * A[i];
        }
        x[row] = sum;
    }
}

// diagonal preconditioner
void precond(int nrows,int* iA,int* jA, double* A, double* diag)
{
    int istart,iend;
    for(int row = 0; row < nrows; row++){
        istart = iA[row];
        iend   = iA[row+1];
        for(int i = istart; i < iend; i++){
            if( jA[i] == row )
                diag[row] = 1.0/A[i];
        }
    }
}


//y=x
void yequalsx(int nrows, double* y, double* x)
{
  for(int row = 0; row < nrows; row++){
    y[row] = x[row];
  }

}

//y=constant
void yequalsconst(int nrows, double* y, double constant)
{
  for(int row = 0; row < nrows; row++){
    y[row] = constant;
  }
}

//dot= x*y // dot product resulting in a scalar
double dotxy(int nrows, double* x, double* y, double &dot)
{

  dot=0.0;
  for(int row = 0; row < nrows; row++){
    dot += x[row] * y[row];
  }

  return dot;
}

//z= x*y // element wise multiplication resulting in a vector
void multxy(int nrows, double* z ,double* x, double*y)
{
  for(int row = 0; row < nrows; row++){
    z[row] = x[row] * y[row];
  }
}

// y= a*x + y // a is a scalar
void axpy(int nrows, double* y, double*x, double a)
{
  for(int row = 0; row < nrows; row++){
    y[row] = a*x[row] + y[row];
  }
}
// y= a*x + b*y 
void axpby(int nrows, double* y, double*x, double a, double b)
{
  for(int row = 0; row < nrows; row++){
    y[row] = a*x[row] + b*y[row];
  }
}

//solve conjugate gradient
void solve(int nrows, int* iA, int* jA, double* A, double* x, double *rhs, double* p0, double* r0, double *Ax, double* s, double* diag)
{
  std::ofstream timetask;
  timetask.open("data_process.sh", std::ios_base::app);
#ifdef PERF_COUNTERS
  pmc.open("PMC.txt", std::ios_base::app);
  // Perf Event Counters
  struct perf_event_attr pea;
  int fd1, fd2, fd3, fd4, fd5, fd6;
  uint64_t id1, id2, id3, id4, id5, id6;
  uint64_t val1, val2, val3, val4, val5, val6;
  char buf[4096];
  struct read_format* rf = (struct read_format*) buf;

  memset(&pea, 0, sizeof(struct perf_event_attr));
  pea.type = PERF_TYPE_HARDWARE;
  pea.size = sizeof(struct perf_event_attr);
  pea.config = PERF_COUNT_HW_CPU_CYCLES;
  pea.disabled = 1;
  pea.exclude_kernel = 1;
  pea.exclude_hv = 1;
  pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
  fd1 = syscall(__NR_perf_event_open, &pea, 0, -1/*!!! => Physical  Core*/, -1, 0);
  ioctl(fd1, PERF_EVENT_IOC_ID, &id1);

  memset(&pea, 0, sizeof(struct perf_event_attr));
  pea.type = PERF_TYPE_HARDWARE;
  pea.size = sizeof(struct perf_event_attr);
  pea.config = PERF_COUNT_HW_INSTRUCTIONS;
  pea.disabled = 1;
  pea.exclude_kernel = 1;
  pea.exclude_hv = 1;
  pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
  fd2 = syscall(__NR_perf_event_open, &pea, 0, -1/*!!! => Physical  Core*/, fd1 /*!!!*/, 0);
  ioctl(fd2, PERF_EVENT_IOC_ID, &id2);

  memset(&pea, 0, sizeof(struct perf_event_attr));
    // pea.type = PERF_TYPE_HARDWARE;
    pea.type = PERF_TYPE_HW_CACHE;
    pea.size = sizeof(struct perf_event_attr);
    // pea.config = PERF_COUNT_HW_CACHE_REFERENCES;
    pea.config = (PERF_COUNT_HW_CACHE_L1D) | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
    pea.disabled = 1;
    pea.exclude_kernel = 0;
    pea.exclude_hv = 1;
    pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
    fd3 = syscall(__NR_perf_event_open, &pea, 0, -1, fd1 /*!!!*/, 0);
    ioctl(fd3, PERF_EVENT_IOC_ID, &id3);

    memset(&pea, 0, sizeof(struct perf_event_attr));
    // pea.type = PERF_TYPE_HARDWARE;
    pea.type = PERF_TYPE_HW_CACHE;
    pea.size = sizeof(struct perf_event_attr);
    // pea.config = PERF_COUNT_HW_CACHE_MISSES;
    pea.config = (PERF_COUNT_HW_CACHE_L1D) | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    pea.disabled = 1;
    pea.exclude_kernel = 0;
    pea.exclude_hv = 1;
    pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
    fd4 = syscall(__NR_perf_event_open, &pea, 0, -1, fd1 /*!!!*/, 0);
    ioctl(fd4, PERF_EVENT_IOC_ID, &id4);

    memset(&pea, 0, sizeof(struct perf_event_attr));
    pea.type = PERF_TYPE_RAW;
    pea.size = sizeof(struct perf_event_attr);
    pea.config = 0x16;
    pea.disabled = 1;
    pea.exclude_kernel = 0;
    pea.exclude_hv = 1;
    pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
    fd5 = syscall(__NR_perf_event_open, &pea, 0, -1, fd1, 0);
    ioctl(fd5, PERF_EVENT_IOC_ID, &id5);

    memset(&pea, 0, sizeof(struct perf_event_attr));
    pea.type = PERF_TYPE_RAW;
    pea.size = sizeof(struct perf_event_attr);
    pea.config = 0x17;
    pea.disabled = 1;
    pea.exclude_kernel = 0;
    pea.exclude_hv = 1;
    pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
    fd6 = syscall(__NR_perf_event_open, &pea, 0, -1, fd1, 0);
    ioctl(fd6, PERF_EVENT_IOC_ID, &id6);
#endif
  
  // for(int total_iter = 0; total_iter < 20; total_iter++){
  double alpha,beta,rho1,rho0,temp1,temp2,tolmax;
  int itmax, it;
  tolmax = 1e-5;
  itmax  = 10000;
  //itmax = 1;
  yequalsx(nrows, p0, x);
  yequalsx(nrows, r0, rhs);
  spmv(nrows, iA, jA,  A,  p0, Ax );
  axpy(nrows, r0, Ax, -1.0);
  multxy(nrows, p0, diag, r0);
  dotxy(nrows, p0, r0, rho1);
  printf ("%d nrows\n", nrows);
  temp2 = 1.0;
  std::vector<double> partial_sum(gotao_nthreads);
  gotao_init_hw(-1,-1,-1);
  gotao_init(3, 6, 0, 0); // Scheduler id = 3, 
//    template <typename Proc, typename IterType> float ParForTask<Proc, IterType>::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS];
  int width = 1;    
#ifdef PERF_COUNTERS
  ioctl(fd1, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
  ioctl(fd1, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
#endif
  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();
  auto start1_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(start);
  auto epoch1 = start1_ms.time_since_epoch();
  std::cout << "Alya starting time: " << epoch1.count() << std::endl;
  for(it = 0; ((tolmax <= sqrt(temp2)) || (it == 0)) && (it < itmax); it++){
#ifdef DEBUG
    std::cout << "Iteration " << it << " Begins... \n";
#endif
    int row;       
    auto data_parallel_1 = __xitao_vec_multiparallel_region_sync(width, row, 0, nrows, xitao_vec_static, CHUNK,
      int istart = iA[row];
      int iend   = iA[row+1];
      double sum = 0.0;
      for(int i = istart; i < iend; i++){
        sum += p0[jA[i]] * A[i];
      }
      Ax[row] = sum;      
    );

    partial_sum.assign(gotao_nthreads, 0);
    auto data_parallel_2 = __xitao_vec_multiparallel_region_sync(width, row, 0, nrows, xitao_vec_static, CHUNK,
      partial_sum[__xitao_thread_id] += p0[row] * Ax[row];
    );

    temp1 = 0.0;
    for (auto& n : partial_sum) temp1 += n;
    alpha = rho1 / temp1;

    auto data_parallel_3 = __xitao_vec_multiparallel_region_sync(width, row, 0, nrows, xitao_vec_static, CHUNK,
      x[row] = alpha*p0[row] + x[row];
    );
    
    auto data_parallel_4 = __xitao_vec_multiparallel_region_sync(width, row, 0, nrows, xitao_vec_static, CHUNK,
      r0[row] = -alpha*Ax[row] + r0[row];
    );
   
    
    temp2 = 0.0;
    partial_sum.assign(gotao_nthreads, 0);
    auto data_parallel_5 = __xitao_vec_multiparallel_region_sync(width, row, 0, nrows, xitao_vec_static, CHUNK,
      partial_sum[__xitao_thread_id] += r0[row] * r0[row];
    );
   
    for (auto& n : partial_sum) temp2 += n;
    auto data_parallel_6 = __xitao_vec_multiparallel_region_sync(width, row, 0, nrows, xitao_vec_static, CHUNK,
      s[row] = diag[row] * r0[row];
    );      
   
    rho0 = rho1;
    rho1 = 0.0;
    partial_sum.assign(gotao_nthreads, 0);
    auto data_parallel_7 = __xitao_vec_multiparallel_region_sync(width, row, 0, nrows, xitao_vec_static, CHUNK,
      partial_sum[__xitao_thread_id] += r0[row] * s[row];
    );
   
    for (auto& n : partial_sum) rho1 += n;
    beta = rho1 / rho0;
    auto data_parallel_8 = __xitao_vec_multiparallel_region_sync(width, row, 0, nrows, xitao_vec_static, CHUNK,
      p0[row] = s[row] + beta * p0[row];
    );  
#ifdef DEBUG
    std::cout << "Iteration " << it << " complete! \n";
#endif
  }
    if( tolmax > sqrt(temp2) && it > 0 )
    {
      printf("Convergence at iteration %d tolerance %e \n",it, sqrt(temp2));
    }
    gotao_fini();
    // }
    end = std::chrono::system_clock::now();
    auto end_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(end);
    auto epoch = end_ms.time_since_epoch();
    std::cout << "Alya ending time: " << epoch.count() << std::endl;
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::cout << "Elapsed Time: " << elapsed_seconds.count() << std::endl;
#ifdef PERF_COUNTERS
    ioctl(fd1, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
    read(fd1, buf, sizeof(buf));
    for (int i = 0; i < rf->nr; i++){
        if (rf->values[i].id == id1) {
            val1 = rf->values[i].value;
        } else if (rf->values[i].id == id2) {
            val2 = rf->values[i].value;
        }else if (rf->values[i].id == id3) {
            val3 = rf->values[i].value;
        }else if (rf->values[i].id == id4) {
            val4 = rf->values[i].value;
        }else if (rf->values[i].id == id5) {
            val5 = rf->values[i].value;
        }else if (rf->values[i].id == id6) {
            val6 = rf->values[i].value;
        }
    }
    pmc << val1 << ", " << val2 << ", " << val3 << ", " << val4 << ", " << val5 << ", " << val6 << ", ";
    pmc << elapsed_seconds.count() << "\n";
    pmc.close();
    std::cout << val1 << ", " << val2 << ", " << val3 << ", " << val4 << ", " << val5 << ", " << val6 << "\n";
#endif
#ifdef NUMTASKS
    for(int a = 1; a < 2; a = a*2){
      for(int b = 0; b < gotao_nthreads; b++){
          std::cout << "Thread " << b << " with width " << a << " completes " << num_task[a * gotao_nthreads + b] << " tasks.\n";
      }
    }
#endif
  timetask << "python Energy.py " << epoch1.count() << "\t" <<  epoch.count() << "\n";
  timetask.close();
}

