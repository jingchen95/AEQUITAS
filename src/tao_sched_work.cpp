#include "tao.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <vector>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <time.h>
#include <fstream>
#include <sstream>
#include <array>
#include "xitao_workspace.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
using namespace xitao;

  // void explain_message_errno_fork(char *message, int message_size, int errnum);

const int EAS_PTT_TRAIN = 2;

int *required_freq;
int *current_freq;
int *active_state;
bool *end_flag;
std::vector<std::vector<int>> stealings(XITAO_MAXTHREADS, std::vector<int>(XITAO_MAXTHREADS, -1));

// std::array<std::atomic<int>, 2> RSM{{ {1}, {0} }};
// std::array<std::atomic<int>, 2> state{{ {0}, {0} }};
int a57_freq;
int denver_freq;

#if (defined TX2) && (defined DVFS)
// 0 denotes highest frequency, 1 denotes lowest. 
// e.g. in 00, first 0 is denver, second 0 is a57. env= 0*2+0 = 0
int env;
#endif

int status[XITAO_MAXTHREADS];
int status_working[XITAO_MAXTHREADS];
int Sched, Parallelism;
int maySteal_DtoA, maySteal_AtoD;
std::atomic<int> DtoA(0);

// define the topology
int gotao_sys_topo[5] = TOPOLOGY;

#ifdef NUMTASKS
int num_task[XITAO_MAXTHREADS] = {0};
#endif
#ifdef DVFS
float MM_power[NUMSOCKETS][FREQLEVELS][XITAO_MAXTHREADS] = {0.0};
float CP_power[NUMSOCKETS][FREQLEVELS][XITAO_MAXTHREADS] = {0.0};
int PTT_UpdateFlag[FREQLEVELS][XITAO_MAXTHREADS][XITAO_MAXTHREADS] = {0};
#else
float MM_power[NUMSOCKETS][XITAO_MAXTHREADS] = {0.0};
float CP_power[NUMSOCKETS][XITAO_MAXTHREADS] = {0.0};
int PTT_UpdateFlag[XITAO_MAXTHREADS][XITAO_MAXTHREADS] = {0};
#endif


struct timespec tim, tim2;
cpu_set_t affinity_setup;
int TABLEWIDTH;
int worker_loop(int);

#ifdef NUMTASKS_MIX
//std::vector<int> num_task(XITAO_MAXTHREADS * XITAO_MAXTHREADS, 0);
int num_task[XITAO_MAXTHREADS][XITAO_MAXTHREADS] = {0}; // First Dimension: kernel index; second dimension: thread id; width = 1
#endif

int PTT_flag[XITAO_MAXTHREADS][XITAO_MAXTHREADS];
std::chrono::time_point<std::chrono::system_clock> t3;
std::mutex m;
std::condition_variable cv;
bool finish = false;

// std::vector<thread_info> thread_info_vector(XITAO_MAXTHREADS);
//! Allocates/deallocates the XiTAO's runtime resources. The size of the vector is equal to the number of available CPU cores. 
/*!
  \param affinity_control Set the usage per each cpu entry in the cpu_set_t
 */
int set_xitao_mask(cpu_set_t& user_affinity_setup) {
  if(!gotao_initialized) {
    resources_runtime_conrolled = true;                                    // make this true, to refrain from using XITAO_MAXTHREADS anywhere
    int cpu_count = CPU_COUNT(&user_affinity_setup);
    runtime_resource_mapper.resize(cpu_count);
    int j = 0;
    for(int i = 0; i < XITAO_MAXTHREADS; ++i) {
      if(CPU_ISSET(i, &user_affinity_setup)) {
        runtime_resource_mapper[j++] = i;
      }
    }
    if(cpu_count < gotao_nthreads) std::cout << "Warning: only " << cpu_count << " physical cores available, whereas " << gotao_nthreads << " are requested!" << std::endl;      
  } else {
    std::cout << "Warning: unable to set XiTAO affinity. Runtime is already initialized. This call will be ignored" << std::endl;      
  }  
}

