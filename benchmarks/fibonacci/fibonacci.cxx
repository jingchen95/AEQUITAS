/*! @example 
 @brief A a program that calculates the Nth Fibonacci term in parallel using XiTAO\n
 we run the example as ./fibonacci <fibonacci_term_number> [grain_size] \n
 where
 \param fibonacci_term_number  := the Fib term to be found\n
 \param grain_size := when to stop creating TAOs (the higher the coarser creation)\n
*/
#include "fibonacci.h"
#include <fstream>

#ifdef DVFS
float FibTAO::time_table[FREQLEVELS][XITAO_MAXTHREADS][XITAO_MAXTHREADS];
#else
float FibTAO::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS];
#endif

const char *scheduler[] = { "PerformanceBased", "EnergyAware", "EDPAware", "RWSS"/* etc */ };

int main(int argc, char** argv) {
  // accept the term number as argument
  if(argc < 3) {
    // prompt for input
    std::cerr << "Usage: ./fibonacci <schedulerid> <fibonacci_term_number> [grain_size]" << std::endl;
    // return error
    return -1;
  }
  int schedulerid = atoi(argv[1]);
  int width =  atoi(argv[2]);
  uint32_t num = atoi(argv[3]); // fetch the number
  grain_size = (argc > 4)? atoi(argv[4]) : num - 2; // you can optionally pass coarsening degree
  // extra guard against error value decided by program
  if(grain_size <= 2) grain_size = 2;
  // error check
  if(num > MAX_FIB or num < 0) {
    // print error
    std::cerr << "Bounds error! acceptable range for term: 0 <= term <= " << MAX_FIB << std::endl;
    // return error
    return -1;  
  }  
  // error check
  if(grain_size >= num or grain_size < 2) {
    // print error
    std::cerr << "Bounds error! acceptable range for grain size: 2 <= grain <= " << num << std::endl;
    // return error
    return -1;  
  }  

  std::cout << "---------------------- Test Application - Fibonacci ---------------------\n";
  std::cout << "--------- You choose " << scheduler[schedulerid] << " scheduler ---------\n";

  // initialize the XiTAO runtime system 
  // gotao_init();  
  gotao_init_hw(-1,-1,-1);
  gotao_init(schedulerid, 32, 0, 0); // second parameter: parallelism => 32 (high)
    
  // Create a NULL tao as the start point => make sure all tasks are released by commit and wake up function
  fib_taos[0] = new FibTAO(1, width);
	fib_taos[0]->tasktype = 0;
  fib_taos[0]->kernel_index = 0;
	fib_taos[0]->kernel_name = "FibTAO";
	fib_taos[0]->criticality = 0;      
  // std::cout << "Creating task " << fib_taos[0]->taskid << std::endl;
  // recursively build the DAG                    
  auto parent = buildDAG(num, width);  
  // push the TAO to fire the DAG execution
  gotao_push(fib_taos[0],0);

  // start the timer
  // xitao::start("Time in XiTAO");
  
  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();
  auto start1_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(start);
  auto epoch1 = start1_ms.time_since_epoch();
  // fire the XiTAO runtime system                    
  gotao_start();          
  // end the XiTAO runtime system                       
  gotao_fini();
  // stop the timer
  end = std::chrono::system_clock::now();
  auto end1_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(end);
  auto epoch1_end = end1_ms.time_since_epoch();
  std::chrono::duration<double> elapsed_seconds = end-start;
  std::time_t end_time = std::chrono::system_clock::to_time_t(end);
  std::cout << epoch1.count() << "\t" <<  epoch1_end.count() << ", execution time: " << elapsed_seconds.count() << "s. " << std::endl;
  std::ofstream timetask;
  timetask.open("data_process.sh", std::ios_base::app);
  timetask << "python Energy.py " << epoch1.count() << "\t" <<  epoch1_end.count() << "\n";
  timetask.close();
  // xitao::stop("Time in XiTAO");
  // start the timer
  // xitao::start("Time in OpenMP");
  // call the serial code  
  // size_t val;
  // #pragma omp parallel 
  // {
  // #pragma omp single
  // { 
  //   val = fib_omp(num);
  // }
  // }
  // // stop the timer
  // // xitao::stop("Time in OpenMP");
  // // check if serial and parallel values agree
  // if(val != parent->val) {
  //   // print out the error
  //   std::cout << "Value error! Serial= " << val <<". Prallel= " << parent->val << std::endl;
  //   // return failure 
  //   return -1;
  // }
  // // print out the result
  // std::cout << "The " << num <<"th Fibonacci term = " << parent->val << std::endl;
  // return success
  return 0;
}