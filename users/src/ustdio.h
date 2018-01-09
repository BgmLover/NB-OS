#ifndef USTDIO_H
#define USTDIO_H

#include"fat.h"


typedef unsigned char* va_list;
#define va_start(ap, v) (ap = (va_list)&v + _INTSIZEOF(v))
#define _INTSIZEOF(n) ((sizeof(n) + sizeof(unsigned int) - 1) & ~(sizeof(unsigned int) - 1))
typedef unsigned int pgd_term;
typedef struct task_struct PCB;
typedef struct list_pcb list_pcb;
struct list_pcb{
    struct list_pcb  *prev;
    struct list_pcb  *next;
    PCB * pcb;
};
typedef struct {
    unsigned int epc;
    unsigned int at;
    unsigned int v0, v1;
    unsigned int a0, a1, a2, a3;
    unsigned int t0, t1, t2, t3, t4, t5, t6, t7;
    unsigned int s0, s1, s2, s3, s4, s5, s6, s7;
    unsigned int t8, t9;
    unsigned int hi, lo;
    unsigned int gp;
    unsigned int sp;
    unsigned int fp;
    unsigned int ra;
} context;
struct task_struct{
    context *context;
    char name[32];
    unsigned char asid;
    unsigned int parent;
    int uid;
    unsigned int counter;
    pgd_term *pgd;
    unsigned long start_time;
    unsigned int state;
    unsigned int priority;
    short policy;
    list_pcb sched;
    list_pcb process;
    struct TCB *thread_head;
    unsigned int  num_thread;  
    FILE * file;
    struct shared_memory* shm;
};
struct shared_memory
{
	unsigned int allocated; // 0->free
	unsigned int signal; // only 1 process can access
    unsigned int pid; // process id
	// struct page shm_page;
	char page[4096];
};


void uputchar(int ch, int fc, int bg);
void putchar_at(int ch,int row,int col);
int ugetchar();
void set_cursor();
int ustrcmp(const char* dest, const char* src) ;
void uclear_screen(int scope);
void uprintf(const char *format, ...);
void uputs(const char *s, int fc, int bg);
void* umalloc(unsigned int size);
void ufree(void* obj);

void ufopen(FILE *file,unsigned char *filename);
void ufclose(FILE *file);
void ufread(FILE *file,unsigned char *buffer,unsigned long count);
void ufwrite(FILE *file,unsigned char *buffer,unsigned long count);
void ucat(unsigned char *path);
void ulistfile(char *para);
void uvi(char *filename);

struct shared_memory* c_shm_get(struct task_struct* PCB);
unsigned int c_shm_mount(unsigned int pid, struct task_struct* PCB);
void c_shm_write(struct shared_memory* task, unsigned int offset, char p);
char c_shm_read(struct shared_memory* task, unsigned int offset);
struct shared_memory* c_shm_test();

int ukill();
int exec();
int print_proc();
unsigned int get_current_pcb();
int* getvga();
void demo_create();

void parse_cmd();


int bootmap_info();
int buddy_info();
int get_time();

#endif
