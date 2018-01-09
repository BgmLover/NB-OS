#include "sche.h"
#include <intr.h>
#include <zjunix/task.h>
#include <zjunix/time.h>
#include <driver/vga.h>
#include <zjunix/slub.h>
#include <zjunix/list_pcb.h>
#include <zjunix/syscall.h>
#include <zjunix/utils.h>
#include <debug.h>
#include <arch.h>
#include <../usr/ps.h>

list_pcb *current=NULL;

PCB* get_current_pcb()
{
    return current->pcb;
}

void print_0_fun()
{
    int i=0;
    while(1)
    {
        i++;
        if(i==200)
            break;
        kernel_printf(" 0 ");
    }
}

void print_2_fun()
{
    int i=0;
    while(1)
    {
        i++;
        if(i==200)
            break;
        kernel_printf(" 2 ");
    }
}

void print_context(context* pt_context)
{
    kernel_printf("epc:  %x\n",pt_context->epc);
    kernel_printf("at:   %x\n",pt_context->at);
    kernel_printf("v0:  %x,  v1:  %x\n",pt_context->v0,pt_context->v1);
    kernel_printf("a0:  %x,  a1:  %x",pt_context->a0,pt_context->a1);
    kernel_printf("a2:  %x,  a3:  %x\n",pt_context->a2,pt_context->a3);
    kernel_printf("t0:  %x,  t1:  %x",pt_context->t0,pt_context->t1);
    kernel_printf("t2:  %x,  t3:  %x",pt_context->t2,pt_context->t3);
    kernel_printf("t4:  %x,  t5:  %x",pt_context->t4,pt_context->t5);
    kernel_printf("t6:  %x,  t7:  %x\n",pt_context->t6,pt_context->t7);
    kernel_printf("s0:  %x,  s1:  %x",pt_context->s0,pt_context->s1);
    kernel_printf("s2:  %x,  s3:  %x",pt_context->s2,pt_context->s3);
    kernel_printf("s4:  %x,  s5:  %x",pt_context->s4,pt_context->s5);
    kernel_printf("s6:  %x,  s7:  %x\n",pt_context->s6,pt_context->s7);
    kernel_printf("t8:  %x,  t9:  %x\n",pt_context->t8,pt_context->t9);
    kernel_printf("hi:  %x,  lo:  %x\n",pt_context->hi,pt_context->lo);
    kernel_printf("gp:  %x\n",pt_context->gp);
    kernel_printf("sp:  %x\n",pt_context->sp);
    kernel_printf("fp:  %x\n",pt_context->fp);
    kernel_printf("ra:  %x\n",pt_context->ra);
 
}

