#include "poly_task.h"
// #include "tao.h"
#include <errno.h> 
#include <cstring>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <numeric>
#include <iterator>
#include <chrono>
#include "xitao_workspace.h"
using namespace xitao;

const int EAS_PTT_TRAIN = 2;

#if defined(TX2)
int idleP_A57 = 152;
int idleP_Denver = 76;
#endif

extern int denver_freq;
extern int a57_freq;
#if (defined TX2) && (defined DVFS)
extern int env;
#endif

#if defined(Haswell)
extern int num_sockets;
#endif

#ifdef OVERHEAD_PTT
extern std::chrono::duration<double> elapsed_ptt;
#endif

#ifdef NUMTASKS
extern int num_task[TASKTYPES][XITAO_MAXTHREADS * XITAO_MAXTHREADS];
#endif

#ifdef DVFS
extern int PTT_UpdateFlag[FREQLEVELS][XITAO_MAXTHREADS][XITAO_MAXTHREADS];
#else
extern int PTT_UpdateFlag[XITAO_MAXTHREADS][XITAO_MAXTHREADS];
#endif

#if (defined DVFS) && (defined TX2)
extern float MM_power[NUMSOCKETS][FREQLEVELS][XITAO_MAXTHREADS];
extern float CP_power[NUMSOCKETS][FREQLEVELS][XITAO_MAXTHREADS];
#else
extern float MM_power[NUMSOCKETS][XITAO_MAXTHREADS];
extern float CP_power[NUMSOCKETS][XITAO_MAXTHREADS];
#endif

extern int status[XITAO_MAXTHREADS];
extern int status_working[XITAO_MAXTHREADS];
extern int Parallelism;
extern int Sched;
extern int TABLEWIDTH;

//The pending PolyTasks count 
std::atomic<int> PolyTask::pending_tasks;

// need to declare the static class memeber
#if defined(DEBUG)
std::atomic<int> PolyTask::created_tasks;
#endif

PolyTask::PolyTask(int t, int _nthread=0) : type(t){
  refcount = 0;
#define GOTAO_NO_AFFINITY (1.0)
  affinity_relative_index = GOTAO_NO_AFFINITY;
  affinity_queue = -1;
#if defined(DEBUG) 
  taskid = created_tasks += 1;
#endif
  LOCK_ACQUIRE(worker_lock[_nthread]);
  if(task_pool[_nthread].tasks == 0){
    pending_tasks += TASK_POOL;
    task_pool[_nthread].tasks = TASK_POOL-1;
      #ifdef DEBUG
    std::cout << "[DEBUG] Requested: " << TASK_POOL << " tasks. Pending is now: " << pending_tasks << "\n";
      #endif
  }
  else task_pool[_nthread].tasks--;
  LOCK_RELEASE(worker_lock[_nthread]);
  threads_out_tao = 0;
  criticality=0;
  marker = 0;
}

// Internally, GOTAO works only with queues, not stas
int PolyTask::sta_to_queue(float x){
  if(x >= GOTAO_NO_AFFINITY){ 
    affinity_queue = -1;
  }
    else if (x < 0.0) return 1;  // error, should it be reported?
    else affinity_queue = (int) (x*gotao_nthreads);
    return 0; 
  }
int PolyTask::set_sta(float x){    
  affinity_relative_index = x;  // whenever a sta is changed, it triggers a translation
  return sta_to_queue(x);
} 
float PolyTask::get_sta(){             // return sta value
  return affinity_relative_index; 
}    
int PolyTask::clone_sta(PolyTask *pt) { 
  affinity_relative_index = pt->affinity_relative_index;    
  affinity_queue = pt->affinity_queue; // make sure to copy the exact queue
  return 0;
}
void PolyTask::make_edge(PolyTask *t){
  out.push_back(t);
  t->refcount++;
}

