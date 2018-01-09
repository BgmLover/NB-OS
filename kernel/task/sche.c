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
    while(1);
        
    // int i=0;
    // while(1)
    // {
    //     i++;
    //     if(i==200)
    //         break;
    //     kernel_printf(" 0 ");
    // }
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

void test_sched()
{
    task_union* proc1=( task_union*)kmalloc(PAGE_SIZE);
    task_union* proc2=( task_union*)kmalloc(PAGE_SIZE);


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
    proc1->pcb.pgd=(pgd_term*)kmalloc(PAGE_SIZE);//鍒嗛厤椤电洰褰曠┖闂?
    if(proc1->pcb.pgd==NULL)
    {
        kernel_printf("failed to kmalloc space for pgd\n");
        return;
    }
    //鍒濆鍖杙gd姣忎竴椤?
    int i=0;
    for(i=0;i<PAGE_SIZE>>2;i++)
    {
        (proc1->pcb.pgd)[i]=0;
    }
    //璁剧疆pgd灞炴€т负榛樿灞炴€р€斺€斿彲鍐?
    kernel_strcpy(proc1->pcb.name, "print_0");
    proc1->pcb.parent=0;//init娌℃湁鐖惰繘绋?
    proc1->pcb.uid=0;
    proc1->pcb.counter=DEFAULT_TIMESLICES;
    proc1->pcb.start_time=0;//get_time();
    proc1->pcb.state=STATE_WAITTING;
    proc1->pcb.priority=HIGH_PRIORITY;//璁剧疆浼樺厛绾т负鏈€浣庝紭鍏堢骇
    proc1->pcb.policy=0;//鏆傛湭瀹氫箟璋冨害绠楁硶
    proc1->pcb.shm=NULL; //shared memory

    INIT_LIST_PCB(&proc1->pcb.sched,&(proc1->pcb));
    INIT_LIST_PCB(&proc1->pcb.process,&(proc1->pcb));
    //鏆備笉鑰冭檻绾跨▼
    proc1->pcb.thread_head=NULL;
    proc1->pcb.num_thread=0;
    
    //add_task(&(proc1->pcb.process));//娣诲姞鍒皃cb閾捐〃涓?
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
    proc2->pcb.pgd=(pgd_term*)kmalloc(PAGE_SIZE);//鍒嗛厤椤电洰褰曠┖闂?
    if(proc2->pcb.pgd==NULL)
    {
        kernel_printf("failed to kmalloc space for pgd\n");
        return;
    }
    //鍒濆鍖杙gd姣忎竴椤?
    for(i=0;i<PAGE_SIZE>>2;i++)
    {
        (proc2->pcb.pgd)[i]=0;
    }
    //璁剧疆pgd灞炴€т负榛樿灞炴€р€斺€斿彲鍐?
    kernel_strcpy(proc2->pcb.name, "ps");
    proc2->pcb.parent=0;//init娌℃湁鐖惰繘绋?
    proc2->pcb.uid=0;
    proc2->pcb.counter=DEFAULT_TIMESLICES;
    proc2->pcb.start_time=0;//get_time();
    proc2->pcb.state=STATE_WAITTING;
    proc2->pcb.priority=HIGH_PRIORITY;//璁剧疆浼樺厛绾т负鏈€浣庝紭鍏堢骇
    proc2->pcb.policy=0;//鏆傛湭瀹氫箟璋冨害绠楁硶
    proc2->pcb.shm=NULL; //shared memory

    INIT_LIST_PCB(&proc2->pcb.sched,&(proc2->pcb));
    INIT_LIST_PCB(&proc2->pcb.process,&(proc2->pcb));
    //鏆備笉鑰冭檻绾跨▼
    proc2->pcb.thread_head=NULL;
    proc2->pcb.num_thread=0;
    
    add_task(&(proc2->pcb.process));//娣诲姞鍒皃cb閾捐〃涓?
    proc2->pcb.state=STATE_RUNNING;

    // kernel_printf("stop\n");
    // while(1);
    //add_to_foreground_list(&(proc1->pcb.process));
    //current=&(proc1->pcb.process);
    //add_to_background_list(&(proc2->pcb.process));
    //add_to_background_list(&(proc1->pcb.process));
    list_pcb_add_tail(&(proc2->pcb.process),&high_list);
    list_pcb_add_tail(&(proc1->pcb.process),&high_list);

    int pid3=exec("/seg.bin","1234");
    task_union *proc3=(task_union*)get_pcb_by_pid(pid3);
    kernel_printf("current name :  %s\n",current->pcb->name);
    list_pcb *pos;
    for(pos=background_list.next;pos!=&background_list;pos=pos->next)
        kernel_printf("back name :  %s\n",pos->pcb->name);
    for(pos=high_list.next;pos!=&high_list;pos=pos->next)
        kernel_printf("fore name :  %s\n",pos->pcb->name);
        
}



