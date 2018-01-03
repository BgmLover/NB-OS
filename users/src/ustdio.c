#include"ustdio.h"
void uputchar(int ch, int fc, int bg){
    unsigned int a0 = (unsigned int)ch;
    unsigned int a1 = (unsigned int)fc;
    unsigned int a2 = (unsigned int)bg;
    asm volatile(
        "move $a0,%0\n\t"
        "move $a1,%1\n\t"
        "move $a2,%2\n\t"
        "addi $v0,$zero,0\n\t"
        "syscall\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0),"r"(a1),"r"(a2)
    );
}

void putchar_at(int ch,int row,int col)
{
    unsigned int a0=ch;
    unsigned int a1=row;
    unsigned int a2=col;
    
    asm volatile(
        "move $a0,%0\n\t"
        "move $a1,%1\n\t"
        "move $a2,%2\n\t"
        "addi $v0,$zero,7\n\t"
        "syscall 7\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0),"r"(a1),"r"(a2)
    );
}

void set_cursor()
{
    asm volatile(
        "addi $v0,$zero,7\n\t"
        "syscall 7\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :
    );
}

int ugetchar()
{
    int c;
    asm volatile(
        "addi $v0,$zero,6\n\t"
        "syscall 6\n\t"
        "move %0,$v0\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :"=r"(c)
        :
    );
    return c;
}

int ustrcmp(const char* dest, const char* src) {
    unsigned int a0 = (unsigned int)dest;
    unsigned int a1 = (unsigned int)src;
    unsigned int v0;
    asm volatile(
        "move $a0,%1\n\t"
        "move $a1,%2\n\t"
        "addi $v0,$zero,1\n\t"
        "syscall\n\t"
        "nop\n\t"
        "move %0,$v0\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :"=r"(v0)
        :"r"(a0),"r"(a1)
    );

    return v0;
}

void uprintf(const char *format, ...){
    unsigned int a0;
    unsigned int a1;
    va_list ap;
    va_start(ap, format);
    a0 = (unsigned int)format;
    a1 = (unsigned int)ap;
    asm volatile(
        "move $a0,%0\n\t"
        "move $a1,%1\n\t"
        "addi $v0,$zero,3\n\t"
        "syscall\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0),"r"(a1)
    );
}

void uputs(const char *s, int fc, int bg) {
    unsigned int a0, a1, a2;
    a0 = (unsigned int)s;
    a1 = (unsigned int)fc;
    a2 = (unsigned int)bg;
    asm volatile(
        "move $a0,%0\n\t"
        "move $a1,%1\n\t"
        "move $a2,%2\n\t"
        "addi $v0,$zero,4\n\t"
        "syscall\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0),"r"(a1),"r"(a2)
    );
}

void uclear_screen(int scope){
    int a0 = scope;
    asm volatile(
        "move $a0,%0\n\t"
        "addi $v0,$zero,2\n\t"
        "syscall\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0)
    );
}

void* umalloc(unsigned int size){
    unsigned int a0 = size;
    unsigned int v0;
    asm volatile(
        "move $a0,%1\n\t"
        "addi $v0,$zero,21\n\t"
        "syscall\n\t"
        "nop\n\t"
        "move %0,$v0\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :"=r"(v0)
        :"r"(a0)
    );
    v0 &= 0x7fffffff;
    return (void*)v0;
}

void ufree(void* obj){
    unsigned int a0 = (unsigned int)obj;
    a0 |= 0x80000000;
    asm volatile(
        "move $a0,%0\n\t"
        "addi $v0,$zero,22\n\t"
        "syscall\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0)
    );
}

void ufopen(FILE *file,unsigned char *filename)
{
    unsigned int a0=(unsigned int)file;
    unsigned int a1=(unsigned int)filename;

    asm volatile(
        "move $a0,%0\n\t"
        "move $a1,%1\n\t"
        "addi $v0,$zero,51\n\t"
        "syscall 51\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0),"r"(a1)
    );
}

void ufclose(FILE *file)
{
    unsigned int a0=(unsigned int)file;

    asm volatile(
        "move $a0,%0\n\t"
        "addi $v0,$zero,52\n\t"
        "syscall 52\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0)
    );
}

void ufread(FILE *file,unsigned char *buffer,unsigned long count)
{
    unsigned int a0=(unsigned int)file;
    unsigned int a1=(unsigned int)buffer;
    unsigned int a2=(unsigned int)count;

    asm volatile(
        "move $a0,%0\n\t"
        "move $a1,%1\n\t"
        "move $a2,%2\n\t"
        "addi $v0,$zero,53\n\t"
        "syscall 53\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0),"r"(a1),"r"(a2)
    );
}

void ufwrite(FILE *file,unsigned char *buffer,unsigned long count)
{
    unsigned int a0=(unsigned int)file;
    unsigned int a1=(unsigned int)buffer;
    unsigned int a2=(unsigned int)count;

    asm volatile(
        "move $a0,%0\n\t"
        "move $a1,%1\n\t"
        "move $a2,%2\n\t"
        "addi $v0,$zero,54\n\t"
        "syscall 54\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0),"r"(a1),"r"(a2)
    );
}

void ucat(unsigned char *path)
{
    unsigned int a0=(unsigned int)path;

    asm volatile(
        "move $a0,%0\n\t"
        "addi $v0,$zero,55\n\t"
        "syscall 55\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0)
    );
}

void ulistfile(char *para)
{
    unsigned int a0=(unsigned int)para;

    asm volatile(
        "move $a0,%0\n\t"
        "addi $v0,$zero,56\n\t"
        "syscall 56\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0)
    );
}

void uvi(char *filename)
{
    unsigned int a0=(unsigned int)filename;

    asm volatile(
        "move $a0,%0\n\t"
        "addi $v0,$zero,57\n\t"
        "syscall 57\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0)
    );
}

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
}

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
}

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
}

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
}
int* getvga()
{
    int *cursor=umalloc(2*sizeof(int));
    asm volatile(
        "addi $v0,$zero,5\n\t"
        "syscall 5\n\t"
        "move %0,$v0\n\t"
        "move %1,$v1\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :"=r"(cursor[0]),"=r"(cursor[1])
        :
    );
    return cursor;
}

int ukill()
{
    return 1;
}

int exec()
{
    return 1;
}

int print_proc()
{
    return 1;
}

int bootmap_info()
{
    return 1;
}

int buddy_info()
{
    return 1;
}

int get_time()
{
    return 1;
}

void demo_create()
{
    asm volatile(
        "addi $v0,$zero,9\n\t"
        "syscall 9\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :
    );
}
struct task_struct* get_current_pcb(){
    unsigned int addr_pcb;
    asm volatile(
        "addi $v0,$zero,40\n\t"
        "syscall 40\n\t"
        "nop\n\t"
        "move %0,v0\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :"=r"(addr_pcb)
        :
    );
    return addr_pcb;
}
