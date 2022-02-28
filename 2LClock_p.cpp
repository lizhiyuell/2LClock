#include"clock_sync.h"
#include<stdio.h>
#include<pthread.h>

#define thread_count 2
#define test_times 50000

int number[10];
long long int ts_array[thread_count][test_times];
long long int cpu_ts_array[thread_count][test_times];
long long int get_ts_duration[thread_count][test_times];
bool flag;
int ready[thread_count];
bool next[2];
int file_id;
int call_duration[2][test_times];

void* test(void* argv){

    int id = *((int*)argv);  // we let two child reside in one node
    int idp = id+1;

    printf("Gonna bind on core %d\n", idp);
    GlobalTimer gt; 
    gt.init(12450+233*(idp), (idp));
    gt.run();

    while(!gt.is_warmup_done()) sched_yield();
    printf("Warm up done in node %d.\n", idp);
    ready[id] = true;

    long long int ts = 0, ts2 = 0, ts3 = 0;
    for(int i=0; i<test_times; i++){
        while(flag==false)
            sched_yield();
        // ts_t start = get_real_clock();
        ts3 = get_real_clock();
        ts = gt.get_timestamp();
        ts2 = get_real_clock();
        // ts_t end = get_real_clock();
        get_ts_duration[id][i] = ts2 - ts3;
        ts_array[id][i] = ts;
        cpu_ts_array[id][i] = ts2;
        // call_duration[id][i] = (int)(end-start);
        next[id] = true;
        while(flag==true)
            sched_yield();
    }
    printf("Gonna finalize.\n");
    gt.set_done();
    printf("Set done finish in %d\n", idp);
    gt.finalize();
    printf("test finish\n");

    printf("Thread %d quits\n", idp);
    return NULL;

}

int main(int argc, char** argv){

    // arg2: file id. arg3: load/no-load
    assert(argc==3);
    file_id = atoi(argv[1]);

    for(int i=0; i<thread_count; i++){
        number[i] = i;
        ready[i] = false;
    }
    flag = false;

    next[0] = false;
    next[1] = false;

    pthread_t thread_array[thread_count];

    printf("start test.\n");
    // test TS bandwidth and latency
    for(int i=0; i<thread_count; i++)
        pthread_create(thread_array+i, NULL, test, number+i);

retry:
    for(int i=0; i<thread_count; i++)
        if(!ready[i]){
            sched_yield();
            goto retry;
        }
            

    sleep(2);
    printf("Gonna test accuracy\n");
    for(int i=0; i<test_times; i++){
        flag = true;
    re:
        for(int j=0; j<thread_count; j++)
            if(!next[j])
                goto re;
        flag = false;
        for(int k=0; k<thread_count; k++)
            next[k] = false;
        usleep(1000);
        if(i%10000==0)
            printf("%d finish\n", i);
    }
    printf("ts test finish\n");

    for(int i=0; i<thread_count; i++)
        pthread_join(thread_array[i], NULL); 

    ts_t error_array[test_times];
    for(int i=0; i<test_times; i++){
        // ts_array[0][i] -= ts_array[1][i];
        error_array[i] = std::abs<ts_t>(ts_array[0][i] - ts_array[1][i] - (cpu_ts_array[0][i] - cpu_ts_array[1][i]));
        // error_array[i] = ts_array[0][i] - ts_array[1][i];
    }

    char output_file[128];
    sprintf(output_file, "./data/2LClock_%d_%s.txt", file_id, argv[2]);


    std::fstream ofs;
    if(file_id!=-1)
        ofs.open(output_file, std::ios::out);

    ts_t error = 0;
    ts_t duration = 0;
    for(int i=0; i<test_times; i++){
        // printf("%lld ns\n", ts_array[0][i]);
        error += error_array[i];
        duration += (get_ts_duration[0][i]+get_ts_duration[1][i]);
        if(file_id!=-1)
            ofs<<error_array[i]<<" "<<cpu_ts_array[0][i]<<" "<<cpu_ts_array[1][i]<<std::endl;
    }
    error /= test_times;
    duration /= (2*test_times);
    printf("[RESULT]:\navg: %lld, ", error);
    if(file_id!=-1)
        ofs.close();

    std::sort(error_array, error_array+test_times);
    
    ts_t mid_error = error_array[test_times/2];
    ts_t p90_error = error_array[(int)(test_times*0.90)];
    ts_t p95_error = error_array[(int)(test_times*0.95)];
    ts_t p99_error = error_array[(int)(test_times*0.99)];
    ts_t max_error = error_array[test_times-1];
    printf("mid: %lld, p90: %lld, p95: %lld, p99: %lld, max: %lld\n", mid_error, p90_error, p95_error, p99_error, max_error);

    return 0;

}