void gotao_wait() {
//  gotao_master_waiting = true;
//  master_thread_waiting.notify_one();
//  std::unique_lock<std::mutex> lk(pending_tasks_mutex);
//  while(gotao_pending_tasks) pending_tasks_cond.wait(lk);
////  gotao_master_waiting = false;
//  master_thread_waiting.notify_all();
  while(PolyTask::pending_tasks > 0);
}
//! Initialize the XiTAO Runtime
/*!
  \param nthr is the number of XiTAO threads 
  \param thrb is the logical thread id offset from the physical core mapping
  \param nhwc is the number of hardware contexts
*/ 
int gotao_init_hw( int nthr, int thrb, int nhwc)
{
  gotao_initialized = true;
 	if(nthr>=0) gotao_nthreads = nthr;
  else {
    if(getenv("GOTAO_NTHREADS")) gotao_nthreads = atoi(getenv("GOTAO_NTHREADS"));
    else gotao_nthreads = XITAO_MAXTHREADS;
  }
  if(gotao_nthreads > XITAO_MAXTHREADS) {
    std::cout << "Fatal error: gotao_nthreads is greater than XITAO_MAXTHREADS of " << XITAO_MAXTHREADS << ". Make sure XITAO_MAXTHREADS environment variable is set properly" << std::endl;
    exit(0);
  }
  
  // Read Power Profile File, including idle and dynamic power
  std::ifstream infile, infile1;

#if (defined DVFS) && (defined TX2)
// Needs to find out the task type ??????
  infile.open("/home/nvidia/Work_1/ERASE_TACO/PowerProfile/TX2_DVFS_MM");
  if(infile.fail()){
    std::cout << "Failed to open power profile file!" << std::endl;
    std::cin.get();
    return 0;
  }
  std::string token;
  while(std::getline(infile, token)) {
    std::istringstream line(token);
    int ii = 0;
    int pwidth = 0;
    int freq = 0;
    float get_power = 0;
    while(line >> token) {
      if(ii == 0){
        freq = stoi(token);
      }else{
        if(ii == 1){
          pwidth = stoi(token);
        }
        else{
          get_power = stof(token);
          MM_power[freq][ii-2][pwidth] = get_power; 
        } 
      }
      ii++;
      //std::cout << "Token :" << token << std::endl;
    }
    // if(infile.unget().get() == '\n') {
    //   std::cout << "newline found" << std::endl;
    // }
  }

  // Output the power reading
  // for(int kk = 0; kk < FREQLEVELS; kk++){
  //   for(int ii = 0; ii < NUMSOCKETS; ii++){
  //     for (int jj = 0; jj < XITAO_MAXTHREADS; jj++){
  //       std::cout << MM_power[kk][ii][jj] << "\t";
  //     }
  //     std::cout << "\n";
  //   }
  // }

  infile1.open("/home/nvidia/Work_1/ERASE_TACO/PowerProfile/TX2_DVFS_CP");
  if(infile1.fail()){
    std::cout << "Failed to open power profile file!" << std::endl;
    std::cin.get();
    return 0;
  }
  //std::string token;
  while(std::getline(infile1, token)) {
    std::istringstream line(token);
    int ii = 0;
    int pwidth = 0;
    int freq = 0;
    float get_power = 0;
    while(line >> token) {
      if(ii == 0){
        freq = stoi(token);
      }else{
        if(ii == 1){
          pwidth = stoi(token);
        }
        else{
          get_power = stof(token);
          CP_power[freq][ii-2][pwidth] = get_power; 
        } 
      }
      ii++;
      //std::cout << "Token :" << token << std::endl;
    }
    // if(infile.unget().get() == '\n') {
    //   std::cout << "newline found" << std::endl;
    // }
  }

  // Output the power reading
  // for(int kk = 0; kk < FREQLEVELS; kk++){
  //   for(int ii = 0; ii < NUMSOCKETS; ii++){
  //     for (int jj = 0; jj < XITAO_MAXTHREADS; jj++){
  //       std::cout << CP_power[kk][ii][jj] << "\t";
  //     }
  //     std::cout << "\n";
  //   }
  // }

#else
#ifdef CATA
  // Denver Frequency: 2035200, A57 Frequency: 1113600
  infile.open("/home/nvidia/Work_1/ERASE_TACO/PowerProfile/COMP_CATA.txt");
  std::string token;
  while(std::getline(infile, token)) {
    std::istringstream line(token);
    int ii = 0;
    int pwidth = 0;
    float get_power = 0;
    while(line >> token) {
      if(ii == 0){
        pwidth = stoi(token);
      }else{
        get_power = stof(token);
        MM_power[ii-1][pwidth] = get_power; 
      }
      ii++;
    }
  }
  for(int ii = 0; ii < NUMSOCKETS; ii++){
    for (int jj = 0; jj < XITAO_MAXTHREADS; jj++){
      std::cout << MM_power[ii][jj] << "\t";
    }
    std::cout << "\n";
  }
#else  
// #ifdef TX2
  if(denver_freq == 0 && a57_freq == 0){
    infile.open("/home/nvidia/Work_1/ERASE_TACO/PowerProfile/TX2_MAXMAX_MatMul.txt");
    infile1.open("/home/nvidia/Work_1/ERASE_TACO/PowerProfile/TX2_MAXMAX_Copy.txt");
  }
  if(denver_freq == 1 && a57_freq == 0){
    infile.open("/home/nvidia/Work_1/ERASE_TACO/PowerProfile/TX2_MINMAX_MatMul.txt");
    infile1.open("/home/nvidia/Work_1/ERASE_TACO/PowerProfile/TX2_MINMAX_Copy.txt");
  }
  if(denver_freq == 0 && a57_freq == 1){
    infile.open("/home/nvidia/Work_1/ERASE_TACO/PowerProfile/TX2_MAXMIN_MatMul.txt");
    infile1.open("/home/nvidia/Work_1/ERASE_TACO/PowerProfile/TX2_MAXMIN_Copy.txt");
  }
  if(denver_freq == 1 && a57_freq == 1){
    infile.open("/home/nvidia/Work_1/ERASE_TACO/PowerProfile/TX2_MINMIN_MatMul.txt");
    infile1.open("/home/nvidia/Work_1/ERASE_TACO/PowerProfile/TX2_MINMIN_Copy.txt");
  }
  if(infile.fail() || infile1.fail()){
    std::cout << "Failed to open power profile file!" << std::endl;
    std::cin.get();
    return 0;
  }
  std::string token;
  while(std::getline(infile, token)) {
    std::istringstream line(token);
    int ii = 0;
    int pwidth = 0;
    float get_power = 0;
    while(line >> token) {
      if(ii == 0){
        pwidth = stoi(token);
      }else{
        get_power = stof(token);
        MM_power[ii-1][pwidth] = get_power; 
      }
      ii++;
    }
  }
  //if(Sched == 1){
    // Output the power reading
    for(int ii = 0; ii < NUMSOCKETS; ii++){
      for (int jj = 0; jj < XITAO_MAXTHREADS; jj++){
        std::cout << MM_power[ii][jj] << "\t";
      }
      std::cout << "\n";
    }
  //}

  while(std::getline(infile1, token)) {
    std::istringstream line(token);
    int ii = 0;
    int pwidth = 0;
    float get_power = 0;
    while(line >> token) {
      if(ii == 0){
        pwidth = stoi(token);
      }else{
        get_power = stof(token);
        CP_power[ii-1][pwidth] = get_power; 
      }
      ii++;
    }
  }
  //if(Sched == 1){
    // Output the power reading
    for(int ii = 0; ii < NUMSOCKETS; ii++){
      for (int jj = 0; jj < XITAO_MAXTHREADS; jj++){
        std::cout << CP_power[ii][jj] << "\t";
      }
      std::cout << "\n";
    }
  //}
#endif
#endif

  const char* layout_file = getenv("XITAO_LAYOUT_PATH");
  if(!resources_runtime_conrolled) {
    if(layout_file) {
      int line_count = 0;
      int cluster_count = 0;
      std::string line;      
      std::ifstream myfile(layout_file);
      int current_thread_id = -1; // exclude the first iteration
      if (myfile.is_open()) {
        bool init_affinity = false;
        while (std::getline(myfile,line)) {         
          size_t pos = 0;
          std::string token;
          if(current_thread_id >= XITAO_MAXTHREADS) {
            std::cout << "Fatal error: there are more partitions than XITAO_MAXTHREADS of: " << XITAO_MAXTHREADS  << " in file: " << layout_file << std::endl;    
            exit(0);    
          }

          // if(line_count == 0){
          //   while ((pos = line.find(";")) != std::string::npos) {
          //     token = line.substr(0, pos);      
          //     int val = stoi(token);
          //     cluster_mapper[cluster_count] = val;
          //     cluster_count++;
          //     line.erase(0, pos + 1);
          //   }
          //   line_count++;
          //   continue;
          // }
          
          //if(line_count > 0){
          int thread_count = 0;
          while ((pos = line.find(",")) != std::string::npos) {
            token = line.substr(0, pos);      
            int val = stoi(token);
            if(!init_affinity) static_resource_mapper[thread_count++] = val;  
            else { 
              if(current_thread_id + 1 >= gotao_nthreads) {
                  std::cout << "Fatal error: more configurations than there are input threads in:" << layout_file << std::endl;    
                  exit(0);
              }
              ptt_layout[current_thread_id].push_back(val);
              for(int i = 0; i < val; ++i) {     
                if(current_thread_id + i >= XITAO_MAXTHREADS) {
                  std::cout << "Fatal error: illegal partition choices for thread: " << current_thread_id <<" spanning id: " << current_thread_id + i << " while having XITAO_MAXTHREADS: " << XITAO_MAXTHREADS  << " in file: " << layout_file << std::endl;    
                  exit(0);           
                }
                inclusive_partitions[current_thread_id + i].push_back(std::make_pair(current_thread_id, val)); 
              }              
            }            
            line.erase(0, pos + 1);
          }          
          //if(line_count > 1) {
            token = line.substr(0, line.size());      
            int val = stoi(token);
            if(!init_affinity) static_resource_mapper[thread_count++] = val;
            else { 
              ptt_layout[current_thread_id].push_back(val);
              for(int i = 0; i < val; ++i) {                
                if(current_thread_id + i >= XITAO_MAXTHREADS) {
                  std::cout << "Fatal error: illegal partition choices for thread: " << current_thread_id <<" spanning id: " << current_thread_id + i << " while having XITAO_MAXTHREADS: " << XITAO_MAXTHREADS  << " in file: " << layout_file << std::endl;    
                  exit(0);           
                }
                inclusive_partitions[current_thread_id + i].push_back(std::make_pair(current_thread_id, val)); 
              }              
            }            
          //}
          if(!init_affinity) { 
            gotao_nthreads = thread_count; 
            init_affinity = true;
          }
          current_thread_id++;    
          line_count++;     
          //}
        }
        myfile.close();
      } else {
        std::cout << "Fatal error: could not open hardware layout path " << layout_file << std::endl;    
        exit(0);
      }
    } else {
        std::cout << "Warning: XITAO_LAYOUT_PATH is not set. Default values for affinity and symmetric resoruce partitions will be used" << std::endl;    
        for(int i = 0; i < XITAO_MAXTHREADS; ++i) 
          static_resource_mapper[i] = i; 
        std::vector<int> widths;             
        int count = gotao_nthreads;        
        std::vector<int> temp;        // hold the big divisors, so that the final list of widths is in sorted order 
        for(int i = 1; i < sqrt(gotao_nthreads); ++i){ 
          if(gotao_nthreads % i == 0) {
            widths.push_back(i);
            temp.push_back(gotao_nthreads / i); 
          } 
        }
        std::reverse(temp.begin(), temp.end());
        widths.insert(widths.end(), temp.begin(), temp.end());
        //std::reverse(widths.begin(), widths.end());        
        for(int i = 0; i < widths.size(); ++i) {
          for(int j = 0; j < gotao_nthreads; j+=widths[i]){
            ptt_layout[j].push_back(widths[i]);
          }
        }

        for(int i = 0; i < gotao_nthreads; ++i){
          for(auto&& width : ptt_layout[i]){
            for(int j = 0; j < width; ++j) {                
              inclusive_partitions[i + j].push_back(std::make_pair(i, width)); 
            }         
          }
        }
      } 
  } else {    
    if(gotao_nthreads != runtime_resource_mapper.size()) {
      std::cout << "Warning: requested " << runtime_resource_mapper.size() << " at runtime, whereas gotao_nthreads is set to " << gotao_nthreads <<". Runtime value will be used" << std::endl;
      gotao_nthreads = runtime_resource_mapper.size();
    }            
  }
#ifdef DEBUG
	std::cout << "XiTAO initialized with " << gotao_nthreads << " threads and configured with " << XITAO_MAXTHREADS << " max threads " << std::endl;
  // std::cout << "The platform has " << cluster_mapper.size() << " clusters.\n";
  // for(int i = 0; i < cluster_mapper.size(); i++){
  //   std::cout << "[DEBUG] Cluster " << i << " has " << cluster_mapper[i] << " cores.\n";
  // }
  for(int i = 0; i < static_resource_mapper.size(); ++i) {
    std::cout << "[DEBUG] Thread " << i << " is configured to be mapped to core id : " << static_resource_mapper[i] << std::endl;
    std::cout << "[DEBUG] PTT Layout Size of thread " << i << " : " << ptt_layout[i].size() << std::endl;
    std::cout << "[DEBUG] Inclusive partition size of thread " << i << " : " << inclusive_partitions[i].size() << std::endl;
    std::cout << "[DEBUG] leader thread " << i << " has partition widths of : ";
    for (int j = 0; j < ptt_layout[i].size(); ++j){
      std::cout << ptt_layout[i][j] << " ";
    }
    std::cout << std::endl;
    std::cout << "[DEBUG] thread " << i << " is contained in these [leader,width] pairs : ";
    for (int j = 0; j < inclusive_partitions[i].size(); ++j){
      std::cout << "["<<inclusive_partitions[i][j].first << "," << inclusive_partitions[i][j].second << "]";
    }
    std::cout << std::endl;
  }
#endif

  if(nhwc>=0){
    gotao_ncontexts = nhwc;
  }
  else{
    if(getenv("GOTAO_HW_CONTEXTS")){
      gotao_ncontexts = atoi(getenv("GOTAO_HW_CONTEXTS"));
    }
    else{ 
      gotao_ncontexts = GOTAO_HW_CONTEXTS;
    }
  }
  if(thrb>=0){
    gotao_thread_base = thrb;
  }
  else{
    if(getenv("GOTAO_THREAD_BASE")){
      gotao_thread_base = atoi(getenv("GOTAO_THREAD_BASE"));
    }
    else{
      gotao_thread_base = GOTAO_THREAD_BASE;
    }
  }
}

