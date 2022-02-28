#ifndef _CLOCK_FAILURE
#define _CLOCK_FAILURE

#include"clock_sync.h"

/*
clock_failure.cpp provides a wrapper for clock_sync.cpp to support failure handling. You can use clock_sync.cpp purely if there is no need for failure recovery.
*/

// the failure-resistant timer
class fr_timer{

public:
    void init(int local_port, int id1, int id2);
    void failure_handle();    
    ts_t get_timestamp(int* state);
    void finalize();

    GlobalTimer* current_timer;
    GlobalTimer* backup_timer;
    ts_t virtual_x[2];
    ts_t virtual_y[2];
    double k;
    ts_t shift;
    ts_t switch_delta;

    pthread_t f_thread;
    int state; // -1 for warmuping of current, 0 for current timer, 1 for virutal time, 2 for smoothing phase, 3 for backup timer
    int timer_id;
    char backup_ip[64];
};

// this contains the previous root and backup root. An offset is calculated after failure, and this will be send to fr_timer as a fixed value
class fr_root{
    
public:
    GlobalTimer* current_timer;
    GlobalTimer* backup_timer;
    void run();  // we assign port and NODE id in function
    void failure_handle();
    void finalize();

    ts_t offset;  // the clock offset when previous root fails

    pthread_t t[2];

};

// the center of fr-timer. 
// We don't take into account the failure of fr-center
class fr_center{
public:
    void run();
    void build_current_connect();
    void build_backup_connect();
};



#endif

