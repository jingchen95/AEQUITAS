/*! @example 
 @brief A program that calculates dotproduct of random two vectors in parallel\n
 we run the example as ./dotprod.out <len> <width> <block> \n
 where
 \param len  := length of vector\n
 \param width := width of TAOs\n
 \param block := how many elements to process per TAO
*/
#include "synthmat.h"
#include "synthcopy.h"
#include "synthstencil.h"
#include <vector>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <atomic>
#include <vector>
#include <algorithm>
#include "xitao_api.h"
using namespace xitao;

float Synth_MatMul::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS];
float Synth_MatCopy::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS];
float Synth_MatStencil::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS];

extern int NUM_WIDTH_TASK[XITAO_MAXTHREADS];

//enum scheduler{PerformanceBased, EnergyAware, EDPAware};
const char *scheduler[] = { "PerformanceBased", "EnergyAware", "EDPAware", "RWSS"/* etc */ };

int main(int argc, char *argv[])
{
  if(argc != 10) {
    std::cout << "./synbench <Scheduler ID> <MM Block Length> <COPY Block Length> <STENCIL Block Length> <Resource Width> <TAO Mul Count> <TAO Copy Count> <TAO Stencil Count> <Degree of Parallelism>" << std::endl; 
    return 0;
  }
  std::ofstream timetask;
  timetask.open("data_process.sh", std::ios_base::app);

  const int arr_size = 1 << 26;
	//const int arr_size = 1 << 16;
  real_t *A = new real_t[arr_size];
  real_t *B = new real_t[arr_size];
  real_t *C = new real_t[arr_size];
  memset(A, rand(), sizeof(real_t) * arr_size);
  memset(B, rand(), sizeof(real_t) * arr_size);
  memset(C, rand(), sizeof(real_t) * arr_size);

  gotao_init_hw(-1,-1,-1);
  int schedulerid = atoi(argv[1]);
  int mm_len = atoi(argv[2]);
  int copy_len = atoi(argv[3]);
  int stencil_len = atoi(argv[4]);
  int resource_width = atoi(argv[5]); 
  int tao_mul = atoi(argv[6]);
  int tao_copy = atoi(argv[7]);
  int tao_stencil = atoi(argv[8]);
  int parallelism = atoi(argv[9]);
  int total_taos = tao_mul + tao_copy + tao_stencil;
  int nthreads = XITAO_MAXTHREADS;
  int tao_types = 0;
  std::cout << "--------- You choose " << scheduler[schedulerid] << " scheduler ---------\n";
  int steal_DtoA = 0;
  int steal_AtoD = 0;
  
  gotao_init(schedulerid, parallelism, steal_DtoA, steal_AtoD);
    
  if(tao_mul > 0){
    tao_types++;
  }
  if(tao_copy > 0){
    tao_types++;
  }
  if(tao_stencil > 0){
    tao_types++;
  }
  //enum scheduler Sched;

  int indx = 0;
  std::ofstream graphfile;
  graphfile.open ("graph.txt");
  graphfile << "digraph DAG{\n";
  
  int current_type = 0;
  int previous_tao_id = 0;
  int current_tao_id = 0;
  AssemblyTask* previous_tao;
  AssemblyTask* startTAO;
  
  // create first TAO
  if(tao_mul > 0) {
    previous_tao = new Synth_MatMul(mm_len, resource_width,  A + indx * mm_len * mm_len, B + indx * mm_len * mm_len, C + indx * mm_len * mm_len);
    graphfile << previous_tao_id << "  [fillcolor = lightpink, style = filled];\n";
    tao_mul--;
    indx++;
    if((indx + 1) * mm_len * mm_len > arr_size) indx = 0;
  } else if(tao_copy > 0){
    previous_tao = new Synth_MatCopy(copy_len, resource_width,  A + indx * copy_len * copy_len, B + indx * copy_len * copy_len);
    graphfile << previous_tao_id << "  [fillcolor = skyblue, style = filled];\n";
    tao_copy--;
    indx++;
    if((indx + 1) * copy_len * copy_len > arr_size) indx = 0;
  } else if(tao_stencil > 0) {
    previous_tao = new Synth_MatStencil(stencil_len, resource_width, A+ indx * stencil_len * stencil_len, B+ indx * stencil_len * stencil_len);
    graphfile << previous_tao_id << "  [fillcolor = palegreen, style = filled];\n";
    tao_stencil--;
    indx++;
    if((indx + 1) * stencil_len * stencil_len > arr_size) indx = 0;
  }
  startTAO = previous_tao;
  previous_tao->criticality = 1;
  total_taos--;
    for(int i = 1; i < total_taos; i+=parallelism) {
      AssemblyTask* new_previous_tao;
      int new_previous_id;
      for(int j = 0; j < parallelism; ++j) {
        AssemblyTask* currentTAO;
        switch(current_type) {
          case 0:
            if(tao_mul > 0) { 
              currentTAO = new Synth_MatMul(mm_len, resource_width, A + indx * mm_len * mm_len, B + indx * mm_len * mm_len, C + indx * mm_len * mm_len);
              currentTAO->tasktype = 0;
              previous_tao->make_edge(currentTAO);                                 
              graphfile << "  " << previous_tao_id << " -> " << ++current_tao_id << " ;\n";
              graphfile << current_tao_id << "  [fillcolor = lightpink, style = filled];\n";
              tao_mul--;
              indx++;
              if((indx + 1) * mm_len * mm_len > arr_size) indx = 0;
              break;
            }
          case 1: 
            if(tao_copy > 0) {
              currentTAO = new Synth_MatCopy(copy_len, resource_width, A + indx * copy_len * copy_len, B + indx * copy_len * copy_len);
              currentTAO->tasktype = 1;
              previous_tao->make_edge(currentTAO); 
              graphfile << "  " << previous_tao_id << " -> " << ++current_tao_id << " ;\n";
              graphfile << current_tao_id << "  [fillcolor = skyblue, style = filled];\n";
              tao_copy--;
              indx++;
              if((indx + 1) * copy_len * copy_len > arr_size) indx = 0;
              break;
            }
          case 2: 
            if(tao_stencil > 0) {
              //currentTAO = new Synth_MatStencil(len, resource_width);
              currentTAO = new Synth_MatStencil(stencil_len, resource_width, A+ indx * stencil_len * stencil_len, B+ indx * stencil_len * stencil_len);
              currentTAO->tasktype = 2;
              previous_tao->make_edge(currentTAO); 
              graphfile << "  " << previous_tao_id << " -> " << ++current_tao_id << " ;\n";
              graphfile << current_tao_id << "  [fillcolor = palegreen, style = filled];\n";
              tao_stencil--;
              indx++;
              if((indx + 1) * stencil_len * stencil_len > arr_size) indx = 0;
              break;
            }
            
          default:
            if(tao_mul > 0) { 
              //currentTAO = new Synth_MatMul(len, resource_width);
              currentTAO = new Synth_MatMul(mm_len, resource_width, A + indx * mm_len * mm_len, B + indx * mm_len * mm_len, C + indx * mm_len * mm_len);
              previous_tao->make_edge(currentTAO); 
              graphfile << "  " << previous_tao_id << " -> " << ++current_tao_id << " ;\n";
              graphfile << current_tao_id << "  [fillcolor = lightpink, style = filled];\n";
              tao_mul--;
              break;
            }
        }
        
        if(j == parallelism - 1) {
          new_previous_tao = currentTAO;
          new_previous_tao->criticality = 1;
          new_previous_id = current_tao_id;
        }
        current_type++;
        if(current_type >= tao_types) current_type = 0;
      }
      previous_tao = new_previous_tao;
      previous_tao_id = new_previous_id;
    }
    //close the output
    graphfile << "}";
    graphfile.close();
    gotao_push(startTAO, 0);
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    auto start1_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(start);
    auto epoch1 = start1_ms.time_since_epoch();
    goTAO_start();
    goTAO_fini();
    end = std::chrono::system_clock::now();
    auto end1_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(end);
    auto epoch1_end = end1_ms.time_since_epoch();
    std::chrono::duration<double> elapsed_seconds = end-start;

#ifdef TX2
  std::cout << total_taos + 1 << "," << parallelism << "," << epoch1.count() << "\t" <<  epoch1_end.count() << "," << elapsed_seconds.count() << "," << (total_taos+1) / elapsed_seconds.count() << "\n";
#endif
  // std::cout << "\n\n";
  timetask << "python Energy.py " << epoch1.count() << "\t" <<  epoch1_end.count() << "\n";
  timetask.close();
}
