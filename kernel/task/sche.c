#include "sche.h"
#include <intr.h>
#include <zjunix/task.h>
#include <zjunix/time.h>
#include <driver/vga.h>
#include <zjunix/list_pcb.h>
#include <zjunix/syscall.h>
#include <zjunix/utils.h>
#include <debug.h>
 
//判断前后台的时间片
int counter;
//前后台的标志1->前台0->后台
int flag;
list_pcb *current=NULL;
//后台队列，固定时间片
list_pcb *background_list;
//前台队列优先级排列,时间片
list_pcb *high_list;
list_pcb *above_normal_list;
list_pcb *normal_list;
list_pcb *below_normal_list;
list_pcb *idle_list;

PCB* get_current_pcb()
{
    return current->pcb;
}


void init_sched()
{//未完成
    //初始化各队列
    init_pcb_list(background_list);
    init_pcb_list(high_list);
    init_pcb_list(above_normal_list);
    init_pcb_list(normal_list);
    init_pcb_list(below_normal_list);
    init_pcb_list(idle_list);

    #ifdef SCHED_DEBUG
    kernel_printf("List init complete!");
    #endif
    
    //初始情况为前台队列进程
    flag=1;
    counter=FOREGROUNG_TIMESLICES;


    
    current=get_first_task(&pcbs);
    add_to_foreground_list(current);
    //register_syscall(10, pc_kill_syscall);
    register_interrupt_handler(7, pc_schedule);

    asm volatile(
        "li $v0, 1000000\n\t"
        "mtc0 $v0, $11\n\t"
        "mtc0 $zero, $9");
}

void pc_schedule(unsigned int status, unsigned int cause, context* pt_context) {
    // Save context
    copy_context(pt_context, current->pcb->context);
    int i;
    
    if(sched())
        goto sched_error;

    // Load context
    copy_context(current->pcb->context, pt_context);
    asm volatile("mtc0 $zero, $9\n\t");

sched_error:
    kernel_printf("No process now!\n");
}

//初始化队列
void init_pcb_list(list_pcb *list)
{
    list->prev=list;
    list->next=list;
}

unsigned int list_is_empty(list_pcb *list)
{
    if((list->prev==list)&&(list->next==list))
        return 1;
    else 
        return 0;
}

//取队列中的第一个进程
list_pcb *get_first_task(list_pcb *head)
{
    list_pcb *task;

    if(head->next==head)
        return NULL;

    task=head->next;
    head->next=task->next;
    task->next->prev=head;

    init_pcb_list(task);

    return task;
}

//将进程加到队列首
void insert_head(list_pcb *task,list_pcb *head)
{
    task->next=head->next;
    task->prev=head;
    head->next=task;
}

//将进程加到就绪队列队尾
void insert_tail(list_pcb *task,list_pcb *head)
{
    task->next=head;
    task->prev=head->prev;
    head->prev->next=task;
}

void add_to_foreground_list(list_pcb *task)
{
    list_pcb_add_tail(task,high_list);
}

void add_to_background_list(list_pcb *task)
{
    list_pcb_add_tail(task,background_list);
}

//后台进程调度
unsigned int background_sched()
{
    list_pcb *next;
    list_pcb *old;

    if(list_is_empty(background_list))
    {
        kernel_printf("No process background\n");
        goto background_sched_error;
    }
    
    next=get_first_task(background_list);
    if(next!=current)
    {
        old=current;
        current=next;
        current->pcb->state=STATE_RUNNING;
        old->pcb->state=STATE_READY;
        old->pcb->counter=BACKGROUND_PER_TIMESLICES;
        add_to_background_list(old);
        init_pcb_list(current);
    }
    else
        current->pcb->counter=BACKGROUND_PER_TIMESLICES;

    return 0;

background_sched_error:
    return 1;  
}

//前台进程调度
unsigned int foreground_sched()
{
    list_pcb *next;
    list_pcb *old;
    unsigned int lower_priority;
    unsigned int lower_timeslices;
    list_pcb *lower_list;

    if(!list_is_empty(high_list))
    {
        next=get_first_task(high_list);
        lower_priority=ABOVE_NORMAL_PRIORITY;
        lower_timeslices=ABOVE_NORMAL_TIMESLICES;
        lower_list=above_normal_list;
        
    }else if(!list_is_empty(above_normal_list))
    {
        next=get_first_task(above_normal_list);
        lower_priority=NORMAL_PRIORITY;
        lower_timeslices=NORMAL_TIMESLICES;
        lower_list=normal_list;
    }else if(!list_is_empty(normal_list))
    {
        next=get_first_task(normal_list);
        lower_priority=BELOW_NORMAL_PRIORITY;
        lower_timeslices=BELOW_NORMAL_TIMESLICES;
        lower_list=below_normal_list;
    }else if(!list_is_empty(below_normal_list))
    {
        next=get_first_task(below_normal_list);
        lower_priority=IDLE_PRIORITY;
        lower_timeslices=IDLE_TIMESLICES;
        lower_list=idle_list;
    }else if(!list_is_empty(idle_list))
    {
        next=get_first_task(idle_list);
        lower_priority=IDLE_PRIORITY;
        lower_timeslices=IDLE_TIMESLICES;
        lower_list=idle_list;
    }else 
    {
        kernel_printf("No process foreground\n");
        return 1;
    }

    old=current;
    current=next;
    current->pcb->state=STATE_RUNNING;
    old->pcb->state=STATE_READY;
    old->pcb->priority=lower_priority;
    old->pcb->counter=lower_timeslices;
    list_pcb_add_tail(old,lower_list);
    init_pcb_list(current);

    #ifdef SCHED_DEBUG
    kernel_printf("switch to %d list",lower_priority);
    kernel_printf("current timeslices is %d",lower_timeslices);
    #endif

    return 0;

}

//调度进程
/*unsigned int sched()
{
    PCB *next;
    PCB *old;  

    next=get_first_task(list_ready)->pcb;
    if(!next){
        kernel_printf("next task is null!\n");
        return 1;
    }

    if(next!=current){
        old=current;
        current=next;
        current->state=STATE_RUNNING;
        old->state=STATE_READY;
        insert_tail(&(old->sched),list_ready);
        INIT_LIST_PCB(&(current->sched));
    }

    return 0;
}*/

unsigned int sched()
{
    if(--counter!=0)
    {
        if(flag)
        {
            flag=0;
            counter=BACKGROUND_TIMESLICES;
            //取出后台队列的第一个进程
            if(background_sched())
                goto sched_error;
        }
        else
        {
            flag=1;
            counter=FOREGROUNG_TIMESLICES;
            //取出前台队列中优先级最高队列的第一个进程
            if(foreground_sched())
                goto sched_error;
        }
    }
    else
    {
        if(--(current->pcb->counter))
            goto sched_ok;
        else
        {
            if(flag)
            {
                if(foreground_sched())
                goto sched_error;
            }
            else
            {
                if(background_sched())
                goto sched_error;
            }
        }
    }

sched_ok:
    return 0;
sched_error:
    return 1;
}