// Initialize gotao from environment vars or defaults
int gotao_init(int scheduler, int parallelism, int STEAL_DtoA, int STEAL_AtoD)
{
//  return gotao_init_hw(-1, -1, -1);
  starting_barrier = new BARRIER(gotao_nthreads);
  tao_barrier = new cxx_barrier(gotao_nthreads);

  // Shared memory delaration between parent and child process
  /* allocate and attach shared memory */
  int shmID = shmget(IPC_PRIVATE, XITAO_MAXTHREADS*sizeof(int), IPC_CREAT | 0666);
  if (shmID < 0) {
    printf("shmget error\n");
    return 0;
  }
  required_freq = (int *)shmat(shmID, NULL, 0);
  int shmID2 = shmget(IPC_PRIVATE, XITAO_MAXTHREADS*sizeof(int), IPC_CREAT | 0666);
  if (shmID2 < 0) {
    printf("shmget error\n");
    return 0;
  }
  current_freq = (int *)shmat(shmID2, NULL, 0);
  int shmID3 = shmget(IPC_PRIVATE, XITAO_MAXTHREADS*sizeof(int), IPC_CREAT | 0666);
  if (shmID3 < 0) {
    printf("shmget error\n");
    return 0;
  }
  active_state = (int *)shmat(shmID3, NULL, 0);
  int shmID4 = shmget(IPC_PRIVATE, sizeof(bool), IPC_CREAT | 0666);
  if (shmID4 < 0) {
    printf("shmget error\n");
    return 0;
  }
  end_flag = (bool *)shmat(shmID4, NULL, 0);
#ifdef DEBUG
  LOCK_ACQUIRE(output_lck);
  std::cout << "[DEBUG] gotao_init: allocate and attach shared memory!" << std::endl;
  LOCK_RELEASE(output_lck);
#endif  
  pid_t ppid_before_fork = getpid();
  /* Fork Child Process */
  pid_t pid;
  pid = fork();
  std::cout << "pid = " << pid << std::endl;
  if(pid < 0) {
    std::cout << "failed to create child\n";
    return 0;
		// int err = errno;
    // char message[3000];
    // explain_message_errno_fork(message, sizeof(message), err, );
    // fprintf(stderr, "%s\n", message);
    // exit(EXIT_FAILURE);

    // fprintf(stderr, "%s\n", explain_fork());
    // exit(EXIT_FAILURE);
  }
  if (pid == 0){ /* child */
    std::cerr << "[Child] Fork Child Process!" << std::endl;
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(1, &cpu_mask);
    sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpu_mask); 
    // int shmID = shmget(IPC_PRIVATE, XITAO_MAXTHREADS*sizeof(int), 0666);
    // if (shmID < 0) {
    //   std::cerr << "shmget error. "<< std::endl;
    //   return 0;
    // }
    // int *required_freq = (int *)shmat(shmID, NULL, 0);
    // int shmID2 = shmget(IPC_PRIVATE, XITAO_MAXTHREADS*sizeof(int), 0666);
    // if (shmID2 < 0) {
    //   std::cerr << "shmget error. "<< std::endl;
    //   return 0;
    // }
    // int *current_freq = (int *)shmat(shmID2, NULL, 0);
    // int shmID3 = shmget(IPC_PRIVATE, XITAO_MAXTHREADS*sizeof(int), 0666);
    // if (shmID3 < 0) {
    //   std::cerr << "shmget error. "<< std::endl;
    //   return 0;
    // }
    // int *active_state = (int *)shmat(shmID3, NULL, 0);
    // int shmID4 = shmget(IPC_PRIVATE, sizeof(bool), 0666);
    // if (shmID4 < 0) {
    //   printf("shmget error\n");
    //   return 0;
    // }
    // bool *end_flag = (bool *)shmat(shmID4, NULL, 0);
    // std::cerr << "[Child] gotao_init: allocate and attach shared memory! " << std::endl;

    int pd_dominate[NUMSOCKETS] = {0,2};
    
    std::ofstream Denver("/sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed");
    std::ofstream ARM("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
    if (!Denver.is_open() || !ARM.is_open()){
      std::cerr << "[Child] Somthing failed while opening the file! " << std::endl;
      return 0;
    }
    // else{
    //   std::cerr << "[Child] Open Frequency files successfully! " << std::endl;
    // }
#ifdef TEST
    int i = 0;
    while(i < 10){
      usleep(250000);
      // sleep(1);
      std::cerr << "[Child] i = " << i << std::endl;
      if(i % 2 == 0){
        std::string data1("345600");
        Denver << data1 << std::endl;
        std::cerr << "[Child] Write 345600 to Denver cluster frequency file. " << std::endl;
      }else{
        std::string data1("2035200");
        Denver << data1 << std::endl;
        std::cerr << "[Child] Write 2035200 to Denver cluster frequency file. " << std::endl;
      }
      i++;
    }
    Denver.close();
    ARM.close();
    return 0;
#endif
    //while(!end_flag[0]){
    // while(! *end_flag){
    while(getppid() == ppid_before_fork){
      int i = 0;
      do{ // Move Denver's dominate 
        pd_dominate[0] = (pd_dominate[0] + 1 < 2)? pd_dominate[0] + 1: 0;
        i++;
      }while(active_state[pd_dominate[0]] == 0 && i <= 2);
      if(i > 2){ // Two Denver cores are both inactive
        Denver << std::to_string(345600) << std::endl;
#ifdef DEBUG
        LOCK_ACQUIRE(output_lck);
        std::cerr << "[Child] Write 345600 to Denver cluster, since 2 Denver cores are both inactive. " << std::endl;
        LOCK_RELEASE(output_lck);
#endif
        for(int j = 0; j < 2; j++){
          current_freq[j] = 345600;
        }
      }
      else{
        if(active_state[pd_dominate[0]] == 1){
#ifdef DEBUG
          LOCK_ACQUIRE(output_lck);
          std::cerr << "[Child] pd_dominate core of Denver cluster passes to " << pd_dominate[0] << std::endl;
          LOCK_RELEASE(output_lck);
#endif
          if(current_freq[pd_dominate[0]] != required_freq[pd_dominate[0]]){
            Denver << std::to_string(required_freq[pd_dominate[0]]) << std::endl;
            current_freq[pd_dominate[0]] = required_freq[pd_dominate[0]];
#ifdef DEBUG
            LOCK_ACQUIRE(output_lck);
            std::cerr << "[Child] Core " << pd_dominate[0] << " slicing time! Change Denver frequency to " << current_freq[pd_dominate[0]] << std::endl;
            LOCK_RELEASE(output_lck);
#endif            
          }
          else{
#ifdef DEBUG
            LOCK_ACQUIRE(output_lck);
            std::cerr << "[Child] Core " << pd_dominate[0] << " slicing time! current frequnecy = required frequency, no need to change." << std::endl;
            LOCK_RELEASE(output_lck);
#endif     
          }
        }
      }
      i = 0;
      do{// Move A57's dominate
        pd_dominate[1] = (pd_dominate[1] + 1 < 6)? pd_dominate[1] + 1: 2;
        i++;
      }while(active_state[pd_dominate[1]] == 0 && i <= 4);
      if(i > 4){
        // Four A57 cores are both inactive
        ARM << std::to_string(345600) << std::endl;
#ifdef DEBUG
        LOCK_ACQUIRE(output_lck);
        std::cerr << "[Child] Write 345600 to A57 cluster, since 4 A57 cores are both inactive. " << std::endl;
        LOCK_RELEASE(output_lck);
#endif
        for(int j = 2; j < 6; j++){
          current_freq[j] = 345600;
        }
      }else{
        if(active_state[pd_dominate[1]] == 1){
#ifdef DEBUG
          LOCK_ACQUIRE(output_lck);
          std::cerr << "[Child] pd_dominate core of A57 cluster passes to " << pd_dominate[1] << std::endl;
          LOCK_RELEASE(output_lck);
#endif
          if(current_freq[pd_dominate[1]] != required_freq[pd_dominate[1]]){
            ARM << std::to_string(required_freq[pd_dominate[1]]) << std::endl;
            current_freq[pd_dominate[1]] = required_freq[pd_dominate[1]];
#ifdef DEBUG
            LOCK_ACQUIRE(output_lck);
            std::cerr << "[Child] Core " << pd_dominate[1] << " slicing time! Change A57 frequency to " << current_freq[pd_dominate[1]] << std::endl;
            LOCK_RELEASE(output_lck);
#endif
          }
          else{
#ifdef DEBUG
            LOCK_ACQUIRE(output_lck);
            std::cerr << "[Child] Core " << pd_dominate[1] << " slicing time! current frequnecy = required frequency, no need to change." << std::endl;
            LOCK_RELEASE(output_lck);
#endif
          }
        }
      }
      usleep(1000000);
      //sleep(1);
    }
    // std::cerr << "[Child] end_flag = " << *end_flag << std::endl;
    Denver.close();
    ARM.close();
    exit(1);
    // return 0;
  } 
  else{
    /* Initialize shared memory and parallel threads */
    //end_flag[0] = false;
    *end_flag = false;
    for(int i = 0; i < gotao_nthreads; i++){
      current_freq[i] = 2035200;
      required_freq[i] = 2035200;
      active_state[i] = 1;
      t[i]  = new std::thread(worker_loop, i);
    }
    Sched = scheduler;
    Parallelism = parallelism;
    maySteal_DtoA = STEAL_DtoA;
    maySteal_AtoD = STEAL_AtoD;
  }
}

