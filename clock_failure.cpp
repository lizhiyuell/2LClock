#include"clock_failure.h"

void fr_timer::init(int local_port, int id1, int id2){

    state = -1;
    current_timer = new GlobalTimer;
    backup_timer = new GlobalTimer;
    timer_id = id1;

    printf("CURRENT timer port is %d, BACKUP timer port is %d\n", local_port+id1*233, local_port+937+id2*233);
    current_timer->init(PORT_MAP(local_port, id1), id1);
    printf("[FR] current timer INIT finish.\n");
    current_timer->run();
    printf("[FR] current timer warmup done.\n");
    __asm__ __volatile__("" ::: "memory");
    state = 0;

    backup_timer->init(PORT_MAP_BACKUP(local_port, id2), id2);
    printf("[FR] backup timer INIT finish.\n"); 

}

void* handle_func(void* argv){

    fr_timer* ptr = (fr_timer*) argv;
    // get the old array dot
    // a try: we just sample two dots
    ptr->current_timer->get_ts_map(ptr->virtual_x, ptr->virtual_y);
    sleep(1);
    ptr->current_timer->get_ts_map(ptr->virtual_x+1, ptr->virtual_y+1);
    ptr->k = 1.0*(ptr->virtual_y[1]-ptr->virtual_y[0])/(ptr->virtual_x[1]-ptr->virtual_x[0]);
    
    // simulation for failure
    printf("failure happens!\n");
    usleep(rand()%1000);  // We assume failure detection can be done within 1 ms
    
    // stop the old timer, this might be unecessary
    ptr->current_timer->set_done();
    // change the state
    ptr->state = 1;
    // ts_t tp1 = get_real_clock();
    __asm__ __volatile__("" ::: "memory");
    // use the second clock
    ptr->backup_timer->run();
    // ts_t tp2 = get_real_clock();
    // calculate the step
    // get a receive msg from the oppo side
    // connection ----------
    // printf("------------------------\n");
    // char ippp[] = "172.23.12.131";

    int comm_port = 11551+123*ptr->timer_id;
    printf("comm port is %s:%d\n", ptr->backup_ip, comm_port);
    rdma_context* ctx = create_rdma();
    rdma_connect(ctx, ptr->backup_ip, comm_port);
    // printf("connection finish\n");
    // ts_t tp3 = get_real_clock();
    ts_t switch_delta = -1;
    rdma_recv(ctx, (char*)(&switch_delta), sizeof(ts_t), NULL);
    // printf("TCP communication finish\n");
    finalize_rdma(ctx);
    // ts_t tp4 = get_real_clock();
    assert(switch_delta!=-1);

    // connection ----------   

    int state;
    ts_t shift = ptr->get_timestamp(&state) - ptr->backup_timer->get_timestamp() - switch_delta;

    // add the shift
    // printf("[Delta] %lld + %lld\n", delta, switch_delta);
    // delta += switch_delta;

    ptr->switch_delta = switch_delta;

    assert(state==1);
    // shift *= 2; // for monotonicity
    printf("[FR] The shift is %lld ns\n", shift);
    // printf("[FR] Backup warmup done, run into smoothing phase\n");
    
    ptr->shift = shift;

    // ts_t tp5 = get_real_clock();

    // printf("[Info] time duration are: %lld, %lld, %lld, %lld ms\n", (tp2-tp1)/1000000, (tp3-tp2)/1000000, (tp4-tp3)/1000000, (tp5-tp4)/1000000);

    if(shift>0){
        // only this circumstance needs a shift
        __asm__ __volatile__("" ::: "memory");
        ptr->state = 2;
        // we set a fixed decrease rate
        ts_t decrease_rate = 2;  // 5 ns/ms
        while (ptr->shift>0)
        {
            usleep(1000);
            ptr->shift = std::max<ts_t>(0, ptr->shift-decrease_rate);
        }
    }
    printf("[FR] Smoothing phase done\n");
    ptr->state = 3;

    return NULL;

}


void fr_timer::failure_handle(){

    pthread_create(&f_thread, NULL, handle_func, (void*)this);
    
}

ts_t fr_timer::get_timestamp(int* current_state){

    *current_state = state;
    ts_t cpu_ts;
    switch (*current_state)
    {
    case 0:
        return current_timer->get_timestamp();
    case 1:
        cpu_ts = get_real_clock();
        return (ts_t)((cpu_ts-virtual_x[1])*k)+virtual_y[1];
    case 2:
        return shift+backup_timer->get_timestamp()+switch_delta;
    case 3:
        return backup_timer->get_timestamp()+switch_delta;
    default:
        assert(false);
    }
    return 0;
}


void fr_timer::finalize(){

    pthread_join(f_thread, NULL);

    backup_timer->set_done();

    current_timer->finalize();
    backup_timer->finalize();
    delete current_timer;
    delete backup_timer;

}



void* run_current(void* argv){

    int current_node = 3;
    int current_port = 12450 + 233*current_node;
    printf("Root current bind on port %d\n", current_port);
    GlobalTimer* ptr = (GlobalTimer*)argv;
    
    ptr->init(current_port, current_node);
    ptr->run();

    printf("Root current warmup done.\n");

    return NULL;

}


void* run_backup(void* argv){

    int backup_node = 0;
    int backup_port = 12450 + 233*backup_node + 937;
    printf("Root back bind on port %d\n", backup_port);
    GlobalTimer* ptr = (GlobalTimer*)argv;

    ptr->init(backup_port, backup_node);
    printf("Root back init finish\n");

    return NULL;

}