void init_sched()
{//鏈畬鎴?
    kernel_printf("begin\n");
    //鍒濆鍖栧悇闃熷垪
    INIT_LIST_PCB(&background_list,NULL);
    INIT_LIST_PCB(&high_list,NULL);
    INIT_LIST_PCB(&above_normal_list,NULL);
    INIT_LIST_PCB(&normal_list,NULL);
    INIT_LIST_PCB(&below_normal_list,NULL);
    INIT_LIST_PCB(&idle_list,NULL);


    //#ifdef SCHED_DEBUG
    kernel_printf("List init complete!\n");
    //#endif
    
    //鍒濆鎯呭喌涓哄墠鍙伴槦鍒楄繘绋?
    flag=1;
    counter=FOREGROUNG_TIMESLICES;
    
    kernel_printf("Timeslices complete!\n");
     
    //鎶奿nit杩涚▼鏀惧湪鍚庡彴闃熷垪涓?
    current=get_first_task(&pcbs); 
    //add_to_background_list(current);
    next_list=&background_list;

    kernel_printf("test sched begin!\n");
    test_sched();
    kernel_printf("Add process complete!\n");
    //register_syscall(10, pc_kill_syscall);
    register_interrupt_handler(7, schedule);

    asm volatile(
        "li $v0, 30000000\n\t"
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

    //淇敼tlb涓璦sid
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

//鍒濆鍖栭槦鍒?
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

//鍙栭槦鍒椾腑鐨勭涓€涓繘绋?骞跺皢鍏朵粠闃熷垪涓垹闄?
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

//鍚庡彴杩涚▼璋冨害
unsigned int background_sched()
{
    //kernel_printf("background sched\n");
    current=get_first_task(&background_list);
    current->pcb->counter=BACKGROUND_PER_TIMESLICES;
    current->pcb->state=STATE_RUNNING;
    if(current==NULL)
    {//鍚庡彴闃熷垪涓虹┖锛屽垏鎹㈠埌鍓嶅彴鐘舵€?
       flag==1; 
       counter=FOREGROUNG_TIMESLICES;
    }
    return 0;
}
// unsigned int background_sched()
// {//閲嶆柊淇敼杩囷紝寰呰皟璇?
//     kernel_printf("background sched begin\n");
//     list_pcb *next;
//     list_pcb *old;

//     if(list_is_empty(&background_list))
//     {
//         kernel_printf("No process background\n");
//         //鍒囨崲鍒板墠鍙扮姸鎬?
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

//鍓嶅彴杩涚▼璋冨害
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
    {//鍓嶅彴闃熷垪涓虹┖锛岃浆鎹㈢姸鎬?
        flag=0;
        counter=BACKGROUND_TIMESLICES;
    }

    list_pcb *pos;
    // for(pos=high_list.next;pos!=&high_list;pos=pos->next)
    //     kernel_printf("high name :  %s\n",pos->pcb->name);
    // for(pos=above_normal_list.next;pos!=&above_normal_list;pos=pos->next)
    //     kernel_printf("above_normal_list name :  %s\n",pos->pcb->name);

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
        {//鍓嶅悗鍙扮姸鎬佹椂闂寸墖鐢ㄥ畬
            if(flag==1)
            {
                //鎶奵urrent鏀惧叆鍒伴€傚綋鐨勯槦鍒椾腑锛屽苟涓旀洿鏂皃cb涓璼ched鍊?
                list_pcb_add_tail(current,next_list);
                current->pcb->sched=*current; 
                current->pcb->state=STATE_READY;
                
                //鍒囨崲鐘舵€侊紝鎵ц璋冨害
                flag=0;
                counter=BACKGROUND_TIMESLICES;
                background_sched();
            }
            else
            {
                //灏嗗綋鍓嶈繘绋嬫斁鍥炲埌鍚庡彴璋冨害闃熷垪灏?
                list_pcb_add_tail(current,&background_list);
                current->pcb->sched=*current;
                current->pcb->state=STATE_READY;
                
                //鍒囨崲鐘舵€侊紝鎵ц璋冨害
                flag=1;
                counter=FOREGROUNG_TIMESLICES;
                foreground_sched();
            }
        }
        else if((--(current->pcb->counter))==0)
        {//绋嬪簭鏃堕棿鐗囩敤瀹?
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

void print_procs()
{
    kernel_printf("pid    name    state\n");
    list_pcb *pos;
    // for(pos=pcbs.next;pos!=&pcbs;pos=pos->next)
    // {
    //     kernel_printf("%d     %s     %d\n",pos->pcb->asid,pos->pcb->name,pos->pcb->state);
    //     //while(1);
    // }

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

    kernel_printf("background\n");
    if(flag==0)
    {
        kernel_printf("%d     %s     %d\n",current->pcb->asid,current->pcb->name,current->pcb->state);
    }
    for(pos=background_list.next;pos!=&background_list;pos=pos->next)
    {
        kernel_printf("%d     %s     %d\n",pos->pcb->asid,pos->pcb->name,pos->pcb->state);
        //while(1);
    }
}