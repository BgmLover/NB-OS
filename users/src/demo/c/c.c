#include "producer.h"

void producer(){
    uprintf("printf:This is a user program.\n");
    uputs("puts:This is a user program.\n",0xfff,0x0);
    uputchar('c',0xfff,0x0);
    uprintf("\n");

    unsigned int addr;
    addr = (unsigned int)umalloc(4096);
    uprintf("malloc addr=%x\n",addr);

    struct shared_memory* shm;
    shm = c_shm_test();
    uprintf("shm:%x\n",(unsigned int)shm);
    return;
    
}
