#include <users/ustdio.h>

void producer(){
    char data;
    int offset = 0;
    PCB* producer_pcb;
    struct shared_memory* shm;

    shm = c_shm_get(producer_pcb);
    producer_pcb = get_current_pcb(); // need implement

    // write 26 letters
    for(data = 'a'; data <= 'z'; data++){
        c_shm_write(producer_pcb, offset, data);
        offset++;
        printf("producer write:%c\n", data);
    }

    while(1){

    }

}