void sched_example()
{
    kernel_printf("example begin\n");
    list_pcb_add_tail(current,&high_list);
    list_pcb *pos;
    for(pos=high_list.next;pos!=&high_list;pos=pos->next)
        kernel_printf("pid: %d,  name:  %s",pos->pcb->asid,pos->pcb->name);
    kernel_printf("stop\n");
    while(1);
    current=get_first_task(&high_list);

}
/*
void test_sched()
{
    task_union* proc1=( task_union*)kmalloc(PAGE_SIZE);
    task_union* proc2=( task_union*)kmalloc(PAGE_SIZE);

    /*kernel_strcpy(proc1->pcb.name, "print_0");
    kernel_strcpy(proc2->pcb.name, "print_2");

    proc1->pcb.context=(context*)((unsigned int)proc1+sizeof(PCB));
    proc2->pcb.context=(context*)((unsigned int)proc2+sizeof(PCB));

    clean_context(proc1->pcb.context);
    clean_context(proc2->pcb.context);

    proc1->pcb.counter=HIGH_TIMESLICES;
    proc2->pcb.counter=HIGH_TIMESLICES;

    proc1->pcb.priority=HIGH_PRIORITY;
    proc2->pcb.priority=ABOVE_NORMAL_PRIORITY;
	
    proc1->pcb.asid = (unsigned char)66;
    proc2->pcb.asid = (unsigned char)77;

    proc1->pcb.state=STATE_READY;
    proc2->pcb.state=STATE_READY;

    proc1->pcb.context->a0=1;
    proc2->pcb.context->a1=0;
    //proc1->pcb.context->epc=(unsigned int)(&print_0_fun);
    //proc2->pcb.context->epc=(unsigned int)(&print_2_fun);

    add_task(&(proc1->pcb.process));
    add_task(&(proc2->pcb.process));

    INIT_LIST_PCB(&proc1->pcb.sched,&(proc1->pcb));
    INIT_LIST_PCB(&proc1->pcb.process,&(proc1->pcb));
    INIT_LIST_PCB(&proc2->pcb.sched,&(proc2->pcb));
    INIT_LIST_PCB(&proc2->pcb.process,&(proc2->pcb));

    //kernel_printf("stop\n");
    //while(1);

    //list_pcb_add_tail(&(proc1->pcb.process),&(proc1->pcb.sched));
    //list_pcb_add_tail(&(proc2->pcb.process),&(proc2->pcb.sched));

    //初始化上下文
    //init->pcb.context=(context*)(init+PAGE_SIZE-(sizeof(context)));
    proc1->pcb.context=(context*)((unsigned int)proc1+sizeof(PCB));
    clean_context(proc1->pcb.context);
    proc1->pcb.context->epc=(unsigned int)(print_0_fun);
    proc1->pcb.context->sp=(unsigned int)proc1+PAGE_SIZE;
    unsigned int init_gp;
    asm volatile("la %0, _gp\n\t" : "=r"(init_gp));
    proc1->pcb.context->gp=init_gp;
    proc1->pcb.asid=get_emptypid();
    if(proc1->pcb.asid<0){
        kernel_printf("failed to get right asid\n");   
        return;
    }
    proc1->pcb.pgd=(pgd_term*)kmalloc(PAGE_SIZE);//分配页目录空间
    if(proc1->pcb.pgd==NULL)
    {
        kernel_printf("failed to kmalloc space for pgd\n");
        return;
    }
    //初始化pgd每一项
    int i=0;
    for(i=0;i<PAGE_SIZE>>2;i++)
    {
        (proc1->pcb.pgd)[i]=0;
    }
    //设置pgd属性为默认属性——可写
    kernel_strcpy(proc1->pcb.name, "print_0");
    proc1->pcb.parent=0;//init没有父进程
    proc1->pcb.uid=0;
    proc1->pcb.counter=DEFAULT_TIMESLICES;
    proc1->pcb.start_time=0;//get_time();
    proc1->pcb.state=STATE_WAITTING;
    proc1->pcb.priority=HIGH_PRIORITY;//设置优先级为最低优先级
    proc1->pcb.policy=0;//暂未定义调度算法
    proc1->pcb.shm=NULL; //shared memory

    INIT_LIST_PCB(&proc1->pcb.sched,&(proc1->pcb));
    INIT_LIST_PCB(&proc1->pcb.process,&(proc1->pcb));
    //暂不考虑线程
    proc1->pcb.thread_head=NULL;
    proc1->pcb.num_thread=0;
    
    add_task(&(proc1->pcb.process));//添加到pcb链表中
    proc1->pcb.state=STATE_RUNNING;


    proc2->pcb.context=(context*)((unsigned int)proc2+sizeof(PCB));
    clean_context(proc2->pcb.context);
    proc2->pcb.context->epc=(unsigned int)(ps);
    proc2->pcb.context->sp=(unsigned int)proc2+PAGE_SIZE;
    asm volatile("la %0, _gp\n\t" : "=r"(init_gp));
    proc2->pcb.context->gp=init_gp;
    proc2->pcb.asid=get_emptypid();
    if(proc2->pcb.asid<0){
        kernel_printf("failed to get right asid\n");   
        return;
    }
    proc2->pcb.pgd=(pgd_term*)kmalloc(PAGE_SIZE);//分配页目录空间
    if(proc2->pcb.pgd==NULL)
    {
        kernel_printf("failed to kmalloc space for pgd\n");
        return;
    }
    //初始化pgd每一项
    for(i=0;i<PAGE_SIZE>>2;i++)
    {
        (proc2->pcb.pgd)[i]=0;
    }
    //设置pgd属性为默认属性——可写
    kernel_strcpy(proc2->pcb.name, "ps");
    proc2->pcb.parent=0;//init没有父进程
    proc2->pcb.uid=0;
    proc2->pcb.counter=DEFAULT_TIMESLICES;
    proc2->pcb.start_time=0;//get_time();
    proc2->pcb.state=STATE_WAITTING;
    proc2->pcb.priority=HIGH_PRIORITY;//设置优先级为最低优先级
    proc2->pcb.policy=0;//暂未定义调度算法
    proc2->pcb.shm=NULL; //shared memory

    INIT_LIST_PCB(&proc2->pcb.sched,&(proc2->pcb));
    INIT_LIST_PCB(&proc2->pcb.process,&(proc2->pcb));
    //暂不考虑线程
    proc2->pcb.thread_head=NULL;
    proc2->pcb.num_thread=0;
    
    add_task(&(proc2->pcb.process));//添加到pcb链表中
    proc2->pcb.state=STATE_RUNNING;

    // kernel_printf("stop\n");
    // while(1);
    //add_to_foreground_list(&(proc1->pcb.process));
    //current=&(proc1->pcb.process);
    //add_to_background_list(&(proc2->pcb.process));
    //add_to_background_list(&(proc1->pcb.process));
    list_pcb_add_tail(&(proc2->pcb.process),&high_list);
    //list_pcb_add_tail(&(proc1->pcb.process),&high_list);

    kernel_printf("current name :  %s\n",current->pcb->name);
    list_pcb *pos;
    for(pos=background_list.next;pos!=&background_list;pos=pos->next)
        kernel_printf("back name :  %s\n",pos->pcb->name);
    for(pos=high_list.next;pos!=&high_list;pos=pos->next)
        kernel_printf("fore name :  %s\n",pos->pcb->name);
    //while(1);
        
}
*/