int gotao_start()
{
  starting_barrier->wait(gotao_nthreads+1);
}

int gotao_fini(){
  resources_runtime_conrolled = false;
  gotao_can_exit = true;
  gotao_initialized = false;
  for(int i = 0; i < gotao_nthreads; i++){
    t[i]->join();
  }
  if (shmdt(required_freq) == -1) {
    perror("main: shmdt: required_freq \n");
  }
  if (shmdt(current_freq) == -1) {
    perror("main: shmdt: current_freq \n");
  }
  if (shmdt(active_state) == -1) {
    perror("main: shmdt: active_state \n");
  }
}

void gotao_barrier(){
  tao_barrier->wait();
}

int check_and_get_available_queue(int queue) {
  bool found = false;
  if(queue >= runtime_resource_mapper.size()) {
    return rand()%runtime_resource_mapper.size();
  } else {
    return queue;
  }  
}
// push work into polytask queue
// if no particular queue is specified then try to determine which is the local
// queue and insert it there. This has some overhead, so in general the
// programmer should specify some queue
int gotao_push(PolyTask *pt, int queue)
{
  if((queue == -1) && (pt->affinity_queue != -1)){
    queue = pt->affinity_queue;
  }
  else{
    if(queue == -1){
      queue = sched_getcpu();
    }
  }
  if(resources_runtime_conrolled) queue = check_and_get_available_queue(queue);
  LOCK_ACQUIRE(worker_lock[queue]);
  worker_ready_q[queue].push_front(pt);
  // critical_ready_q[queue].push_front(pt);
  LOCK_RELEASE(worker_lock[queue]);
}

