#ifndef _ZJUNIX_SCHE_H
#define _ZJUNIX_SCHE_H

#include <zjunix/list.h>
#include <zjunix/task.h>

//前后台时间片
#define FOREGROUNG_TIMESLICES 8
#define BACKGROUND_TIMESLICES 2
//不同队列的时间片
#define BACKGROUND_PER_TIMESLICES 4
#define HIGH_TIMESLICES 2
#define ABOVE_NORMAL_TIMESLICES 4
#define NORMAL_TIMESLICES 8
#define BELOW_NORMAL_TIMESLICES 12
#define IDLE_TIMESLICES 16

typedef struct list_head list_head;
extern list_pcb *current;
//判断前后台的时间片
int counter;
//前后台的标志1->前台  0->后台
int flag;

//后台队列，固定时间片
list_pcb background_list;
//前台队列优先级排列,时间片
list_pcb high_list;
list_pcb above_normal_list;
list_pcb normal_list;
list_pcb below_normal_list;
list_pcb idle_list;
list_pcb test;
list_pcb example_list;
list_pcb *next_list;

void test_sched();
void sched_example();

PCB* get_current_pcb();
//初始化调度系统
void init_sched();

//计时器
unsigned int sched_timer();

unsigned int list_is_empty(list_pcb *list);

//进程调度操作，选取下一个进程，切换上下文
unsigned int sched();

//取就绪队列中的第一个进程
list_pcb *get_first_task(list_pcb *task);

//将进程添加到后台就绪队列
void add_to_background_list(list_pcb *task);

//将进程添加到前台优先级最高的队列
void add_to_foreground_list(list_pcb *task);

//取出后台队列的第一个进程
unsigned int background_sched();

//取出前台队列中优先级最高队列的第一个进程
unsigned int foreground_sched();

void init_list(list_pcb *list);

//将进程加到就绪队列队尾
void insert_tail(list_pcb *task,list_pcb *head);

//调度函数
void schedule(unsigned int status, unsigned int cause, context* pt_context);

void print_context(context* pt_context);
#endif