void init_sched()
{//未完成
    kernel_printf("begin\n");
    //初始化各队列
    INIT_LIST_PCB(&background_list,NULL);
    INIT_LIST_PCB(&high_list,NULL);
    INIT_LIST_PCB(&above_normal_list,NULL);
    INIT_LIST_PCB(&normal_list,NULL);
    INIT_LIST_PCB(&below_normal_list,NULL);
    INIT_LIST_PCB(&idle_list,NULL);


    //#ifdef SCHED_DEBUG
    kernel_printf("List init complete!\n");
    //#endif
    
    //初始情况为前台队列进程
    flag=1;
    counter=FOREGROUNG_TIMESLICES;
    
    kernel_printf("Timeslices complete!\n");
     
    //把init进程放在后台队列中
    current=get_first_task(&pcbs); 
    //add_to_background_list(current);
    next_list=&background_list;

    kernel_printf("test sched begin!\n");
    // test_sched();
    kernel_printf("Add process complete!\n");
    //register_syscall(10, pc_kill_syscall);
    register_interrupt_handler(7, schedule);

    asm volatile(
        "li $v0, 100000000\n\t"
        "mtc0 $v0, $11\n\t"
        "mtc0 $zero, $9");
}

void schedule(unsigned int status, unsigned int cause, context* pt_context) {
    // Save context
    list_pcb *pos;
    //kernel_printf("copy context\n");
    copy_context(pt_context, current->pcb->context);
   *GPIO_SEG =counter;

    sched();

    //修改tlb中asid
    unsigned int new_Entryhi=0;
    new_Entryhi|=current->pcb->asid;
    asm volatile(
        "mtc0 %0,$10\n\t"
        :"=r"(new_Entryhi));   
    
    copy_context(current->pcb->context, pt_context);
    
    //kernel_printf("load context\n");
    //print_context(pt_context);
 
    //kernel_printf("name:  %s\n",current->pcb->name);
   asm volatile("mtc0 $zero, $9\n\t");
    //kernel_printf("set time\n");
}

//初始化队列
void init_list(list_pcb *list)
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

//取队列中的第一个进程,并将其从队列中删除
list_pcb *get_first_task(list_pcb *list)
{
    list_pcb *task;
    if(list->next==list)
        return NULL;
    task=list->next;
    __list_pcb_del(list,task->next);
    init_list(task);
    return task;
}


void add_to_foreground_list(list_pcb *task)
{
    list_pcb_add_tail(task,&high_list);
}

void add_to_background_list(list_pcb *task)
{
    list_pcb_add_tail(task,&background_list);
}

//后台进程调度
unsigned int background_sched()
{
    //kernel_printf("background sched\n");
    current=get_first_task(&background_list);
    current->pcb->counter=BACKGROUND_PER_TIMESLICES;
    current->pcb->state=STATE_RUNNING;
    if(current==NULL)
    {//后台队列为空，切换到前台状态
       flag==1; 
       counter=FOREGROUNG_TIMESLICES;
    }
    return 0;
}
// unsigned int background_sched()
// {//重新修改过，待调试
//     kernel_printf("background sched begin\n");
//     list_pcb *next;
//     list_pcb *old;

//     if(list_is_empty(&background_list))
//     {
//         kernel_printf("No process background\n");
//         //切换到前台状态
//         flag=1;
//         counter=FOREGROUNG_TIMESLICES;
//         kernel_printf("current state timeslices is %d\n",counter);
//         kernel_printf("current state is %d\n",flag);
//         goto background_sched_error;
//     }
    
