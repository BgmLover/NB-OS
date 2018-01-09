#include "customer.h"

void customer(){
    char data;
    int offset=0;
    int flag;
    int i;
    PCB* customer_pcb;
customer_pcb = (PCB*)get_current_pcb(); // need implement

    flag = c_shm_mount(1, customer_pcb);
    if(flag == 0){
        uprintf("mount error!\n");
        while(1){

        }
    }

    

    for(i = 0; i<26; i++){
        data = c_shm_read(customer_pcb, offset);
        offset++;
        uprintf("customer read:%c\n", data);
    }

    while(1){

    }
}
