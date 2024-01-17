#include <iostream>
#include <chrono>
#include <omp.h>
#include <math.h>
#include "xitao.h"
using namespace std;

// enable a known trick to avoid redundant recursion for evaluated cases
//#define MEMOIZE
// the maximum number of Fibonacci terms that can fit in unsigned 64 bit
const uint32_t MAX_FIB = 92;

// a global variable to manage the granularity of TAO creation (coarsening level)
uint32_t grain_size;

// declare the class
class FibTAO;
// init the memoization array of TAOs 
FibTAO* fib_taos[MAX_FIB + 1];

// basic Fibonacci implementation
size_t fib(uint32_t num) {
	// return 0 for 0 and negative terms (undefined)
	if(num <= 0) return 0; 
	// return 1 for the term 1
	else if(num == 1) return 1;
	// recursively find the result
	return fib(num - 1) + fib(num - 2);
}

// basic Fibonacci implementation
size_t fib_omp(uint32_t num) {
	// return 0 for 0 and negative terms (undefined)
	if(num <= 0) return 0; 
	// return 1 for the term 1
	else if(num == 1) return 1;
	// recursively find the result
#pragma omp task if (num > grain_size)
	auto num_1 = fib_omp(num - 1);
#pragma omp task if (num > grain_size)
	auto num_2 = fib_omp(num - 2);
#pragma omp taskwait 
	return num_1 + num_2;
}

// the Fibonacci TAO (Every TAO class must inherit from AssemblyTask)
class FibTAO : public AssemblyTask {
public: 
#ifdef DVFS
  static float time_table[][XITAO_MAXTHREADS][XITAO_MAXTHREADS];
#else
  static float time_table[][XITAO_MAXTHREADS];
#endif
	// the n - 1 tao
	FibTAO* prev1;		
	// the n - 2 tao																	
	FibTAO* prev2;
	// the term number																				
	uint32_t term;		
	// the Fib value for the TAO																					
 	size_t val;							
 	// the tao construction. resource hint 1															
 	FibTAO(int _term, int width): term(_term), AssemblyTask(width) { }		
 	// the work function
 	void execute(int nthread) {	
 		// calculate locally if at required granularity
 		if(term <= grain_size){
			val = fib(term);
		} 
 		// if this is not a terminal term													
 		else{
			if(term > 1){
				// calculate the value
				// if(width == 1){
				val = prev1->val + prev2->val;
				// }  												
				// Jing - New Implementation
				// if(width == 2){
				// 	size_t val_0 = 0;
				// 	size_t val_1 = 0;
				// 	if(nthread - leader == 0){
				// 		val_0 = prev1->val;
				// 	}
				// 	if(nthread - leader == 1){
				// 		val_1 = prev2->val;
				// 	}
				// 	if(val_0 != 0  && val_1 !=0)
				// 	{
				// 		val = val_0 + val_1;
				// 	}
				// }
			}									
		}
 	}
 	void cleanup(){  }
	
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
};

// build the DAG by reversing the recursion tree 
FibTAO* buildDAG(uint32_t term, int width) {
	// gaurd against negative terms
	if(term <  0) term = 0;
	// if this is terminal term
	if(term <= 1) { 
		// create the terminal tao
		fib_taos[term] = new FibTAO(term, width);
		fib_taos[term]->tasktype = 0;
		fib_taos[term]->kernel_name = "FibTAO";
		fib_taos[term]->criticality = 0;
		fib_taos[term]->kernel_index = 0;
		// fib_taos[term]->width = wid;
		// push the tao
		gotao_push(fib_taos[term]);
		// return the tao
		return fib_taos[term];
	} 
#ifdef MEMOIZE
	// if this TAO has already been created (avoid redundant calculation)
	if(fib_taos[term]) return fib_taos[term];
#endif	

	// construct the tao			
	fib_taos[term] = new FibTAO(term, width);
	fib_taos[term]->tasktype = 0;
	fib_taos[term]->kernel_name = "FibTAO";
	fib_taos[term]->criticality = 0;
	fib_taos[term]->kernel_index = 0;
	// fib_taos[term]->width = wid;
	// std::cout << "Creating task " << fib_taos[term]->taskid << ", term = " << term << std::endl;
	// create TAOs as long as you are above the grain size
	if(term > grain_size) { 
		// build DAG of n - 1 term
		fib_taos[term]->prev1 = buildDAG(term - 1, width);
		// fib_taos[term]->prev1->tasktype = 0;
		// fib_taos[term]->prev1->kernel_name = "FibTAO";
		// fib_taos[term]->prev1->criticality = 0;
		// make edge to current
		fib_taos[term]->prev1->make_edge(fib_taos[term]);
		// build DAG of n - 1 term
		fib_taos[term]->prev2 = buildDAG(term - 2, width);
		// fib_taos[term]->prev2->tasktype = 0;
		// fib_taos[term]->prev2->kernel_name = "FibTAO";
		// fib_taos[term]->prev2->criticality = 0;
		// make edge to current
		fib_taos[term]->prev2->make_edge(fib_taos[term]);
	} else { // you have reached a terminal TAO 
		fib_taos[0]->make_edge(fib_taos[term]);
		// int queue = rand() % gotao_nthreads; //LU0 tasks are randomly executed on Denver
		// if(queue < 2){ // Schedule to Denver
		// 	fib_taos[term]->width = pow(2, rand() % 2); // Width: 1 2
		// 	fib_taos[term]->leader = (rand() % (2/fib_taos[term]->width)) * fib_taos[term]->width;
		// }else{ // Schedule to A57
		// 	fib_taos[term]->width = pow(2, rand() % 3); // Width: 1 2 4
		// 	fib_taos[term]->leader = 2 + (rand() % (4/fib_taos[term]->width)) * fib_taos[term]->width;
		// }
		// LOCK_ACQUIRE(worker_lock[fib_taos[term]->leader]);
		// worker_ready_q[fib_taos[term]->leader].push_back(fib_taos[term]);
		// LOCK_RELEASE(worker_lock[fib_taos[term]->leader]);
	}
	// return the current tao (the head of the DAG)
	return fib_taos[term];
}