//     next=get_first_task(&background_list);
//     kernel_printf("get next\n");

//     if(next!=current)
//     {
//         kernel_printf("next != current\n");
//         kernel_printf("next is %s\n",next->pcb->name);
//         //old=current;
//         current=next;
//         current->pcb->state=STATE_RUNNING;
//         //old->pcb->state=STATE_READY;
//         //old->pcb->counter=BACKGROUND_PER_TIMESLICES;
//         //init_pcb_list(current);    
//     }
//     else
//         current->pcb->counter=BACKGROUND_PER_TIMESLICES;
//     kernel_printf("current state timeslices is %d\n",current->pcb->counter);

//     return 0;

// background_sched_error:
//     return 1;  
// }

//前台进程调度
unsigned int foreground_sched()
{
    //kernel_printf("foreground sched\n");
    if(!list_is_empty(&high_list))
    {
        current=get_first_task(&high_list);
        current->pcb->counter=HIGH_TIMESLICES;
        current->pcb->state=STATE_RUNNING;
        if(current->pcb->priority==HIGH_PRIORITY)
        {
            next_list=&high_list;
        }
        else
        {
            next_list=&above_normal_list;
        }
        current->pcb->priority=ABOVE_NORMAL_PRIORITY;
    }
    else if(!list_is_empty(&above_normal_list))
    {
        current=get_first_task(&above_normal_list);
        current->pcb->counter=ABOVE_NORMAL_TIMESLICES;
        current->pcb->state=STATE_RUNNING;
        next_list=&normal_list;
        current->pcb->priority=NORMAL_PRIORITY;
    }
    else if(!list_is_empty(&normal_list))
    {
        current=get_first_task(&normal_list);
        current->pcb->counter=NORMAL_TIMESLICES;
        current->pcb->state=STATE_RUNNING;
        next_list=&below_normal_list;
        current->pcb->priority=BELOW_NORMAL_PRIORITY;
    }
    else if(!list_is_empty(&below_normal_list))
    {
        current=get_first_task(&below_normal_list);
        current->pcb->counter=BELOW_NORMAL_TIMESLICES;
        current->pcb->state=STATE_RUNNING;
        next_list=&idle_list;
        current->pcb->priority=IDLE_PRIORITY;
    }
    else if(!list_is_empty(&idle_list))
    {
        current=get_first_task(&idle_list);
        current->pcb->counter=IDLE_TIMESLICES;
        current->pcb->state=STATE_RUNNING;
        next_list=&idle_list;
        current->pcb->priority=IDLE_PRIORITY;
    }
    else
    {//前台队列为空，转换状态
        flag=0;
        counter=BACKGROUND_TIMESLICES;
    }

    list_pcb *pos;
    for(pos=high_list.next;pos!=&high_list;pos=pos->next)
        kernel_printf("high name :  %s\n",pos->pcb->name);
    for(pos=above_normal_list.next;pos!=&above_normal_list;pos=pos->next)
        kernel_printf("above_normal_list name :  %s\n",pos->pcb->name);

    return 0;
}

unsigned int sched()
{
    if(current==NULL)
    {
        if(flag==1)
        {
            foreground_sched();
        }
        else
        {
            background_sched();
        }
    }
    else
    {
        if((--counter)==0)
        {//前后台状态时间片用完
            if(flag==1)
            {
                //把current放入到适当的队列中，并且更新pcb中sched值
                list_pcb_add_tail(current,next_list);
                current->pcb->sched=*current; 
                current->pcb->state=STATE_READY;
                
                //切换状态，执行调度
                flag=0;
                counter=BACKGROUND_TIMESLICES;
                background_sched();
            }
            else
            {
                //将当前进程放回到后台调度队列尾
                list_pcb_add_tail(current,&background_list);
                current->pcb->sched=*current;
                current->pcb->state=STATE_READY;
                
                //切换状态，执行调度
                flag=1;
                counter=FOREGROUNG_TIMESLICES;
                foreground_sched();
            }
        }
        else if((--(current->pcb->counter))==0)
        {//程序时间片用完
            if(flag==1)
            {
                list_pcb_add_tail(current,next_list);
                current->pcb->sched=*current;
                current->pcb->state=STATE_READY;
                foreground_sched();
            }
            else{
                list_pcb_add_tail(current,&background_list);
                current->pcb->sched=*current;
                current->pcb->state=STATE_READY;
                background_sched();
            }
        }
    }

}
