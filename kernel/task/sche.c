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
 
//判断前后台的时间片
int counter;
//前后台的标志1->前台  0->后台
int flag;
list_pcb *current=NULL;
//后台队列，固定时间片
list_pcb background_list;
//前台队列优先级排列,时间片
list_pcb high_list;
list_pcb above_normal_list;
list_pcb normal_list;
list_pcb below_normal_list;
list_pcb idle_list;
list_pcb test;

PCB* get_current_pcb()
{
    return current->pcb;
}

void print_0_fun()
{
    while(1)
    {
        kernel_printf(" 0 \n");
    }
}

void print_2_fun()
{
    while(1)
    {
        kernel_printf(" 2 \n");
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
    INIT_LIST_PCB(&proc2->pcb.process,&(proc2->pcb));*/

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


    add_to_foreground_list(&(proc1->pcb.process));
    //current=&(proc1->pcb.process);

   // add_to_foreground_list(&(proc2->pcb.process));
}



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
    //counter=BACKGROUND_TIMESLICES;
    
    kernel_printf("Timeslices complete!\n");
     
    current=get_first_task(&pcbs);
    //add_to_background_list(current);
    
    add_to_foreground_list(current);

    test_sched();
    kernel_printf("Add process complete!\n");
    //register_syscall(10, pc_kill_syscall);
    register_interrupt_handler(7, pc_schedule);

    asm volatile(
        "li $v0, 100000000\n\t"
        "mtc0 $v0, $11\n\t"
        "mtc0 $zero, $9");
}