void fr_root::run(){

    current_timer = new GlobalTimer;
    backup_timer = new GlobalTimer;

    pthread_create(t, NULL, run_current, (void*)current_timer);
    pthread_create(t+1, NULL, run_backup, (void*)backup_timer);

}


void fr_root::finalize(){


    pthread_join(t[0], NULL);
    pthread_join(t[1], NULL);


    // backup_timer->set_done();
    // while(!backup_timer->is_done())
    //     sched_yield();
    backup_timer->finalize();

    current_timer->set_done();
    current_timer->finalize();

    delete current_timer;
    delete backup_timer;

}

// 
void* send_msg_1(void* argv){

    ts_t delta = *((ts_t*)argv);
    int comm_port = 11551+123*1;
    rdma_context* ctx = create_rdma();
    rdma_bind(ctx, comm_port);
    rdma_send(ctx, (char*)(&delta), sizeof(ts_t), NULL);
    finalize_rdma(ctx);
    
    return NULL;
}

void* send_msg_2(void* argv){

    ts_t delta = *((ts_t*)argv);
    int comm_port = 11551+123*2;
    rdma_context* ctx = create_rdma();
    rdma_bind(ctx, comm_port);
    rdma_send(ctx, (char*)(&delta), sizeof(ts_t), NULL);
    finalize_rdma(ctx);
    return NULL;
}

void fr_root::failure_handle(){

    printf("Detect a failure, now switch to another root.\n");

    // record an offset
    ts_t local_time, delta;
    current_timer->get_nic_ts_map(&local_time, &delta);
    
    // run backup timer
    backup_timer->run();

    printf("------------------------\n");
    // communication connection
    pthread_t t[2];
    pthread_create(t, NULL, send_msg_1, (void*)(&delta));
    pthread_create(t+1, NULL, send_msg_2, (void*)(&delta));

    pthread_join(t[0], NULL);
    pthread_join(t[1], NULL);

    printf("communication finish.\n");

}

void fr_center::run(){
    build_current_connect();
    build_backup_connect();
}


void fr_center::build_current_connect(){

    uint32_t count;
    char** addr_list;
    int* port_list;
    char buff[64];

    std::fstream ifs("frconfig_normal", std::ios::in);
    if(!ifs){
        printf("Open config file failed\n");
        assert(false);
    }
    
    ifs>>count;

    addr_list = (char**) malloc(count*sizeof(char*));
    port_list = (int*) malloc(count*sizeof(int));

    int base_port = 12450;

    for(uint32_t idx=0; idx<count; idx++){
        addr_list[idx] = new char[64];
        ifs>>buff;
        strcpy(addr_list[idx], buff);
        port_list[idx] = base_port+233*idx;
    }

    ifs.close();

    GTCenter gtCenter(addr_list, port_list, count);

    gtCenter.build_connection_config(TREE_LAYOUT);

    printf("Send connection config to CURRENT timer finish\n");

    // collect warmup finish info
    msg2cent msg_array[count];
    gtCenter.collect_info(msg_array);

    for(uint32_t i=0; i<count; i++){
        assert(msg_array[i].is_warmup_done);
        assert(msg_array[i].msg_id==111);
    }
        
    printf("collect msg in CURRENT timer finish.\n");

    // send start msg to all nodes
    msg2master msg_array2[count];
    for(uint32_t i=0; i<count; i++)
        msg_array2[i].msg_id = 222;

    gtCenter.broadcast_info(msg_array2);

    // build TCP connection with other nodes

    printf("warmup done in CURRENT timer.\n");


    // printf("Test finish, gonna release resources\n");

    free(port_list);

    for(uint32_t idx=0; idx<count; idx++){
        delete addr_list[idx];
    }

    free(addr_list);

    gtCenter.finalize();
    printf("Releasing current Center success\n");

}

void fr_center::build_backup_connect(){

    // this function will run until failure happens

    uint32_t count;
    char** addr_list;
    int* port_list;
    char buff[64];

    std::fstream ifs("frconfig_backup", std::ios::in);
    if(!ifs){
        printf("Open config file failed\n");
        assert(false);
    }
    
    ifs>>count;

    addr_list = (char**) malloc(count*sizeof(char*));
    port_list = (int*) malloc(count*sizeof(int));

    int base_port = 12450 + 937;

    for(uint32_t idx=0; idx<count; idx++){
        addr_list[idx] = new char[64];
        ifs>>buff;
        strcpy(addr_list[idx], buff);
        port_list[idx] = base_port+233*idx;
    }

    ifs.close();

    GTCenter gtCenter(addr_list, port_list, count);

    gtCenter.build_connection_config(TREE_LAYOUT);

    printf("Send connection config finish.\nStart waiting for failure ...\n");

    // collect warmup finish info
    msg2cent msg_array[count];
    gtCenter.collect_info(msg_array);

    for(uint32_t i=0; i<count; i++){
        assert(msg_array[i].is_warmup_done);
        assert(msg_array[i].msg_id==111);
    }
        
    printf("Collect recover warm-up msg finish.\n");

    // send start msg to all nodes
    msg2master msg_array2[count];
    for(uint32_t i=0; i<count; i++)
        msg_array2[i].msg_id = 222;

    gtCenter.broadcast_info(msg_array2);

    // build TCP connection with other nodes

    printf("Recover connection build, center quits.\n");

    free(port_list);

    for(uint32_t idx=0; idx<count; idx++){
        delete addr_list[idx];
    }
    free(addr_list);

    gtCenter.finalize();
    printf("Releasing resources success\n");

}