//History-based molding
//#if defined(CRIT_PERF_SCHED)
void PolyTask::history_mold(int _nthread, PolyTask *it){
  int new_width = 1; 
  int new_leader = -1;
  float shortest_exec = 1000.0f;
  float comp_perf = 0.0f; 
  auto&& partitions = inclusive_partitions[_nthread];
  if(rand()%10 != 0) { 
    for(auto&& elem : partitions) {
      int leader = elem.first;
      int width  = elem.second;
      auto&& ptt_val = 0;
#ifdef DVFS
#else
      ptt_val = it->get_timetable(leader, width - 1);
#endif
      if(ptt_val == 0.0f) {
        new_width = width;
        new_leader = leader;       
        break;
      }
#ifdef CRI_COST
      //For Cost
      comp_perf = width * ptt_val;
#endif
#ifdef CRI_PERF
      //For Performance
      comp_perf = ptt_val;
#endif
      if (comp_perf < shortest_exec) {
        shortest_exec = comp_perf;
        new_width = width;
        new_leader = leader;      
      }
    }
  } else { 
    auto&& rand_partition = partitions[rand() % partitions.size()];
    new_leader = rand_partition.first;
    new_width  = rand_partition.second;
  }
  if(new_leader != -1) {
    it->leader = new_leader;
    it->width  = new_width;
  }
} 
  //Recursive function assigning criticality
int PolyTask::set_criticality(){
  if((criticality)==0){
    int max=0;
    for(std::list<PolyTask *>::iterator edges = out.begin();edges != out.end();++edges){
      int new_max =((*edges)->set_criticality());
      max = ((new_max>max) ? (new_max) : (max));
    }
    criticality=++max;
  } 
  return criticality;
}

int PolyTask::set_marker(int i){
  for(std::list<PolyTask *>::iterator edges = out.begin(); edges != out.end(); ++edges){
    if((*edges) -> criticality == critical_path - i){
      (*edges) -> marker = 1;
      i++;
      (*edges) -> set_marker(i);
      break;
    }
  }
  return marker;
}

//Determine if task is critical task
int PolyTask::if_prio(int _nthread, PolyTask * it){
#ifdef EAS_NoCriticality
	if((Sched == 1) || (Sched == 2)){
	  return 0;
  }
	if(Sched == 0){
#endif
    return it->criticality;
#ifdef EAS_NoCriticality
  }
#endif
}

// #ifdef DVFS
// void PolyTask::print_ptt(float table[][XITAO_MAXTHREADS][XITAO_MAXTHREADS], const char* table_name) { 
// #else
void PolyTask::print_ptt(float table[][XITAO_MAXTHREADS], const char* table_name) { 
// #endif
  std::cout << std::endl << table_name <<  " PTT Stats: " << std::endl;
  for(int leader = 0; leader < ptt_layout.size() && leader < gotao_nthreads; ++leader) {
    auto row = ptt_layout[leader];
    std::sort(row.begin(), row.end());
    std::ostream time_output (std::cout.rdbuf());
    std::ostream scalability_output (std::cout.rdbuf());
    std::ostream width_output (std::cout.rdbuf());
    std::ostream empty_output (std::cout.rdbuf());
    time_output  << std::scientific << std::setprecision(3);
    scalability_output << std::setprecision(3);    
    empty_output << std::left << std::setw(5);
    std::cout << "Thread = " << leader << std::endl;    
    std::cout << "==================================" << std::endl;
    std::cout << " | " << std::setw(5) << "Width" << " | " << std::setw(9) << std::left << "Time" << " | " << "Scalability" << std::endl;
    std::cout << "==================================" << std::endl;
    for (int i = 0; i < row.size(); ++i) {
      int curr_width = row[i];
      if(curr_width <= 0) continue;
      auto comp_perf = table[curr_width - 1][leader];
      std::cout << " | ";
      width_output << std::left << std::setw(5) << curr_width;
      std::cout << " | ";      
      time_output << comp_perf; 
      std::cout << " | ";
      if(i == 0) {        
        empty_output << " - ";
      } else if(comp_perf != 0.0f) {
        auto scaling = table[row[0] - 1][leader]/comp_perf;
        auto efficiency = scaling / curr_width;
        if(efficiency  < 0.6 || efficiency > 1.3) {
          scalability_output << "(" <<table[row[0] - 1][leader]/comp_perf << ")";  
        } else {
          scalability_output << table[row[0] - 1][leader]/comp_perf;
        }
      }
      std::cout << std::endl;  
    }
    std::cout << std::endl;
  }
}  

