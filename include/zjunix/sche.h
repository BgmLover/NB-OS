#ifndef _ZJUNIX_SCHE_H
#define _ZJUNIX_SCHE_H

#include<zjunix/task.h>
PCB *current;
PCB* get_current_pcb(){return current;}
#endif