// Push work when not yet running. This version does not require locks
// Semantics are slightly different here
// 1. the tid refers to the logical core, before adjusting with gotao_thread_base
// 2. if the queue is not specified, then put everything into the first queue
int gotao_push_init(PolyTask *pt, int queue)
{
  if((queue == -1) && (pt->affinity_queue != -1)){
    queue = pt->affinity_queue;
  }
  else{
    if(queue == -1){
      queue = gotao_thread_base;
    }
  }
  if(resources_runtime_conrolled) queue = check_and_get_available_queue(queue);
  worker_ready_q[queue].push_front(pt);
  //critical_ready_q[queue].push_front(pt);
}

// alternative version that pushes to the back
int gotao_push_back_init(PolyTask *pt, int queue)
{
  if((queue == -1) && (pt->affinity_queue != -1)){
    queue = pt->affinity_queue;
  }
  else{
    if(queue == -1){
      queue = gotao_thread_base;
    }
  }
  worker_ready_q[queue].push_back(pt);
}

long int r_rand(long int *s)
{
  *s = ((1140671485*(*s) + 12820163) % (1<<24));
  return *s;
}

void __xitao_lock()
{
  smpd_region_lock.lock();
  //LOCK_ACQUIRE(smpd_region_lock);
}
void __xitao_unlock()
{
  smpd_region_lock.unlock();
  //LOCK_RELEASE(smpd_region_lock);
}

int worker_loop(int nthread){
  int phys_core;
  if(resources_runtime_conrolled) {
    if(nthread >= runtime_resource_mapper.size()) {
      LOCK_ACQUIRE(output_lck);
      std::cout << "Error: thread cannot be created due to resource limitation" << std::endl;
      LOCK_RELEASE(output_lck);
      exit(0);
    }
    phys_core = runtime_resource_mapper[nthread];
  } else {
    phys_core = static_resource_mapper[gotao_thread_base+(nthread%(XITAO_MAXTHREADS-gotao_thread_base))];   
  }
#ifdef DEBUG
  LOCK_ACQUIRE(output_lck);
  std::cout << "[DEBUG] nthread: " << nthread << " mapped to physical core: "<< phys_core << std::endl;
  LOCK_RELEASE(output_lck);
#endif  
  unsigned int seed = time(NULL);
  cpu_set_t cpu_mask;
  CPU_ZERO(&cpu_mask);
  CPU_SET(phys_core, &cpu_mask);

  const pid_t pid = getpid();
  sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_mask); 
  // When resources are reclaimed, this will preempt the thread if it has no work in its local queue to do.

  PolyTask *st = nullptr;
  starting_barrier->wait(gotao_nthreads+1);  
  auto&&  partitions = inclusive_partitions[nthread];

  int idle_try = 0;
  int idle_times = 0;
  int SleepNum = 0;
  int AccumTime = 0;

  if(Sched == 1){
    for(int i=0; i<XITAO_MAXTHREADS; i++){ 
      status[i] = 1;
      status_working[i] = 1;
      //for(int j=0; j < XITAO_MAXTHREADS; j++){
      //  PTT_flag[i][j] = 1;
      //}
    }
  }
  bool stop = false;
  // int queue_size = 0;
 
  // stealings.clear();
  // Accumulation of tasks execution time
  // Goal: Get runtime idle time
#ifdef EXECTIME
  //std::chrono::time_point<std::chrono::system_clock> idle_start, idle_end;
  std::chrono::duration<double> elapsed_exe;
