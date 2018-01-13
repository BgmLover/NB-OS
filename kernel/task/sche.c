#include "sche.h"
#include <intr.h>
#include <zjunix/task.h>
#include <zjunix/time.h>
#include <driver/vga.h>
#include <zjunix/slub.h>
#include <zjunix/shm.h>
#include <zjunix/list_pcb.h>
#include <zjunix/syscall.h>
#include <zjunix/utils.h>
#include <zjunix/shm.h>
#include <debug.h>
#include <arch.h>
#include <../usr/ps.h>

list_pcb *current=NULL;
extern list_pcb pcbs;

//获取当前进程pcb
PCB* get_current_pcb()
{
    return current->pcb;
}

void print_0_fun()
{
    while(1);
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

//打印上下文
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
{//进程调度测试
    task_union* proc1=create_task_union();
    task_union* proc2=create_task_union();

    proc1->pcb.context->epc=(unsigned int)(print_0_fun);
    kernel_strcpy(proc1->pcb.name, "print_0");
    add_task(&(proc1->pcb.process));


    proc2->pcb.context->epc=(unsigned int)(ps);
    kernel_strcpy(proc2->pcb.name, "ps");
    add_task(&(proc2->pcb.process));

    list_pcb_add_tail(&(proc1->pcb.sched),&high_list);
    list_pcb_add_tail(&(proc2->pcb.sched),&high_list);    
}



void init_sched()
{
    kernel_printf("begin\n");
    //初始化各队列
    INIT_LIST_PCB(&background_list,NULL);
    INIT_LIST_PCB(&high_list,NULL);
    INIT_LIST_PCB(&above_normal_list,NULL);
    INIT_LIST_PCB(&normal_list,NULL);
    INIT_LIST_PCB(&below_normal_list,NULL);
    INIT_LIST_PCB(&idle_list,NULL);

    //初始情况为前台队列进
    flag=1;
    counter=FOREGROUNG_TIMESLICES;
    
    kernel_printf("Timeslices complete!\n");
     
    //把init作为初始情况current进程
    current=&(pcbs.next->pcb->sched);
    //当前进程将下次调度是会放入后台队列
    next_list=&background_list;

    kernel_printf("test sched begin!\n");
    test_sched();
    kernel_printf("Add process complete!\n");
    //注册调度中断
    register_interrupt_handler(7, schedule);

    //计时器初值
    asm volatile(
        "li $v0, 10000000\n\t"
        "mtc0 $v0, $11\n\t"
        "mtc0 $zero, $9");
}

//调度中断处理函数
void schedule(unsigned int status, unsigned int cause, context* pt_context) {
    //保存上下文
    list_pcb *pos;
    copy_context(pt_context, current->pcb->context);

    sched();

    //修改tlb中asid
    unsigned int new_Entryhi=0;
    new_Entryhi|=current->pcb->asid;
    asm volatile(
        "mtc0 %0,$10\n\t"
        :"=r"(new_Entryhi));   
    
    //恢复上下文
    copy_context(current->pcb->context, pt_context);
    //计时器寄存器重新赋值
    asm volatile("mtc0 $zero, $9\n\t");
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

//取队列中的第一个进程，并将其从队列中删除
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

list_pcb *get_init_task(list_pcb *list)
{
    list_pcb *task;
    if(list->next==list)
        return NULL;
    task=list->next;
    init_list(task);
    return task;
}

//加入到前台队列
void add_to_foreground_list(list_pcb *task)
{
    list_pcb_add_tail(task,&high_list);
}

//加入到后台队列
void add_to_background_list(list_pcb *task)
{
    list_pcb_add_tail(task,&background_list);
}

//后台进程调度
unsigned int background_sched()
{
    current=get_first_task(&background_list);
    if(current==NULL)
    {//后台队列为空，切换到前台状
       flag==1; 
       counter=FOREGROUNG_TIMESLICES;
    }
    else{
        current->pcb->counter=BACKGROUND_PER_TIMESLICES;
        current->pcb->state=STATE_RUNNING;
    }

    return 0;
}

//前台进程调度
unsigned int foreground_sched()
{
    //从优先级最高的队列开始判断是否为空，若为空则判断下一个队列，否则在当前队列中取出队首的进程执行
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

    return 0;
}

//进程调度主控函数，负责判断前后台，将当前进程放入到合适的队列并且选取不同调度方案
unsigned int sched()
{
    if(current==NULL)
    {//如果当前进程为空，则判断状态，使用合适的调度
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
                //把current放入到适当的队列中，并且更新pcb中sched
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
                //将当前进程放回到后台调度队列
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

//打印进程信息
void print_procs()
{
    kernel_printf("pid    name    state\n");
    list_pcb *pos;

    //打印前台调度队列中的进程
    kernel_printf("foreground\n");
    if(flag==1)
    {
        kernel_printf("%d     %s     %d\n",current->pcb->asid,current->pcb->name,current->pcb->state);
    }
    if(!list_is_empty(&high_list))
    {
        for(pos=high_list.next;pos!=&high_list;pos=pos->next)
        {
            kernel_printf("%d     %s     %d\n",pos->pcb->asid,pos->pcb->name,pos->pcb->state);
            //while(1);
        }
    }
    else if(!list_is_empty(&above_normal_list))
    {
        for(pos=above_normal_list.next;pos!=&above_normal_list;pos=pos->next)
        {
            kernel_printf("%d     %s     %d\n",pos->pcb->asid,pos->pcb->name,pos->pcb->state);
            //while(1);
        }
    }
    else if(!list_is_empty(&normal_list))
    {
        for(pos=normal_list.next;pos!=&normal_list;pos=pos->next)
        {
            kernel_printf("%d     %s     %d\n",pos->pcb->asid,pos->pcb->name,pos->pcb->state);
            //while(1);
        }
    }
    else if(!list_is_empty(&below_normal_list))
    {
        for(pos=below_normal_list.next;pos!=&below_normal_list;pos=pos->next)
        {
            kernel_printf("%d     %s     %d\n",pos->pcb->asid,pos->pcb->name,pos->pcb->state);
            //while(1);
        }
    }
    else if(!list_is_empty(&idle_list))
    {
        for(pos=idle_list.next;pos!=&idle_list;pos=pos->next)
        {
            kernel_printf("%d     %s     %d\n",pos->pcb->asid,pos->pcb->name,pos->pcb->state);
            //while(1);
        }
    }

    //打印后台调度队列中的进程
    kernel_printf("background\n");
    if(flag==0)
    {
        kernel_printf("%d     %s     %d\n",current->pcb->asid,current->pcb->name,current->pcb->state);
    }
    for(pos=background_list.next;pos!=&background_list;pos=pos->next)
    {
        kernel_printf("%d     %s     %d\n",pos->pcb->asid,pos->pcb->name,pos->pcb->state);
    }
}

//创建时间进程
void creat_time()
{
    task_union* time_proc=create_task_union();

    time_proc->pcb.context->epc=(unsigned int)(system_time_proc);
    kernel_strcpy(time_proc->pcb.name, "time_proc");
    add_task(&(time_proc->pcb.process));

    //加入到后台调度队列
    add_to_background_list(&(time_proc->pcb.sched));
}

//生产者函数
void producer(){
    char data;
    unsigned int offset = 0;
    struct shared_memory* shm;
    PCB* producer_pcb = get_current_pcb();
    kernel_printf("pid:%d\n",producer_pcb->asid);
    kernel_printf("PCB:%x\n",(unsigned int)producer_pcb);
    // while(1);

	shm = shm_get(producer_pcb);
    kernel_printf("shm=%x\n",(unsigned int)(producer_pcb->shm));

    // write 26 letters
    data='a';
    
    for(data = 'a';data<='z';data++){
        shm_write(producer_pcb,offset,data);
        offset++;
        kernel_printf("producer write:%c\n",data);
    }
    
}

//消费者函数
void customer(){
    char data;
    unsigned int offset=0;
    int flag;
    int i;
    PCB* customer_pcb = get_current_pcb();
    shm_mount(4,customer_pcb);


    for(i = 0; i<30; i++){
        data = shm_read(customer_pcb, offset);
        
        if(data!=0){
            offset++;
            kernel_printf("customer read:%c\n", data);
        }
        else{
            kernel_printf("Shm empty!\n");
        }
        
    }
    
}

//生产者消费者demo
void demo()
{
    //创建生产者和消费者
    task_union* proc1=create_task_union();
    task_union* proc2=create_task_union();

    proc1->pcb.context->epc=(unsigned int)(producer);
    kernel_strcpy(proc1->pcb.name, "producer");
    add_task(&(proc1->pcb.process));


    proc2->pcb.context->epc=(unsigned int)(customer);
    kernel_strcpy(proc2->pcb.name, "customer");
    add_task(&(proc2->pcb.process));

    list_pcb_add_tail(&(proc1->pcb.sched),&high_list);
    list_pcb_add_tail(&(proc2->pcb.sched),&high_list);
}