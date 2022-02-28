#include"clock_failure.h"

int main(){

    srand(time(NULL));

    fr_root froot;
    froot.run();
    printf("[Info] Wait for failure detection\n");
    // getchar();
    // usleep(2000000+rand()%1000000);
    sleep(5);
    printf("Simulated failure happens!\n");

    froot.failure_handle();

    // sleep(40);
    printf("froot gonna quit.\n");
    froot.finalize();
    return 0;
    
}