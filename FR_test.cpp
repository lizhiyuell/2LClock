#include"clock_failure.h"
#include<stdio.h>

#define thread_count 2
#define test_times 50000

/* change backup ip*/
char backup_ip[64];

int number[10];
long long int ts_array[thread_count][test_times];
int state_array[thread_count][test_times];
bool flag;
int ready[thread_count];
bool next[2];
int file_id;
// int cpu_core[thread_count]={10, 15};
int call_duration[2][test_times];
int cpu_core[2][test_times];

fr_timer ftimer_array[2];

void* test(void* argv){

    int id = *((int*)argv);  // we let two child reside in one node
    int idp = id+1;

    // printf("Gonna bind on core %d\n", idp);
    // cpu_set_t cpu;
    // CPU_ZERO(&cpu);
    // int cpu_id = (10*id+8)%40;
    // int cpu_id = 8+id;
    // CPU_SET(cpu_id, &cpu);
    // printf("%d set on %d\n", id, cpu_id);
    // sched_setaffinity(0, sizeof(cpu), &cpu);

    fr_timer* ftimer = ftimer_array+id;
    ftimer->init(12450, idp, idp);
    strcpy(ftimer->backup_ip, backup_ip);

    ready[id] = true;

    long long int ts = 0;
    int state;
    for(int i=0; i<test_times; i++){
        while(flag==false)
            sched_yield();
        // ts_t start = get_real_clock();
        ts = ftimer->get_timestamp(&state);
        // ts_t end = get_real_clock();
        state_array[id][i] = state;
        ts_array[id][i] = ts;
        // call_duration[id][i] = (int)(end-start);
        // cpu_core[id][i] = sched_getcpu();
        next[id] = true;
        while(flag==true)
            sched_yield();
    }
    printf("Gonna finalize.\n");
    ftimer->finalize();
    printf("Thread %d quits\n", idp);

    return NULL;

}

int main(int argc, char** argv){

    srand(time(NULL));

    assert(argc==2);
    strcpy(backup_ip, argv[1]);

    for(int i=0; i<thread_count; i++){
        number[i] = i;
    //     ready[i] = false;
    }
    // flag = false;

    // next[0] = false;
    // next[1] = false;

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
    printf("[Info]Timer ready, wait for a while time and simulate failure.\n");
    // getchar();
    sleep(2);
    // printf("Simulated failure happens!\n");

    // simulate failure
    ftimer_array[0].failure_handle();
    ftimer_array[1].failure_handle();


    printf("Gonna test precision\n");
    for(int i=0; i<test_times; i++){
        flag = true;
    re:
        for(int j=0; j<thread_count; j++)
            if(!next[j])
                goto re;
        flag = false;
        for(int k=0; k<thread_count; k++)
            next[k] = false;
        usleep(500);
        if(i%1000==0)
            printf("%d finish\n", i);
    }
    printf("ts test finish\n");

    ts_t error_array[test_times];

    for(int i=0; i<thread_count; i++)
        pthread_join(thread_array[i], NULL); 

    for(int i=0; i<test_times; i++){
        error_array[i] = ts_array[0][i] - ts_array[1][i];
    }

    char output_file[128];
    sprintf(output_file, "./precision_result/failure.txt");

    std::fstream ofs(output_file, std::ios::out);

    // ts_t error_array[test_times];

    // ts_t error = 0;
    for(int i=0; i<test_times; i++){
        // error += std::abs<long long int>(ts_array[0][i]);
        ofs<<error_array[i]<<", "<<ts_array[0][i]<<", "<<ts_array[1][i]<<", "<<state_array[0][i]<<", "<<state_array[1][i]<<std::endl;
    }


    ofs.close();



    return 0;

}
