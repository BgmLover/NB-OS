#ifndef _ZJUNIX_SHM_H
#define _ZJUNIX_SHM_H

#include <zjunix/buddy.h>
#include <zjunix/slub.h>
#include <zjunix/task.h>

#define MAX_SHM 256



// extern unsigned int shm_map[MAX_SHM];
extern struct shared_memory shm[MAX_SHM];


extern void shm_init();
extern void shm_setsignal(struct shared_memory* shm); // set signal = 1
extern struct shared_memory* shm_get(unsigned int size); // get a allocated shared memory
extern void shm_delete(struct shared_memory* shm); //delete a shared memory
extern void shm_mount(struct task_struct* task, struct shared_memory* addr); // mount a shm to a process
extern void shm_umount(struct task_struct* task, struct shared_memory* addr); // umount a shm from a process
extern void shm_write(struct task_struct* task, unsigned int offset, unsigned int p);
extern unsigned int shm_read(struct task_struct* task, unsigned int offset);


#endif