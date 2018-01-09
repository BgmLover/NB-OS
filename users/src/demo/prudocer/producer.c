#include "producer.h"

void producer(){
    
    char data;
    unsigned int offset = 0;
    struct shared_memory* shm;
    
    uprintf("%d\n", 4321);
    /*
    producer_pcb = (PCB*)get_current_pcb(); // need implement
    uprintf("%x\n",producer_pcb);
    */
    
	shm = c_shm_test();
    uprintf("%x\n", (unsigned int)shm);
    //  while(1);

    // write 26 letters
    data='a';
    offset=0;
    uprintf("offset:%x\n",offset);
    while(1){
        uprintf("offset:%x\n",offset);
        uprintf("write:%c\n",data);
        offset++;
        uprintf("offset:%x\n",offset);
        while(1);
    }

    while(1){

    }
    
    
}
