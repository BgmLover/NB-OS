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



struct shared_memory* shm_get(struct task_struct* PCB){
	// void* addr;
	// struct shared_memory shm;
	// struct page* page;
	unsigned int i;

	// find a free shm
	i=0;
	while(shm[i].allocated!=0){
		i++;
		if(i==256){
			kernel_printf("SHM run not!\n");
			while(1){

			}
		}
	} 
	shm[i].allocated = 1;
	shm[i].pid = PCB->asid;
	shm_setsignal(&shm[i]);

	return &shm[i];
	/*
	addr = kmalloc(size);
	page = pages + ((unsigned int)addr >> PAGE_SHIFT);
	*/

}

struct shared_memory* shm_test(){
	unsigned int i;
	// find a free shm
	i=0;
	while(shm[i].allocated!=0){
		i++;
		if(i==256){
			kernel_printf("SHM run not!\n");
			while(1){

			}
		}
	}
	shm[i].allocated = 1;
	shm[i].pid = 0;
	shm_setsignal(&shm[i]);
	return &shm[i];
}



void shm_delete(struct shared_memory* shm){
	unsigned int i; 

	shm_setsignal(shm);
	shm->allocated = 0; //free
	for(i=0;i<4096;i++){
		shm->page[i]=0;
	}
}


unsigned int shm_mount(unsigned int pid, struct task_struct* PCB){
	int i;
	for(i=0;i<256;i++){
		if(shm[i].pid==pid) break;
	}
	if(i==256){
		kernel_printf("mount error!\n");
		while(1){

		}
		return 0;
	}

	PCB->shm = &shm[i];
	return 1;
}



unsigned int shm_umount(unsigned int pid, struct task_struct* PCB){
	int i;
	for(i=0;i<256;i++){
		if(shm[i].pid==pid) break;
	}
	if(i==256){
		kernel_printf("umount error!\n");
		while(1){

		}
		return 0;
	}

	PCB->shm = 0;
	return 1;
}	

void shm_write(struct shared_memory* shm, unsigned int offset, char p){
	/*
	if(task->shm==0){
		kernel_printf("No shared memory!\n");
		while(1){

		}
	}
	while(task->shm->signal!=1){

	}*/
	shm->signal = 0;
	// kernel_printf("process%d:lock write\n", (unsigned int)task->asid);
	*((shm->page)+offset)=p;
	// *(task->shm->page+offset)=p;
	shm->signal = 1;
	// kernel_printf("process%d:unlock write\n", (unsigned int)task->asid);
	return;
}

char shm_read(struct shared_memory* shm, unsigned int offset){
	char res;
	/*
	if(task->shm==0){
		kernel_printf("No shared memory!\n");
		while(1){
			
		}
	}
	while(task->shm->signal!=1){

	}*/
	shm->signal = 0;
	// kernel_printf("process%d:lock read\n", (unsigned int)task->asid);
	res = *((shm->page)+offset);
	shm->signal = 1;
	// kernel_printf("process%d:unlock read\n", (unsigned int)task->asid);
	return res;
	
}
/*
// a0 = PCB
struct shared_memory* c_shm_get(struct task_struct* PCB){
	unsigned int a0;
	unsigned int v0;
	a0 = (unsigned int)PCB;

	asm volatile()
        "move $a0,%1\n\t"
        "addi $v0,$zero,36\n\t"
        "syscall\n\t"
        "nop\n\t"
        "move %0,$v0\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :"=r"(v0)
        :"r"(a0)
    );

    return (struct shared_memory*)v0;	
}*/

void syscall_shm_get_36(unsigned int status, unsigned int cause, context* pt_context){
	struct shared_memory* shm;
	struct task_struct* PCB;
	PCB = (struct task_struct*)pt_context->a0;
	shm = shm_get(PCB);
	pt_context->v0 = (unsigned int)shm;
}
/*
// a0 = pid, a1 = PCB
unsigned int c_shm_umount(unsigned int pid, struct task_struct* PCB){
	unsigned int a0;
	unsigned int a1;
	unsigned int v0;

	a0 = (unsigned int)pid;
	a1 = (unsigned int)PCB;
	asm volatile(
        "move $a0,%1\n\t"
        "move $a1,%2\n\t"
        "addi $v0,$zero,37\n\t"
        "syscall\n\t"
        "nop\n\t"
		"move %0,$v0\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :"=r"(v0)
        :"r"(a0),"r"(a1)
    );
	
	return v0;
}*/
void syscall_shm_mount_37(unsigned int status, unsigned int cause, context* pt_context){
	unsigned int pid;
	struct task_struct* PCB;
	unsigned int flag;
	pid = pt_context->a0;
	PCB = (struct task_struct*)pt_context->a1;
	flag = shm_mount(pid, PCB);
	pt_context->v0 = flag;
}

/*
// a0 = task, a1 = offset, a2 = p
void c_shm_write(struct task_struct* task, unsigned int offset, char p){
	unsigned int a0, a1, a2;
	a0 = (unsigned int)task;
	a1 = (unsigned int)offset;
	a2 = (unsigned int)p;
	asm volatile(
        "move $a0,%0\n\t"
        "move $a1,%1\n\t"
		"move $a2,%2\n\t"
        "addi $v0,$zero,38\n\t"
        "syscall\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0),"r"(a1),"r"(a3)
    );
}*/

void syscall_shm_write_38(unsigned int status, unsigned int cause, context* pt_context){
	unsigned int offset;
	struct task_struct* task;
	char p;
	task = (struct task_struct*)pt_context->a0;
	offset = pt_context->a1;
	p = (char)pt_context->a2;
	shm_write(task, offset, p);
}
/*
// a0 = task, a1 = offset
char c_shm_read(struct task_struct* task, unsigned int offset){
	unsigned int a0, a1;
	unsigned int v0;
	a0 = (unsigned int)task;
	a1 = (unsigned int)offset;
	asm volatile(
        "move $a0,%1\n\t"
        "move $a1,%2\n\t"
        "addi $v0,$zero,39\n\t"
        "syscall\n\t"
        "nop\n\t"
		"move %0,$v0\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :"=r"(v0)
        :"r"(a0),"r"(a1)
    );
	return (char)v0;
}*/

void syscall_shm_read_39(unsigned int status, unsigned int cause, context* pt_context){
	struct task_struct* task;
	unsigned int offset;
	char p;
	task = (struct task_struct*)pt_context->a0;
	offset = pt_context->a1;
	p = shm_read(task,offset);
	pt_context->v0 = (unsigned int)p;
}

void syscall_shm_test_41(unsigned int status, unsigned int cause, context* pt_context){
	struct shared_memory* shm;
	shm = shm_test();
	pt_context->v0 = (unsigned int)shm;
}
