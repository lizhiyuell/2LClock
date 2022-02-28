#include"clock_sync.h"
#include<stdio.h>
#include<pthread.h>

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>

// #define thread_count 2
#define test_times 50000

int number[3];
ts_t result[2][test_times];
char master_ip[64];

void* master(void* argv){

    int id = *((int*)argv);

    rdma_context* ctx = create_rdma();
    rdma_bind(ctx, 12345+233*id);
    printf("Connection built with node 0.\n");

    
    char buff[8];

    for(int i=0; i<test_times; i++){
       
        rdma_recv(ctx, buff, 0, NULL);
        ts_t time = get_real_clock();

        rdma_send(ctx, buff, 0, NULL);
        result[id][i] = time;
        // tsocket_recv_ud(ctx, 1);

        if(i%2000==0)
            printf("Master %d finish %d\n", id, i);

    }

    printf("Master %d quits\n", id);
    return NULL;

}


void* slave(void* argv){

    int id = *((int*)argv);

    rdma_context* ctx = create_rdma();
    rdma_connect(ctx, master_ip, 12345+233*id);
    printf("Connection built with master.\n");

    char buff[8];

    for(int i=0; i<test_times; i++){

        ts_t start = get_real_clock();
        rdma_send(ctx, buff, 0, NULL);
        // int cnt = tsocket_poll_send(ctx, ts_array, qp_idx);
        // assert(cnt==1);
        // cnt = tsocket_poll_recv(ctx, ts_array, qp_idx, id_array, 1);
        rdma_recv(ctx, buff, 0, NULL);
        ts_t end = get_real_clock();
        // assert(cnt==1);
        // tsocket_recv_ud(ctx, 1);
        result[id][i] = (start+end)/2;
        usleep(1000);

    }

    printf("Slave thread quits\n");
    return NULL;

}

int main(int argc, char** argv){

    // args: file_id, load/noload, M(master) or S(slave)
    assert(argc>=4);

    bool is_master = true;
    if(argv[3][0]=='S'){
        is_master = false;
        assert(argc==5);
        strcpy(master_ip, argv[4]);
    }
        
    else
        assert(argv[3][0]=='M');

    int file_id = atoi(argv[1]);

    for(int i=0; i<2; i++){
        number[i] = i;
    }

    pthread_t thread_array[2];

    printf("start test.\n");
    // test TS bandwidth and latency

    if(is_master){
        pthread_create(thread_array, NULL, master, number);
        pthread_create(thread_array+1, NULL, master, number+1);
    }
    else{
        pthread_create(thread_array, NULL, slave, number);
        pthread_create(thread_array+1, NULL, slave, number+1);
    }
    

    pthread_join(thread_array[0], NULL);

    pthread_join(thread_array[1], NULL);

    if(!is_master){
        
        // Slave side send result to Master process
        int   sockfd;
        // char  recvline[4096], sendline[4096];
        struct sockaddr_in  servaddr;

        if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
            return 0;
        }

        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(10010);
        if( inet_pton(AF_INET, master_ip, &servaddr.sin_addr) <= 0){
            printf("inet_pton error for %s\n",master_ip);
            return 0;
        }

        while( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
            printf("connect error: %s(errno: %d), retrying...\n",strerror(errno),errno);
            // return 0;
        }

        printf("send msg to MASTER: \n");

        int n = send(sockfd, result, sizeof(result), 0);
        if( n < 0){
            printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
            return 0;
        }
        printf("Send %d finish\n", n);
        close(sockfd);
        // end of TCP transport
        printf("--- MSG exchange finish in SLAVE ---\n");

    }
    else{

        // Master side get result from slave process using TCP
        int  listenfd, connfd;
        struct sockaddr_in  servaddr;
        ts_t oppo_ts[2][test_times];
        char* buff = (char*)oppo_ts;

        if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
            printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
            return 0;
        }

        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(10010);

        if( bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
            printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
            return 0;
        }

        if( listen(listenfd, 10) == -1){
            printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
            return 0;
        }

        // printf("======waiting for client's request======\n");
        if(sizeof(oppo_ts)!=sizeof(result))
            assert(false);

        if( (connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
            assert(false);
        }

        int recv_count = 0;
        while(recv_count!=sizeof(oppo_ts)){
            recv_count += recv(connfd, buff+recv_count, sizeof(oppo_ts), 0);
            // printf("-*- %d, %d -*-\n", recv_count, sizeof(oppo_ts));
        }
        close(connfd);
        close(listenfd);
        printf("--- MSG exchange finish in MASTER ---\n");


        char path[128];
        sprintf(path, "./data/rdma_ntp_%d_%s.txt", file_id, argv[2]);

        std::fstream ofs(path, std::ios::out);

        ts_t error = 0;
        ts_t error_array[test_times];
        for(int i=0; i<test_times; i++){
            ts_t time1 = result[0][i]-oppo_ts[0][i];
            ts_t time2 = result[1][i]-oppo_ts[1][i];
            ts_t e = std::abs<long long int>(time1 - time2);
            error_array[i] = e;
            error+=e;

        }
        
        int len = test_times;
        // printf("[RESULT] avg: %lld, ", error/len);
        printf("avg: %lld, ", error/len);
        std::sort(error_array, error_array+len);

        for(int i=0; i<test_times; i++)
            ofs<<error_array[i]<<std::endl;
        ofs.close();
        
        ts_t mid_error = error_array[len/2];
        ts_t p90_error = error_array[(int)(len*0.90)];
        ts_t p95_error = error_array[(int)(len*0.95)];
        ts_t p99_error = error_array[(int)(len*0.99)];
        ts_t max_error = error_array[len-1];
        printf("mid: %lld, p90: %lld, p95: %lld, p99: %lld, max: %lld\n", mid_error, p90_error, p95_error, p99_error, max_error);

    }



    return 0;

}
