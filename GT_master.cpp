#include"clock_sync.h"
#include<stdio.h>
#include<pthread.h>

GlobalTimer gt;

int main(int argc, char** argv){

    assert(argc==2);
    int node_id = atoi(argv[1]);

    gt.init(12450+233*node_id, node_id);
    gt.run();
    while(!gt.is_warmup_done()) sched_yield();
    printf("Warm up done.\n");

    // example API : get timestamp
    // printf("Please enter a char.\n");
    // getchar();
    long long int timestamp = gt.get_timestamp();
    printf("Timestamp is %lld\n", timestamp);

    sleep(10);

    gt.set_done();
    gt.finalize();

    return 0;

}