void pc_schedule(unsigned int status, unsigned int cause, context* pt_context) {
    // Save context
    list_pcb *pos;
    kernel_printf("copy context\n");
    copy_context(pt_context, current->pcb->context);
    //kernel_printf("%x\n",pt_context->epc);
    //kernel_printf("%x\n",current->pcb->context->epc);
    //for(pos=high_list.next;pos!=&high_list;pos=pos->next)
        //kernel_printf("pid:%x   name:%s\n",pos->pcb->asid,pos->pcb->name);
    //kernel_printf("current state is %d\n",flag);
    //kernel_printf("current state timeslices is %d\n",counter);
    *GPIO_SEG =counter;
    sched();

    unsigned int new_Entryhi=0;
    new_Entryhi|=current->pcb->asid;
    asm volatile(
        "mtc0 %0,$10\n\t"
        :"=r"(new_Entryhi));
    
    kernel_printf("%x\n",pt_context->epc);
    copy_context(current->pcb->context, pt_context);
    kernel_printf("load context\n");
    print_context(pt_context);

    
    kernel_printf("%x\n",current->pcb->context->epc);
   asm volatile("mtc0 $zero, $9\n\t");
    //kernel_printf("set time\n");
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

//取队列中的第一个进程,并将其从队列中删除
list_pcb *get_first_task(list_pcb *head)
{
    list_pcb *task;

    if(head->next==head)
        return NULL;

    task=head->next;
    //kernel_printf("head->next is %s\n",task->pcb->name);
    if(task->next==head)
    {
        kernel_printf("task->next=head\n");
        //while(1);
    }
    __list_pcb_del(head,task->next);
    if(head->next==head)
    {
        kernel_printf("head list is empty\n");
        //while(1);
    }
    

    init_pcb_list(task);

    return task;
}

//将进程加到队列首
void insert_head(list_pcb *task,list_pcb *head)
{
    task->next=head->next;
    task->prev=head;
    head->next->prev=task;
    head->next=task;
}

//将进程加到就绪队列队尾
void insert_tail(list_pcb *task,list_pcb *head)
{
    task->next=head;
    task->prev=head->prev;
    head->prev->next=task;
    head->prev=task;
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
    kernel_printf("background sched begin\n");
    list_pcb *next;
    list_pcb *old;
    if(!current)
    {
        current->pcb->state=STATE_READY;
        current->pcb->counter=BACKGROUND_PER_TIMESLICES;
        add_to_background_list(current);
    }

    // current->pcb->state=STATE_READY;
    // current->pcb->counter=BACKGROUND_PER_TIMESLICES;
    // add_to_background_list(current);

    if(list_is_empty(&background_list))
    {
        kernel_printf("No process background\n");
        //切换到前台状态
        counter=FOREGROUNG_TIMESLICES;
        flag=1;

        kernel_printf("current state timeslices is %d\n",counter);
        kernel_printf("current state is %d\n",flag);
        goto background_sched_error;
    }
    
    next=get_first_task(&background_list);
    kernel_printf("get next\n");

    if(next!=current)
    {
        kernel_printf("next != current\n");
        kernel_printf("next is %s\n",next->pcb->name);
        old=current;
        current=next;
        current->pcb->state=STATE_RUNNING;
        old->pcb->state=STATE_READY;
        old->pcb->counter=BACKGROUND_PER_TIMESLICES;
        init_pcb_list(current);    
    }
    else
        current->pcb->counter=BACKGROUND_PER_TIMESLICES;
    kernel_printf("current state timeslices is %d\n",current->pcb->counter);

    return 0;

background_sched_error:
    return 1;  
}

//前台进程调度
unsigned int foreground_sched()
{
    kernel_printf("foreground sched begin\n");

    list_pcb *next;
    list_pcb *old;
    int lower_priority;
    int timeslices;
    list_pcb *lower_list;

    if(!list_is_empty(&high_list))
    {
        kernel_printf("high list is not empty\n");
        next=get_first_task(&high_list);
        if(current->pcb->priority==HIGH_PRIORITY)
        {
            timeslices=HIGH_TIMESLICES;
            lower_list=&high_list;
            list_pcb_add_tail(current,&high_list);
        }
        else 
        {
            timeslices=ABOVE_NORMAL_TIMESLICES;
            lower_list=&above_normal_list;
            list_pcb_add_tail(current,&above_normal_list);
        }
        lower_priority=ABOVE_NORMAL_PRIORITY;
        //kernel_printf("next process is %s\n",next->pcb->name);
    }else if(!list_is_empty(&above_normal_list))
    {
        kernel_printf("above normal list is not empty\n");
        next=get_first_task(&above_normal_list);
        lower_priority=NORMAL_PRIORITY;
        timeslices=NORMAL_TIMESLICES;
        list_pcb_add_tail(current,&normal_list);
       // kernel_printf("next process is %s\n",next->pcb->name);
        //kernel_printf("next list contain %s\n",normal_list.next->pcb->name);
    }else if(!list_is_empty(&normal_list))
    {
        kernel_printf("normal list is not empty\n");
        next=get_first_task(&normal_list);
        lower_priority=BELOW_NORMAL_PRIORITY;
        timeslices=BELOW_NORMAL_TIMESLICES;
        lower_list=&below_normal_list;
        list_pcb_add_tail(current,&below_normal_list);
    }else if(!list_is_empty(&below_normal_list))
    {
        kernel_printf("below normal list is not empty\n");
        next=get_first_task(&below_normal_list);
        lower_priority=IDLE_PRIORITY;
        timeslices=IDLE_TIMESLICES;
        lower_list=&idle_list;
        list_pcb_add_tail(current,&idle_list);
    }else if(!list_is_empty(&idle_list))
    {
        kernel_printf("idle list is not empty\n");
        next=get_first_task(&idle_list);
        lower_priority=IDLE_PRIORITY;
        timeslices=IDLE_TIMESLICES;
        lower_list=&idle_list;
        list_pcb_add_tail(current,&idle_list);
    }else 
    {
        kernel_printf("No process foreground\n");
        flag=0;
        counter=BACKGROUND_TIMESLICES;
        //current->pcb->counter=BACKGROUND_PER_TIMESLICES;
        return 1;
    }

    old=current;
    current=next;
    current->pcb->state=STATE_RUNNING;
    old->pcb->state=STATE_READY;
    old->pcb->priority=lower_priority;
    old->pcb->counter=timeslices;
    //list_pcb_add_tail(old,lower_list);
    //init_pcb_list(current);


    kernel_printf("              current process name is                 %s\n",current->pcb->name);
    //#ifdef SCHED_DEBUG
    kernel_printf("switch to %d list\n",lower_priority);
    kernel_printf("current timeslices is %d\n",timeslices);
    //#endif

    return 0;

}

unsigned int sched()
{
    int index;
    kernel_printf("sched begin\n");
    if(--counter==0)
    {
        kernel_printf("state timeslices used up!\n");
        if(flag)
        {
            flag=0;

            kernel_printf("change to %d state\n",flag);

            counter=BACKGROUND_TIMESLICES;
            //执行后台调度
            //index=background_sched();
            if(background_sched())
                goto sched_error;
        }
        else
        {
            flag=1;

            kernel_printf("change to %d state\n",flag);

            counter=FOREGROUNG_TIMESLICES;
            //取出前台队列中优先级最高队列的第一个进程
            if(foreground_sched())
                goto sched_error;
        }
        
    }
    else
    {
        kernel_printf("state timeslice has not used up,rest %d\n",counter);
        if(--(current->pcb->counter))
        {
            kernel_printf("task timeslice has not used up,rest %d\n",current->pcb->counter);
            goto sched_ok;
        }
            
        else
        {
            kernel_printf("task timeslice has used up\n");
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
