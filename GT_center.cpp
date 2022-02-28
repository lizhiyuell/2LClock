#include"clock_sync.h"
#include<stdio.h>
#include<string.h>
#include<fstream>
#include<assert.h>
#include <csignal>
#include <unistd.h>

int main(){

    uint32_t count;
    char** addr_list;
    int* port_list;
    char buff[64];

    std::fstream ifs("ifconfig", std::ios::in);
    if(!ifs){
        printf("Open config file failed\n");
        return 0;
    }
    
    ifs>>count;

    addr_list = (char**) malloc(count*sizeof(char*));
    port_list = (int*) malloc(count*sizeof(int));


    for(uint32_t idx=0; idx<count; idx++){
        addr_list[idx] = new char[64];
        ifs>>buff;
        strcpy(addr_list[idx], buff);
        port_list[idx] = 12450+233*idx;
    }

    ifs.close();

    GTCenter gtCenter(addr_list, port_list, count);

    gtCenter.build_connection_config(TREE_LAYOUT);

    printf("Send connection config finish\n");

    // collect warmup finish info
    msg2cent msg_array[count];
    gtCenter.collect_info(msg_array);

    for(uint32_t i=0; i<count; i++){
        assert(msg_array[i].is_warmup_done);
        assert(msg_array[i].msg_id==111);
    }
        
    printf("collect msg finish.\n");

    // send start msg to all nodes
    msg2master msg_array2[count];
    for(uint32_t i=0; i<count; i++)
        msg_array2[i].msg_id = 222;

    gtCenter.broadcast_info(msg_array2);

    // build TCP connection with other nodes

    printf("Warmup finish.\n");

    free(port_list);

    for(uint32_t idx=0; idx<count; idx++){
        delete addr_list[idx];
    }

    free(addr_list);

    gtCenter.finalize();
    printf("Releasing resources success\n");

    return 0;
}