#endif

#ifdef OVERHEAD_PTT
  std::chrono::duration<double> elapsed_ptt;
#endif

  while(true)
  {    
    int random_core = 0;
    AssemblyTask *assembly = nullptr;
    SimpleTask *simple = nullptr;

  // 0. If a task is already provided via forwarding then exeucute it (simple task)
  //    or insert it into the assembly queues (assembly task)
    if(st){
      if(st->type == TASK_SIMPLE){
        SimpleTask *simple = (SimpleTask *) st;
        simple->f(simple->args, nthread);
        st = simple->commit_and_wakeup(nthread);
        simple->cleanup();
        //delete simple;
      }
      else 
        if(st->type == TASK_ASSEMBLY){
          AssemblyTask *assembly = (AssemblyTask *) st;
	        assembly->leader = nthread;
  #ifdef DEBUG
          LOCK_ACQUIRE(output_lck);
          std::cout << "[DEBUG] Distributing task " << assembly->taskid << " with width " << assembly->width << " to workers [" << assembly->leader << "," << assembly->leader + assembly->width << ")" << std::endl;
          LOCK_RELEASE(output_lck);
  #endif
          LOCK_ACQUIRE(worker_assembly_lock[nthread]);
          worker_assembly_q[nthread].push_back(st);
#ifdef NUMTASKS
	        // num_task[assembly->width * gotao_nthreads + nthread]++;
          num_task[nthread]++;
#endif
#ifdef NUMTASKS_MIX
	        num_task[assembly->kernel_index][nthread]++;
#endif
          LOCK_RELEASE(worker_assembly_lock[nthread]);
          //queue_size += 1;
  // #ifdef DEBUG
  //         LOCK_ACQUIRE(output_lck);
  //         std::cout << "[DEBUG] After pushing in task " <<  assembly->taskid << ", thread " <<  nthread << "'s queue size becomes " << queue_size << ". Current_frequency[" << nthread << "] = " << current_freq[nthread] << std::endl;
  //         LOCK_RELEASE(output_lck);
  // #endif
          if(current_freq[nthread] < 2035200){ 
            if(worker_ready_q[nthread].size() >= 4*(float(Parallelism)/5.0)){ // For Parallelism = 8, five threshold levels 
              required_freq[nthread] = 2035200;
            }else{
              if(worker_ready_q[nthread].size() >= 3*(float(Parallelism)/5.0) && current_freq[nthread] < 1574400){
                required_freq[nthread] = 1574400;
              }else{
                if(worker_ready_q[nthread].size() >= 2*(float(Parallelism)/5.0) && current_freq[nthread] < 1113600){
                  required_freq[nthread] = 1113600;
                }else{
                  if(worker_ready_q[nthread].size() >= float(Parallelism)/5.0 && current_freq[nthread] < 652800){
                    required_freq[nthread] = 652800;
                  }
                }
              }
            }
            // if(worker_ready_q[nthread].size() >= 6.4){ // For Parallelism = 8, five threshold levels 
            //   required_freq[nthread] = 2035200;
            // }else{
            //   if(worker_ready_q[nthread].size() >= 4.8 && current_freq[nthread] < 1574400){
            //     required_freq[nthread] = 1574400;
            //   }else{
            //     if(worker_ready_q[nthread].size() >= 3.2 && current_freq[nthread] < 1113600){
            //       required_freq[nthread] = 1113600;
            //     }else{
            //       if(worker_ready_q[nthread].size() >= 1.6 && current_freq[nthread] < 652800){
            //         required_freq[nthread] = 652800;
            //       }
            //     }
            //   }
            // }
            // if(worker_ready_q[nthread].size() >= 4.8){ // For Parallelism = 6, five threshold levels 
            //   required_freq[nthread] = 2035200;
            // }else{
            //   if(worker_ready_q[nthread].size() >= 3.6 && current_freq[nthread] < 1574400){
            //     required_freq[nthread] = 1574400;
            //   }else{
            //     if(worker_ready_q[nthread].size() >= 2.4 && current_freq[nthread] < 1113600){
            //       required_freq[nthread] = 1113600;
            //     }else{
            //       if(worker_ready_q[nthread].size() >= 1.2 && current_freq[nthread] < 652800){
            //         required_freq[nthread] = 652800;
            //       }
            //     }
            //   }
            // }
            // if(worker_ready_q[nthread].size() >= 4){ // For Parallelism = 4, five threshold levels 
            //   required_freq[nthread] = 2035200;
            // }else{
            //   if(worker_ready_q[nthread].size() >= 3 && current_freq[nthread] < 1574400){
            //     required_freq[nthread] = 1574400;
            //   }else{
            //     if(worker_ready_q[nthread].size() >= 2 && current_freq[nthread] < 1113600){
            //       required_freq[nthread] = 1113600;
            //     }else{
            //       if(worker_ready_q[nthread].size() >= 1 && current_freq[nthread] < 652800){
            //         required_freq[nthread] = 652800;
            //       }
            //     }
            //   }
            // }
            // if(worker_ready_q[nthread].size() >= 25.6){ // For sparse LU, NB=32, five threshold levels 
            //   required_freq[nthread] = 2035200;
            // }else{
            //   if(worker_ready_q[nthread].size() >= 19.2 && current_freq[nthread] < 1574400){
            //     required_freq[nthread] = 1574400;
            //   }else{
            //     if(worker_ready_q[nthread].size() >= 12.8 && current_freq[nthread] < 1113600){
            //       required_freq[nthread] = 1113600;
            //     }else{
            //       if(worker_ready_q[nthread].size() >= 6.4 && current_freq[nthread] < 652800){
            //         required_freq[nthread] = 652800;
            //       }
            //     }
            //   }
            // }
            // if(worker_ready_q[nthread].size() >= 51.2){ // For sparse LU, NB = 64, five threshold levels 
            //   required_freq[nthread] = 2035200;
            // }else{
            //   if(worker_ready_q[nthread].size() >= 38.4 && current_freq[nthread] < 1574400){
            //     required_freq[nthread] = 1574400;
            //   }else{
            //     if(worker_ready_q[nthread].size() >= 25.6 && current_freq[nthread] < 1113600){
            //       required_freq[nthread] = 1113600;
            //     }else{
            //       if(worker_ready_q[nthread].size() >= 12.8 && current_freq[nthread] < 652800){
            //         required_freq[nthread] = 652800;
            //       }
            //     }
            //   }
            // }
#ifdef DEBUG
            LOCK_ACQUIRE(output_lck);
            std::cout << "[DEBUG] Thread " << nthread << " queue size: " << worker_ready_q[nthread].size() <<  " tasks. The required frequency = " << required_freq[nthread] << std::endl;
            LOCK_RELEASE(output_lck);
#endif
          }
          // if(queue_size[nthread] < 5){
          //   required_freq[nthread] = 345600;
          // }else{
          //   if(queue_size[nthread] < 10){
          //     required_freq[nthread] = 1113600;
          //   }else{
          //     required_freq[nthread] = 2035200;
          //   }
          // }
          st = nullptr;
        }
      continue;
    }

    // 1. check for assemblies
    if(!worker_assembly_q[nthread].pop_front(&st)){
      st = nullptr;
    }
    // assemblies are inlined between two barriers
    if(st) {
//      LOCK_ACQUIRE(worker_assembly_lock[nthread]);
      // queue_size -= 1;
//      LOCK_RELEASE(worker_assembly_lock[nthread]);    
#ifdef TEMPO
#ifdef DEBUG
      LOCK_ACQUIRE(output_lck);
      std::cout << "[DEBUG] After poping out task " <<  assembly->taskid << ", thread " << nthread << "'s queue size becomes " << queue_size << std::endl;
      LOCK_RELEASE(output_lck);
#endif
// #ifdef TEMPO
      if(queue_size < 5){
        required_freq[nthread] = 345600;
      }else{
        if(queue_size < 10){
          required_freq[nthread] = 1113600;
        }else{
          required_freq[nthread] = 2035200;
        }
      }
#ifdef DEBUG
      LOCK_ACQUIRE(output_lck);
      std::cout << "[DEBUG] Thread " << nthread << " requires changing frequency to " << required_freq[nthread] << std::endl;
      LOCK_RELEASE(output_lck);
#endif
      
#endif
      
      int _final; // remaining
      assembly = (AssemblyTask *) st;

#ifdef DEBUG
      LOCK_ACQUIRE(output_lck);
      std::cout << "[DEBUG] Thread "<< nthread << " starts executing task " << assembly->taskid << "......\n";
      LOCK_RELEASE(output_lck);
#endif
      std::chrono::time_point<std::chrono::system_clock> t1,t2;
      t1 = std::chrono::system_clock::now();
      assembly->execute(nthread);
      t2 = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsed_seconds = t2-t1;
#ifdef DEBUG
      LOCK_ACQUIRE(output_lck);
      std::cout << "[NOV 21] Task " << assembly->taskid << " execution time on thread " << nthread << " is: " << elapsed_seconds.count() << "\n";
      LOCK_RELEASE(output_lck);
#endif
#ifdef EXECTIME
        elapsed_exe += t2-t1;
#endif

    _final = (++assembly->threads_out_tao == assembly->width);

    st = nullptr;
    if(_final){ // the last exiting thread updates  
      task_completions[nthread].tasks++;
      if(task_completions[nthread].tasks > 0){
        PolyTask::pending_tasks -= task_completions[nthread].tasks;
#ifdef DEBUG
        LOCK_ACQUIRE(output_lck);
        std::cout << "[DEBUG] Thread " << nthread << " completed " << task_completions[nthread].tasks << " tasks. Pending tasks = " << PolyTask::pending_tasks << "\n";
        LOCK_RELEASE(output_lck);
#endif
        task_completions[nthread].tasks = 0;
        if(stealings[nthread][0] >=0){
        // if(stealings[nthread].begin() >=0){
          int freq_up_core = stealings[nthread][0];
#ifdef DEBUG
          LOCK_ACQUIRE(output_lck);
          std::cout << "[DEBUG] Thread " << nthread << " is victim, current_stealer_freq["<< freq_up_core << "] = " << current_freq[freq_up_core] << std::endl;
          LOCK_RELEASE(output_lck);
#endif
          if(current_freq[freq_up_core] < 2035200){
            if(current_freq[freq_up_core] == 345600){
              required_freq[freq_up_core] = 652800;
            }else{
              if(current_freq[freq_up_core] == 652800){
                required_freq[freq_up_core] = 1113600;

              }else{
                if(current_freq[freq_up_core] == 1113600){
                  required_freq[freq_up_core] = 1574400;
                }else{
                  if(current_freq[freq_up_core] == 1574400){
                    required_freq[freq_up_core] = 2035200;
                  }
                }
              }
            }
#ifdef DEBUG
            LOCK_ACQUIRE(output_lck);
            std::cout << "[DEBUG] Up the most immediate stealer " << freq_up_core << " frequency to " << required_freq[freq_up_core] << std::endl;
            LOCK_RELEASE(output_lck);
#endif
          }else{
#ifdef DEBUG
            LOCK_ACQUIRE(output_lck);
            std::cout << "[DEBUG] No need to increase frequency." << std::endl;
            LOCK_RELEASE(output_lck);
#endif
          }
          stealings[nthread].erase(stealings[nthread].begin());

          if(stealings[freq_up_core][0] >= 0){
          // if(stealings[freq_up_core].begin() >= 0){
            int freq_up_next_core = stealings[freq_up_core][0];
#ifdef DEBUG
            LOCK_ACQUIRE(output_lck);
            std::cout << "[DEBUG] Thread " << freq_up_core << " is victim, current_stealer_freq["<< freq_up_next_core << "] = " << current_freq[freq_up_next_core] << std::endl;
            LOCK_RELEASE(output_lck);
#endif
            if(current_freq[freq_up_next_core] < 2035200){
              if(current_freq[freq_up_next_core] == 345600){
                required_freq[freq_up_next_core] = 652800;
              }else{
                if(current_freq[freq_up_next_core] == 652800){
                  required_freq[freq_up_next_core] = 1113600;
                }else{
                  if(current_freq[freq_up_next_core] == 1113600){
                    required_freq[freq_up_next_core] = 1574400;
                  }else{
                    if(current_freq[freq_up_next_core] == 1574400){
                      required_freq[freq_up_next_core] = 2035200;
                    }
                  }
                }
              }
#ifdef DEBUG
              LOCK_ACQUIRE(output_lck);
              std::cout << "[DEBUG] Up the next most immediate stealer " << freq_up_next_core << " frequency to " << required_freq[freq_up_next_core] << std::endl;
              LOCK_RELEASE(output_lck);
#endif
            }else{
  #ifdef DEBUG
                LOCK_ACQUIRE(output_lck);
                std::cout << "[DEBUG] No need to increase frequency." << std::endl;
                LOCK_RELEASE(output_lck);
  #endif                
            }
            stealings[freq_up_core].erase(stealings[freq_up_core].begin());
          }
        }  
      }
      //st = 
      assembly->commit_and_wakeup(nthread);
      assembly->cleanup();
    }
    idle_try = 0;
    idle_times = 0;
    continue;
  }

    // 2. check own queue
    LOCK_ACQUIRE(worker_lock[nthread]);
    if(!worker_ready_q[nthread].empty()){
      st = worker_ready_q[nthread].front(); 
      worker_ready_q[nthread].pop_front();
      LOCK_RELEASE(worker_lock[nthread]);
      continue;
    }  
    LOCK_RELEASE(worker_lock[nthread]);       

//#ifdef WORK_STEALING
    // 3. try to steal rand_r(&seed)
    //if(rand() % STEAL_ATTEMPTS == 0){
    if(rand() % gotao_nthreads == 0){
      status_working[nthread] = 0;
      int attempts = gotao_nthreads;
      idle_try++;
      if(active_state[nthread] == 1 && idle_try > IDLE_SLEEP){
        active_state[nthread] = 0;
#ifdef DEBUG
        LOCK_ACQUIRE(output_lck);
        std::cout << "[DEBUG] Thread " << nthread << " steals for " << IDLE_SLEEP << " times. Active state sets to  " << active_state[nthread] << ". \n";
        LOCK_RELEASE(output_lck);          
#endif	
      }
      do{
				do{
          random_core = (rand_r(&seed) % gotao_nthreads);
        } while(random_core == nthread);

        LOCK_ACQUIRE(worker_lock[random_core]);
        if(!worker_ready_q[random_core].empty()){
          st = worker_ready_q[random_core].back();
          //if((st->criticality == 0 && Sched == 0) || (Sched == 3)){
            worker_ready_q[random_core].pop_back();
            st->leader = nthread;
            tao_total_steals++;
#ifdef DEBUG
            LOCK_ACQUIRE(output_lck);
            std::cout << "[DEBUG] Thread " << nthread << " steals task " << st->taskid << " from victim thread " << random_core << " successfully. \n";
            LOCK_RELEASE(output_lck);          
#endif		
            auto it = stealings[random_core].begin();
            stealings[random_core].insert(it, nthread);
            if(current_freq[random_core] == 2035200){
              required_freq[nthread] = 1574400;
            }
            if(current_freq[random_core] == 1574400){
              required_freq[nthread] = 1113600;
            }
            if(current_freq[random_core] == 1113600){
              required_freq[nthread] = 652800;
            }
            if(current_freq[random_core] == 652800){
              required_freq[nthread] = 345600;
            }
#ifdef DEBUG
            LOCK_ACQUIRE(output_lck);
            std::cout << "[DEBUG] Victim " << random_core << " current frequency is " << current_freq[random_core] << ", stealer " << nthread << " one level down to " << required_freq[nthread] << "\n";
            LOCK_RELEASE(output_lck);          
#endif	
          //}else{
            //st = NULL;
            //LOCK_RELEASE(worker_lock[random_core]);
            //continue;
          //}
        }
        LOCK_RELEASE(worker_lock[random_core]);  
      }while(!st && (attempts-- > 0));
      if(st){
        idle_try = 0;
        idle_times = 0;
        active_state[nthread] = 1;
        // status_working[nthread] = 1;
        continue;
      }
    }
#if (defined SLEEP) 
    if(idle_try >= IDLE_SLEEP){
      long int limit = (SLEEP_LOWERBOUND * pow(2,idle_times) < SLEEP_UPPERBOUND) ? SLEEP_LOWERBOUND * pow(2,idle_times) : SLEEP_UPPERBOUND;  
// #ifdef DEBUG
//         LOCK_ACQUIRE(output_lck);      
//         std::cout << "Thread " << nthread << " sleep for " << limit/1000000 << " ms.\n";
//         LOCK_RELEASE(output_lck);
// #endif
      status[nthread] = 0;
      status_working[nthread] = 0;
      tim.tv_sec = 0;
      tim.tv_nsec = limit;
      nanosleep(&tim , &tim2);
      //SleepNum++;
      AccumTime += limit/1000000;
      idle_times++;
      idle_try = 0;
      status[nthread] = 1;
    }
#endif
//#endif
    // 4. It may be that there are no more tasks in the flow
    // this condition signals termination of the program
    // First check the number of actual tasks that have completed
    LOCK_ACQUIRE(worker_lock[nthread]);
    // Next remove any virtual tasks from the per-thread task pool
    if(task_pool[nthread].tasks > 0){
      PolyTask::pending_tasks -= task_pool[nthread].tasks;
#ifdef DEBUG
      LOCK_ACQUIRE(output_lck);
      std::cout << "[DEBUG] Thread " << nthread << " removed " << task_pool[nthread].tasks << " virtual tasks. Pending tasks = " << PolyTask::pending_tasks << "\n";
      LOCK_RELEASE(output_lck);
#endif
      task_pool[nthread].tasks = 0;
    }
    LOCK_RELEASE(worker_lock[nthread]);
    
    // Finally check if the program has terminated
    if(gotao_can_exit && (PolyTask::pending_tasks == 0)){
      // end_flag[0] = true;
      *end_flag = true;
//#ifdef DEBUG
//      LOCK_ACQUIRE(output_lck);
//      std::cout << "[DEBUG] end_flag = true!\n";
//	    std::cout << "[DEBUG] end_flag " << *end_flag << std::endl;
//      LOCK_RELEASE(output_lck);
//#endif
#ifdef SLEEP
      LOCK_ACQUIRE(output_lck);
      std::cout << "Thread " << nthread << " sleeps for " << AccumTime << " ms. \n";
      LOCK_RELEASE(output_lck);
#endif
#ifdef NUMTASKS
      LOCK_ACQUIRE(output_lck);
      // for(int a = 1; a < 2; a = a*2){
          // std::cout << "Thread " << nthread << " with width " << a << " completes " << num_task[a * gotao_nthreads + nthread] << " tasks.\n";
          // num_task[a * gotao_nthreads + nthread] = 0;         
      // } 
      std::cout << "Thread " << nthread << " with width 1 completes " << num_task[nthread] << " tasks.\n";
      num_task[nthread] = 0;
      LOCK_RELEASE(output_lck);
#endif
#if (defined NUMTASKS_MIX) && (defined TX2)
      LOCK_ACQUIRE(output_lck);
      for(int b = 0;b < gotao_nthreads; b++){ // Different kernels
        // for(int a = 1; a < gotao_nthreads; a = a*2){ // Different width: 1,2,4 on Jetson TX2
          std::cout << "Kernel index "<< b << ": thread " << nthread << " with width 1 completes " << num_task[b][nthread] << " tasks.\n";
          num_task[b][nthread] = 0;
        // }
      }
      LOCK_RELEASE(output_lck);
#endif
#ifdef EXECTIME
      LOCK_ACQUIRE(output_lck);
      std::cout << "The total execution time of thread " << nthread << " is " << elapsed_exe.count() << " s.\n";
      LOCK_RELEASE(output_lck);
#endif
      return 0;
    }
  }
  return 0;
}