// void PolyTask::print_ptt(float table[][XITAO_MAXTHREADS], const char* table_name) { 
//   std::cout << std::endl << table_name <<  " PTT Stats: " << std::endl;
//   for(int leader = 0; leader < ptt_layout.size() && leader < gotao_nthreads; ++leader) {
//     auto row = ptt_layout[leader];
//     std::sort(row.begin(), row.end());
//     std::ostream time_output (std::cout.rdbuf());
//     std::ostream scalability_output (std::cout.rdbuf());
//     std::ostream width_output (std::cout.rdbuf());
//     std::ostream empty_output (std::cout.rdbuf());
//     time_output  << std::scientific << std::setprecision(3);
//     scalability_output << std::setprecision(3);    
//     empty_output << std::left << std::setw(5);
//     std::cout << "Thread = " << leader << std::endl;    
//     std::cout << "==================================" << std::endl;
//     std::cout << " | " << std::setw(5) << "Width" << " | " << std::setw(9) << std::left << "Time" << " | " << "Scalability" << std::endl;
//     std::cout << "==================================" << std::endl;
//     for (int i = 0; i < row.size(); ++i){
//       int curr_width = offset * XITAO_MAXTHREADS + row[i];
//       auto comp_perf = table[curr_width - 1][leader];
//       std::cout << " | ";
//       width_output << std::left << std::setw(5) << row[i];
//       std::cout << " | ";      
//       time_output << comp_perf; 
//       std::cout << " | ";
//       if(i == 0) {        
//         empty_output << " - ";
//       } else if(comp_perf != 0.0f) {
//         auto scaling = table[offset * XITAO_MAXTHREADS + row[0] - 1][leader]/comp_perf;
//         auto efficiency = scaling / row[i];
//         if(efficiency  < 0.6 || efficiency > 1.3) {
//           scalability_output << "(" <<table[offset * XITAO_MAXTHREADS + row[0] - 1][leader]/comp_perf << ")";  
//         } else {
//           scalability_output << table[offset * XITAO_MAXTHREADS + row[0] - 1][leader]/comp_perf;
//         }
//       }
//       std::cout << std::endl;  
//     }
//     std::cout << std::endl;
//   }
// }  

int PolyTask::globalsearch_Perf(int nthread, PolyTask * it){
  float shortest_exec = 100000.0f;
  float comp_perf = 0.0f; 
  int new_width = 1; 
  int new_leader = -1;
  for(int leader = 0; leader < ptt_layout.size(); ++leader) {
    for(auto&& width : ptt_layout[leader]) {
       auto&& ptt_val = 0.0f;
#if (defined DVFS) && (defined TX2)
      if(leader < 2){
        ptt_val = it->get_timetable(denver_freq, leader, width - 1);
      }else{
        ptt_val = it->get_timetable(a57_freq, leader, width - 1);
      }
#else
      ptt_val = it->get_timetable(leader, width-1);
#ifdef DEBUG
      LOCK_ACQUIRE(output_lck);
      std::cout <<"[Sched=FCC] Priority=1, ptt value(: "<< leader << "," << width << ") = " << ptt_val << std::endl;
      LOCK_RELEASE(output_lck);
#endif
#endif
      if(Sched == 0){
        if(ptt_val == 0.0f) {
          new_width = width;
          new_leader = leader; 
          leader = ptt_layout.size(); 
          break;
        }
      }
      if(Sched == 1){
#ifdef DVFS
        if(ptt_val == 0.0f) {
#else
        if(ptt_val == 0.0f|| (PTT_UpdateFlag[leader][width-1] <= EAS_PTT_TRAIN)) {
        //if(ptt_val == 0.0f) {
#endif
          new_width = width;
          new_leader = leader; 
          leader = ptt_layout.size(); 
          break;
        }
      }
      
#ifdef CRI_COST
      //For Cost
      comp_perf = width * ptt_val;
#endif
#ifdef CRI_PERF
      //For Performance
      comp_perf = ptt_val;
#endif

      if (comp_perf < shortest_exec) {
        shortest_exec = comp_perf;
        new_width = width;
        new_leader = leader;      
      }
#ifdef EAS
    }
#endif
    }
  }  
  it->width = new_width; 
  it->leader = new_leader;
  it->updateflag = 1;
#ifdef DEBUG
  LOCK_ACQUIRE(output_lck);
  std::cout <<"[Sched=FCC] Priority=1, task "<< it->taskid <<" will run on thread "<< it->leader << ", width become " << it->width << std::endl;
  LOCK_RELEASE(output_lck);
#endif
  return new_leader;
}

