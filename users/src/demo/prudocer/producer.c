#include "producer.h"

void producer(){
    /*
    char data;
    unsigned int offset = 0;
    struct shared_memory* shm;
    
    uprintf("%d\n", 4321);
    
    
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
    */
    // uputs("This is a user program.\n",0xf0,0);
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


    while(1);
    
}
