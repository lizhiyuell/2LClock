#ifndef _GLOBAL_H
#define _GLOBAL_H

#include<time.h>
#include<unistd.h>
#include<infiniband/verbs.h>
#include<infiniband/mlx5dv.h>
#include<algorithm>
#include<stdio.h>
#include<iostream>
#include<string.h>
#include<pthread.h>
#include<fstream>

#define buffersize 1024
#define port_base 30000
#define ts_port_delta 100

// net-work layouts
#define TREE_LAYOUT 0
#define CENTER_LAYOUT 1

#define TREE_DEGREE_MAX 3

// constant
#define BILLION 1000000000LL
#define MILLION 1000000

// Global clock
#define GTCenter_base_port 18999
#define MAX_SERVERS 100

// Slave clock record history offset dots
#define RECORD_MAX 100

// turn this on will ouput probe data
// #define OUTPUT_MODE

//------ type and structure ------//
typedef long long int ts_t;

// cpu&nic synchronization(CNS) parameter
#define CNS_interval 0.5 // in second
#define CNS_gap 1000
#define recalculation_epoch ((int)(CNS_interval*1000000)/CNS_gap) 
#define CNS_history_len 100
#define CNS_warmup_epoch 2
#define CNS_filter_percentage 0.8 // max is 1, the percentage to leave
#define CNS_alpha 0.5

// low-cpu-util related parameters
#define lcu_batch 200  // number of batch
#define MAX_PAIR	100  // the maximum opposite nodes
#define UD_port_delta 2727  // the connection port offset between RC and UD
#define DL_len (10*dot_num)  // the length of buffer in DataList
#define ud_batch 2000  // it must bigger than lcu batch
#define slave_batch_time 10000  // in us
#define lcu_buffer_size (sizeof(batch_data))
#define large_recv_batch 10
#define MODE_COMPENSATION 110 // in ns, which is the RC_OWD - UD_OWD

// timeslot parameters
#define fit_interval 0.5 // seconds
#define interval 1000 // this is probe interval, in us
#define dot_num (int)(fit_interval*1000000/interval)  // number of sampling in each epoch
#define warmup_epoch 3

// SVM fitting parameters
#define SVM_delay 2  // SVM delay slot(plus 0.5)

// constants
// ts state
#define PROBE_INV 0  // invalid
#define PROBE_UPR 1  // remote upper probe received
#define PROBE_LWR 1<<1  // remote lower probe received
#define PROBE_LR  1<<2  // local probe ready
#define PROBE_OK (PROBE_UPR|PROBE_LWR|PROBE_LR)


// port mapping function
#define PORT_MAP(port, id) (port+233*id)
#define PORT_MAP_BACKUP(port, id) (port+233*id+937)

// this structure define the data in large buffer
typedef struct{

	int data_num;
	int request_id[ud_batch];
	ts_t timestamp[ud_batch];
	bool is_rc_data[ud_batch]; // we mix two type of ts together, and use this to mark the ts type
	
}batch_data;


// get clock in ns
inline ts_t get_real_clock(){

	struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_nsec + 1000000000*ts.tv_sec;

}

inline ts_t get_nic_ts(ibv_context* context, ibv_values_ex* vex, mlx5dv_clock_info* clock_info){

    ibv_query_rt_values_ex(context, vex);
 
    return mlx5dv_ts_to_ns(clock_info, vex->raw_clock.tv_nsec);

}


#endif