int PolyTask::globalsearch_PTT(int nthread, PolyTask * it){
  float shortest_exec = 100000.0f;
  float comp_perf = 0.0f;
  int new_width = 1;
  int new_leader = -1;
#ifdef DVFS
  int new_freq = -1; 
#endif
  auto idleP = 0.0f;
  float dyna_P = 0;
#ifdef ACCURACY_TEST
  auto new_idleP = 0.0f;
  float new_dynaP = 0;
#endif
  int start_idle, end_idle, sum_cluster_active, real_p = 0;
  auto&& partitions = inclusive_partitions[nthread];
// #ifdef TX2
//   int PTT_Count = 0;
//   if(PTT_Count < 20){
// #endif
/* Firstly, fill out PTT table */
#ifdef DVFS
  for(int freq = 0; freq < FREQLEVELS; freq++){
#endif
    for(int leader = 0; leader < ptt_layout.size(); ++leader) {
       for(auto&& width : ptt_layout[leader]) {
        auto&& ptt_val = 0.0f;
#ifdef DVFS
        ptt_val = it->get_timetable(freq, leader, width - 1);
#ifdef DEBUG
        LOCK_ACQUIRE(output_lck);
        std::cout << "ptt_val(freq=" << freq << ",leader=" << leader <<",width=" << width << ") = " << it->get_timetable(freq, leader, width - 1) << "\n";
        LOCK_RELEASE(output_lck);
#endif
#else
        ptt_val = it->get_timetable(leader, width - 1);
#ifdef DEBUG
        LOCK_ACQUIRE(output_lck);
        std::cout << "ptt_val(leader=" << leader <<",width=" << width << ") = " << it->get_timetable( leader, width - 1) << "\n";
        LOCK_RELEASE(output_lck);
#endif
#endif
        //if((ptt_val == 0.0f)  || (PTT_UpdateFlag[fr][leader][width-1] <= EAS_PTT_TRAIN)) {
        if(ptt_val == 0.0f){
          it->width  = width;
          it->leader = leader;
          it->updateflag = 1;
#ifdef DVFS
          new_freq = freq;
          freq = FREQLEVELS;
#endif
#if (defined TX2) && (defined DVFS)
          // Change frequency
          if(it->leader < 2){
            // first check if the frequency is the same? If so, go ahead; if not, need to change frequency!
            if(new_freq != denver_freq){
              std::cout << "[DVFS] Denver frequency changes from = " << denver_freq << " to " << new_freq << ".\n";
              // Write new frequency to Denver cluster
              if(new_freq == 0){
                system("echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed");
              }
              if(new_freq == 1){
                system("echo 345600 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed");
              }
              denver_freq = new_freq;
            }
          }else{
            // Write new frequency to A57 cluster
            if(new_freq != a57_freq){
              std::cout << "[DVFS] A57 frequency changes from = " << a57_freq << " to " << new_freq << ".\n";
              if(new_freq == 0){
                system("echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
              }
              if(new_freq == 1){
                system("echo 345600 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
              }
              a57_freq = new_freq;
            }
          }
#endif
          //leader = ptt_layout.size();
          // PTT_Count++;
          // break;
          return new_leader;
        }
        else{
          continue;
        }
        //std::cout << "After break, in poly task " << it->taskid << ", it->width = " << it->width << ", it->leader = " << it->leader << "\n";
      }
    }
#ifdef DVFS
  }  
#endif
// #ifdef TX2
//   }
//   else{
// #endif
/* Secondly, search for energy efficient one */
#ifdef DVFS
  for(int freq = 0; freq < FREQLEVELS; freq++){
#endif
    for(int leader = 0; leader < ptt_layout.size(); ++leader) {
#if defined(TX2)
      //idleP =(leader < 2)? 76 : 152; //mWatt
      if(it->tasktype == 0){
#ifdef DVFS
        idleP =(leader < 2)? MM_power[denver_freq][0][0] : MM_power[a57_freq][1][0];
#else
        idleP =(leader < 2)? MM_power[0][0] : MM_power[1][0];
#endif
      }
      if(it->tasktype == 1){
#ifdef DVFS
        idleP =(leader < 2)? CP_power[denver_freq][0][0] : CP_power[a57_freq][1][0];
#else
        idleP =(leader < 2)? CP_power[0][0] : CP_power[1][0];
#endif
      }
      start_idle = (leader < 2)? 0 : 2;
      end_idle = (leader < 2)? 2 : gotao_nthreads;
#endif

#if defined(Haswell)
      start_idle = (leader < gotao_nthreads/num_sockets)? 0 : gotao_nthreads/num_sockets;
      end_idle = (leader < gotao_nthreads/num_sockets)? gotao_nthreads/num_sockets : gotao_nthreads;
#endif
      sum_cluster_active = std::accumulate(status+ start_idle, status+end_idle, 0);

      // Real Parallelism (Dynamic Power sharing, for COPY case)
      real_p = (Parallelism < sum_cluster_active) ? Parallelism : sum_cluster_active;

#if defined(TX2)
      float idleP2 = 0;
      if(leader < 2){
        //idleP2 = 228 - (status[2] || status[3] || status[4] || status[5]) * (228-idleP);
        idleP2 = (76 + 152 + 76 * ~(a57_freq)) - (status[2] || status[3] || status[4] || status[5]) * ((76 + 152 + 76 * ~(a57_freq))-idleP);
      }else{
        //idleP2 = 228 - (status[0] || status[1]) * (228-idleP);
        idleP2 = (76 + 152 + 76 * ~(a57_freq)) - (status[0] || status[1]) * ((76 + 152 + 76 * ~(a57_freq))-idleP);
      }
#endif
      for(auto&& width : ptt_layout[leader]) {
        auto&& ptt_val = 0.0f;
#ifdef DVFS
        ptt_val = it->get_timetable(freq, leader, width - 1);
// #ifdef DEBUG
//         LOCK_ACQUIRE(output_lck);
//         std::cout << "ptt_val(freq=" << freq << ",leader=" << leader <<",width=" << width << ") = " << it->get_timetable(freq, leader, width - 1) << "\n";
//         LOCK_RELEASE(output_lck);
// #endif
        //if((ptt_val == 0.0f) || (PTT_UpdateFlag[freq][leader][width-1] <= EAS_PTT_TRAIN)){   
#else
        ptt_val = it->get_timetable(leader, width - 1);
        //if((ptt_val == 0.0f) || (PTT_UpdateFlag[leader][width-1] <= EAS_PTT_TRAIN)){
#endif
//         if(ptt_val == 0.0f) {
//           new_width = width;
//           new_leader = leader;
// #ifdef DVFS
//           new_freq = freq;
//           freq = FREQLEVELS;
// #endif
//           leader = ptt_layout.size();
//           break;
//         }
        /********* ********* Idle Power ********* *********/
        int num_active = 0;
        for(int i = leader; i < leader+width; i++){
          if(status[i] == 0){
            num_active++;
          }
        }
#ifdef TX2
        idleP = idleP2 * width / (sum_cluster_active + num_active);
#endif

#ifdef Haswell
        //idleP = idleP * width / (sum_cluster_active + num_active);
        idleP = power[leader/COREPERSOCKET][0] * width / (sum_cluster_active + num_active);
#endif 

        /********* ********* Dynamic Power ********* *********/
#ifdef TX2
        // int real_core_use = (((real_p + num_active) * width) < (end_idle - start_idle)) ? ((real_p + num_active) * width) : (end_idle - start_idle);
        // int real_use_bywidth = real_core_use / width;
        // dyna_P = it->get_power(leader, real_core_use, real_use_bywidth); 
#ifdef DVFS
        // MM tasks
        if(it->tasktype == 0){
          if(leader < 2){
            dyna_P = MM_power[denver_freq][0][width];
          }
          else{
            dyna_P = MM_power[a57_freq][1][width];
          }
        }else{
          if(it->tasktype == 1){
            if(leader < 2){
              dyna_P = CP_power[denver_freq][0][width];
            }
            else{
              dyna_P = CP_power[a57_freq][1][width];
            }
          }
        }         
        
#else
        if(it->tasktype == 0){
          if(leader < 2){
            dyna_P = MM_power[0][width];
          }
          else{
            dyna_P = MM_power[1][width];
          }
        }
        else{
          if(it->tasktype == 1){
            if(leader < 2){
              dyna_P = CP_power[0][width];
            }
            else{
              dyna_P = CP_power[1][width];
            }
          }
        }
#endif
#endif

#if defined(Haswell)
        //dyna_P = it->Haswell_Dyna_power(leader, width);
        dyna_P = power[leader/COREPERSOCKET][width];
#endif
        /********* ********* Energy Prediction ********* *********/
        // Prediction is based on idle and dynamic power
        comp_perf = ptt_val * (idleP + dyna_P);
        // Prediction is based on only dynamic power
        //comp_perf = ptt_val * dyna_P;
				if (comp_perf < shortest_exec) {
#ifdef DVFS
          new_freq = freq;
#endif
          shortest_exec = comp_perf;
          new_width = width;
          new_leader = leader;
#ifdef ACCURACY_TEST
          new_dynaP = dyna_P;
          new_idleP = idleP;
#endif
        }
		  }
	  }
#ifdef DVFS
  }
#endif
// #ifdef TX2
// }
// #endif
  it->width = new_width;
  it->leader = new_leader;
  it->updateflag = 1;
#ifdef DEBUG
  LOCK_ACQUIRE(output_lck);
  std::cout << "[DEBUG] For task " << it->taskid << ", energy efficient choce: it->width = " << it->width << ", it->leader = " << it->leader << "\n";
  LOCK_RELEASE(output_lck);
#endif
#ifdef ACCURACY_TEST
  LOCK_ACQUIRE(output_lck);
  std::chrono::time_point<std::chrono::system_clock> start;
  start = std::chrono::system_clock::now();
  auto start1_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(start);
  auto epoch1 = start1_ms.time_since_epoch();
  std::cout << "Task " << it->taskid << ", " << epoch1.count() << ", idle power: " << new_idleP << ", dynamic power: " << new_dynaP << ", total: " << new_idleP + new_dynaP << std::endl;
  LOCK_RELEASE(output_lck);
#endif
  //std::cout << "After break, in poly task " << it->taskid << ", it->width = " << it->width << ", it->leader = " << it->leader << "\n";
#if (defined TX2) && (defined DVFS)
  // Change frequency
  if(it->leader < 2){
    // first check if the frequency is the same? If so, go ahead; if not, need to change frequency!
    if(new_freq != denver_freq){
      std::cout << "[DVFS] Denver frequency changes from = " << denver_freq << " to " << new_freq << ".\n";
      // Write new frequency to Denver cluster
      
      if(new_freq == 0){
        int status = system("echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed");
        if (status < 0){
          std::cout << "Error: " << strerror(errno) << '\n';
        }
      }
      if(new_freq == 1){
        int status = system("echo 345600 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed");
        if (status < 0){
          std::cout << "Error: " << strerror(errno) << '\n';
        }
      }
      denver_freq = new_freq;
    }
  }else{
    // Write new frequency to A57 cluster
    if(new_freq != a57_freq){
      std::cout << "[DVFS] A57 frequency changes from = " << a57_freq << " to " << new_freq << ".\n";
      if(new_freq == 0){
        system("echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
      }
      if(new_freq == 1){
        system("echo 345600 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
      }
      a57_freq = new_freq;
    }
  }
#endif
  return new_leader;
}
//#endif

int PolyTask::eas_width_mold(int _nthread, PolyTask *it){
  int new_width = 1; 
  int new_leader = -1;
#ifdef DVFS
  int new_freq = -1; 
#endif
  float shortest_exec = 1000.0f;
  float comp_perf = 0.0f; 
  auto&& partitions = inclusive_partitions[_nthread];
  auto idleP = 0.0f;
  float dyna_P = 0;
  int start_idle, end_idle, sum_cluster_active, real_p = 0;

#ifdef DVFS
  for(int freq = 0; freq < FREQLEVELS; freq++){
#endif
  for(auto&& elem : partitions) {
    int leader = elem.first;
    int width  = elem.second;
    auto&& ptt_val = 0.0f;
#ifdef DVFS
    ptt_val = it->get_timetable(freq, leader, width - 1);
    //if((ptt_val == 0.0f) || (PTT_UpdateFlag[freq][leader][width-1] <= EAS_PTT_TRAIN)) {
    if(ptt_val == 0.0f) {
#else
    ptt_val = it->get_timetable(leader, width - 1);
    //if(ptt_val == 0.0f){
    if((ptt_val == 0.0f) || (PTT_UpdateFlag[leader][width-1] <= EAS_PTT_TRAIN)) {
#endif
      it->width = width;
      it->leader = leader;    
      it->updateflag = 1;
#ifdef DVFS  
      new_freq = freq; 
      freq = FREQLEVELS;
#endif
#ifdef DVFS
      // Change frequency
      if(it->leader < 2){
        // first check if the frequency is the same? If so, go ahead; if not, need to change frequency!
        if(new_freq != denver_freq){
          std::cout << "[DVFS] Denver frequency changes from = " << denver_freq << " to " << new_freq << ".\n";
          // Write new frequency to Denver cluster
          if(new_freq == 0){
            system("echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed");
          }else{
            system("echo 345600 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed");
          }
          denver_freq = new_freq;
        }
      }else{
        // Write new frequency to A57 cluster
        if(new_freq != a57_freq){
          std::cout << "[DVFS] A57 frequency changes from = " << a57_freq << " to " << new_freq << ".\n";
          if(new_freq == 0){
            system("echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
          }else{
            system("echo 345600 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
          }
          a57_freq = new_freq;
        }
      }
#endif
      return new_leader;
    }
    else{
      continue;
    }
  }
#ifdef DVFS
  }
#endif


#ifdef DVFS
  for(int freq = 0; freq < FREQLEVELS; freq++){
#endif
  for(auto&& elem : partitions) {
    int leader = elem.first;
    int width  = elem.second;
    auto&& ptt_val = 0.0f;
#ifdef DVFS
    ptt_val = it->get_timetable(freq, leader, width - 1);
#else
    ptt_val = it->get_timetable(leader, width - 1);
#endif
#if defined(TX2)
    //idleP =(leader < 2)? 76 : 152; //mWatt
    if(it->tasktype == 0){
#ifdef DVFS
      idleP =(leader < 2)? MM_power[denver_freq][0][0] : MM_power[a57_freq][1][0];
#else
      idleP =(leader < 2)? MM_power[0][0] : MM_power[1][0];
#endif
    }
    if(it->tasktype == 1){
#ifdef DVFS
      idleP =(leader < 2)? CP_power[denver_freq][0][0] : CP_power[a57_freq][1][0];
#else
      idleP =(leader < 2)? CP_power[0][0] : CP_power[1][0];
#endif
    }
    start_idle = (leader < 2)? 0 : 2;
    end_idle = (leader < 2)? 2 : gotao_nthreads;
#endif

#if defined(Haswell)
    start_idle = (leader < gotao_nthreads/num_sockets)? 0 : gotao_nthreads/num_sockets;
    end_idle = (leader < gotao_nthreads/num_sockets)? gotao_nthreads/num_sockets : gotao_nthreads;
#endif
    sum_cluster_active = std::accumulate(status+ start_idle, status+end_idle, 0);

    real_p = (Parallelism < sum_cluster_active) ? Parallelism : sum_cluster_active;

#if defined(TX2)
    float idleP2 = 0;
    if(leader < 2){
      //idleP2 = 228 - (status[2] || status[3] || status[4] || status[5]) * (228-idleP);
      idleP2 = (76 + 152 + 76 * ~(a57_freq)) - (status[2] || status[3] || status[4] || status[5]) * ((76 + 152 + 76 * ~(a57_freq))-idleP);
    }else{
      //idleP2 = 228 - (status[0] || status[1]) * (228-idleP);
      idleP2 = (76 + 152 + 76 * ~(a57_freq)) - (status[0] || status[1]) * ((76 + 152 + 76 * ~(a57_freq))-idleP);
    }
#endif

    /********* ********* Idle Power ********* *********/
    int num_active = 0;
    for(int i = leader; i < leader+width; i++){
      if(status[i] == 0){
        num_active++;
      }
    }
#if defined(TX2)
    idleP = idleP2 * width / (sum_cluster_active + num_active);
#endif

#if defined(Haswell)
    //idleP = idleP * width / (sum_cluster_active + num_active);
    idleP = power[leader/COREPERSOCKET][0] * width / (sum_cluster_active + num_active);
#endif 

    /********* ********* Dynamic Power ********* *********/
#if defined(TX2)
    // int real_core_use = (((real_p + num_active) * width) < (end_idle - start_idle)) ? ((real_p + num_active) * width) : (end_idle - start_idle);
    // int real_use_bywidth = real_core_use / width;
    // dyna_P = it->get_power(leader, real_core_use, real_use_bywidth); 
#ifdef DVFS
        // MM tasks
        if(it->tasktype == 0){
          if(leader < 2){
            dyna_P = MM_power[denver_freq][0][width];
          }
          else{
            dyna_P = MM_power[a57_freq][1][width];
          }
        }else{
          if(it->tasktype == 1){
            if(leader < 2){
              dyna_P = CP_power[denver_freq][0][width];
            }
            else{
              dyna_P = CP_power[a57_freq][1][width];
            }
          }
        }         
        
#else
        if(it->tasktype == 0){
          if(leader < 2){
            dyna_P = MM_power[0][width];
          }
          else{
            dyna_P = MM_power[1][width];
          }
        }
        else{
          if(it->tasktype == 1){
            if(leader < 2){
              dyna_P = CP_power[0][width];
            }
            else{
              dyna_P = CP_power[1][width];
            }
          }
        }
#endif
#endif

#if defined(Haswell)
    //dyna_P = it->Haswell_Dyna_power(leader, width);
    dyna_P = power[leader/COREPERSOCKET][width];
#endif
    /********* ********* Energy Prediction ********* *********/
    // Prediction is based on idle and dynamic power
    comp_perf = ptt_val * (idleP + dyna_P);
    if (comp_perf < shortest_exec) {
#ifdef DVFS
      new_freq = freq;
#endif
      shortest_exec = comp_perf;
      new_width = width;
      new_leader = leader;      
    }
  }
#ifdef DVFS
  }
#endif
  if(new_leader != -1) {
    //std::cout << "[Mold Width - Jing]\n";
    it->leader = new_leader;
    it->width  = new_width;
    it->updateflag = 1;
  }
#ifdef DVFS
  // Change frequency
  if(it->leader < 2){
    // first check if the frequency is the same? If so, go ahead; if not, need to change frequency!
    if(new_freq != denver_freq){
      std::cout << "[DVFS] Denver frequency changes from = " << denver_freq << " to " << new_freq << ".\n";
      // Write new frequency to Denver cluster
      if(new_freq == 0){
        system("echo 2035200 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed");
      }else{
        system("echo 345600 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed");
      }
      denver_freq = new_freq;
    }
  }else{
    // Write new frequency to A57 cluster
    if(new_freq != a57_freq){
      std::cout << "[DVFS] A57 frequency changes from = " << a57_freq << " to " << new_freq << ".\n";
      if(new_freq == 0){
        system("echo 2035200 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
      }else{
        system("echo 345600 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
      }
      a57_freq = new_freq;
    }
  }
#endif
  return new_leader;
} 


PolyTask * PolyTask::commit_and_wakeup(int _nthread){
  PolyTask *ret = nullptr;
  for(std::list<PolyTask *>::iterator it = out.begin(); it != out.end(); ++it){
    int refs = (*it)->refcount.fetch_sub(1);
    if(refs == 1){
      LOCK_ACQUIRE(worker_lock[_nthread]);
      worker_ready_q[_nthread].push_back(*it);
      LOCK_RELEASE(worker_lock[_nthread]);
//      int ndx = rand() % gotao_nthreads;
//      LOCK_ACQUIRE(worker_lock[ndx]);
//      worker_ready_q[ndx].push_back(*it);
//      LOCK_RELEASE(worker_lock[ndx]);
#ifdef DEBUG
      LOCK_ACQUIRE(output_lck);
      std::cout << "[DEBUG] Task " << (*it)->taskid << " became ready, insert into thread " << _nthread << " queue.\n";
      //std::cout << "[DEBUG] Task " << (*it)->taskid << " became ready, insert into thread " << ndx << " queue.\n";
      LOCK_RELEASE(output_lck);
#endif
//       if(!ret && (((*it)->affinity_queue == -1) || (((*it)->affinity_queue/(*it)->width) == (_nthread/(*it)->width)))){
//         ret = *it; // forward locally only if affinity matches
// #ifdef DEBUG
//         LOCK_ACQUIRE(output_lck);
//         std::cout << "[DEBUG] Task " << (*it)->taskid << " became ready, insert into thread " << _nthread << " queue.\n";
//         LOCK_RELEASE(output_lck);
// #endif
//       }
//       else{
//         int ndx = (*it)->affinity_queue;
//         if((ndx == -1) || (((*it)->affinity_queue/(*it)->width) == (_nthread/(*it)->width)))

//       } 
    }
  }
}       
