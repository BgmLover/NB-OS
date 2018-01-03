#include <users/ustdio.h>

void customer(){
    char data;
    int offset=0;
    int flag;
    int i;
    PCB* customer_pcb;

    flag = c_shm_mount(1, customer);
    if(flag == 0){
        printf("mount error!\n");
        while(1){

        }
    }

    customer_pcb = ; // need implement

    for(i = 0; i<26; i++){
        data = c_shm_read(customer_pcb, offset);
        offset++;
        printf("customer read:%c\n", data);
    }

    while(1){

    }
}