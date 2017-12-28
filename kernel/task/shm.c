#include <zjunix/shm.h>
#include <driver/vga.h>
#include <zjunix/task.h>

// struct shared_memory shm;

// unsigned int shm_map[MAX_SHM]
struct shared_memory shm[MAX_SHM];


void shm_setsignal(struct shared_memory* shm){
	shm->signal = 1;
}

void shm_init(){
	unsigned int i;
	unsigned int j;

	for(i=0; i<MAX_SHM; i++){
		// shm_map[i] = 0;
		shm_setsignal(&shm[i]);
		shm[i].allocated=0;
		// shm[i].page=0;
		for(j=0;j<4096; j++){
			shm[i].page[j]=0;
		}
	}
}



struct shared_memory* shm_get(unsigned int size){
	// void* addr;
	// struct shared_memory shm;
	// struct page* page;
	unsigned int i;

	// find a free shm
	i=0;
	while(shm[i].allocated!=0) i++;
	shm[i].allocated = 1;
	shm_setsignal(&shm[i]);

	return &shm[i];
	/*
	addr = kmalloc(size);
	page = pages + ((unsigned int)addr >> PAGE_SHIFT);
	*/

}



void shm_delete(struct shared_memory* shm){
	unsigned int i; 

	shm_setsignal(shm);
	shm->allocated = 0; //free
	for(i=0;i<4096;i++){
		shm->page[i]=0;
	}
}


void shm_mount(struct task_struct* task, struct shared_memory* shm){
	task->shm = shm;
	return;
}



void shm_umount(struct task_struct* task, struct shared_memory* shm){
	task->shm = 0;
	return;
}	

void shm_write(struct task_struct* task, unsigned int offset, unsigned int p){
	while(task->shm->signal!=1){

	}
	shm->signal = 0;
	kernel_printf("process%d:lock write\n", (unsigned int)task->asid);
	*(unsigned int*)(task->shm->page+offset)=p;
	shm->signal = 1;
	kernel_printf("process%d:unlock write\n", (unsigned int)task->asid);
	return;
}

unsigned int shm_read(struct task_struct* task, unsigned int offset){
	unsigned int res;
	while(task->shm->signal!=1){

	}
	shm->signal = 0;
	kernel_printf("process%d:lock read\n", (unsigned int)task->asid);
	res = *(unsigned int*)(task->shm->page+offset);
	shm->signal = 1;
	kernel_printf("process%d:unlock read\n", (unsigned int)task->asid);
	return res;
	
}