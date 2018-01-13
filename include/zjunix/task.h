#ifndef _ZJUNIX_TASK_H
#define _ZJUNIX_TASK_H


#include<zjunix/fs/fat.h>
#include<page.h>

//进程状态定义
#define STATE_WAITTING 0
#define STATE_READY    1
#define STATE_RUNNING  2
//默认时间片长度
#define DEFAULT_TIMESLICES 6
//设置5个进程优先级
#define HIGH_PRIORITY 13
#define ABOVE_NORMAL_PRIORITY 10
#define NORMAL_PRIORITY 8
#define BELOW_NORMAL_PRIORITY 6
#define IDLE_PRIORITY 4
//定义内核栈大小为一个页
#define KERNEL_STACK_SIZE 4096


//typedef 两个结构，方便使用
typedef struct list_pcb list_pcb;
typedef struct task_struct PCB;


//共享内存结构
struct shared_memory
{
	unsigned int allocated;     // 0->free
	unsigned int signal;        // only 1 process can access
    unsigned int pid;           // process id
	// struct page shm_page;
    int writep;
    int readp;
	char page[4096];
};

//context结构
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


// list_pcb是一个PCB链表中的一个具体节点的数据结构
struct list_pcb{
    list_pcb  *prev;
    list_pcb  *next;
    PCB * pcb;
};

//struct of PCB
struct task_struct{
    context *context;               //上下文
    char name[32];                  //进程名
    unsigned char asid;             //地址空间标志ID，即PID
    unsigned int parent;            //父进程id
    unsigned int counter;           //时间片
    pgd_term *pgd;                  //页目录指针（第一级页表）
    unsigned int state;             //进程的状态
    unsigned int priority;          //优先级
    list_pcb sched;                 //调度队列链表节点
    list_pcb process;               //PCB链表节点
    FILE * file;                    //文件指针
    struct shared_memory* shm;      //共享内存指针
};

/*task_union结构，为每个进程分配一个页大小的task_union，底部为PCB上面为进程内核栈*/
typedef union{
    PCB pcb;
    unsigned char kernel_stack[4096];
}task_union;

extern list_pcb pcbs;               //进程队列
extern unsigned char idmap[32];     //设置256个进程id

/*下面是与进程相关的主要函数，每个函数具体的说明在task.c当中*/

void copy_context(context* src, context* dest); 
void clean_context(context* dest);
task_union *create_task_union();
void init_task();
void init_code();
int do_fork(context* args,PCB*parent);
pgd_term *copy_pagetables(PCB* child,PCB* parent);
int exec2(PCB *task,char* filename);
int exec1(char* filename);
int exec(char *filename,char* taskname);
unsigned char get_emptypid();
PCB *get_pcb_by_pid(unsigned int pid);
void add_task(list_pcb* process);
unsigned int  del_task(unsigned int asid);
void print_tasks();



#endif