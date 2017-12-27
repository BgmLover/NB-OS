#include "task.h"
#include <zjunix/utils.h>
#include <arch.h>
#include <driver/vga.h>
#include <zjunix/time.h>
#include <zjunix/slub.h>
#include <zjunix/list_pcb.h>
#include <zjunix/buddy.h>
#include <zjunix/bootmem.h>
#include <debug.h>
#include <page.h>
#include <zjunix/shm.h>
extern struct page *pages;
list_pcb pcbs;//进程队列
unsigned char idmap[32];//设置256个进程id
unsigned char bits_map[8]={1,2,4,8,16,32,64,128};

void task_test()
{
/*test shared memory*/
    struct shared_memory* shm;
    unsigned char p;
     task_union* proc1=( task_union*)kmalloc(PAGE_SIZE);
     task_union* proc2=( task_union*)kmalloc(PAGE_SIZE);
	
    proc1->pcb.asid = (unsigned char)66;
    proc2->pcb.asid = (unsigned char)77;
    proc1->pcb.shm=NULL;
    proc2->pcb.shm=NULL;
    // shm_init();
    shm=shm_get(4096);
    shm_mount(&proc1->pcb, shm);
    shm_mount(&proc2->pcb, shm);
    shm_write(&proc1->pcb, 0, 'f');
    p = shm_read(&proc2->pcb, 0);
    kernel_printf("shm:%c\n", p);

add_task(&proc1->pcb.process);
add_task(&proc2->pcb.process);
/*end test shared memory*/


    kernel_printf("begin to test\n");
    unsigned int entry0,entry1,entryhi,index;
    asm volatile(
        "mfc0 %0, $2\n\t"
        "mfc0 %1, $3\n\t"
        "mfc0 %2, $10\n\t"
        "mfc0 %3, $0\n\t"
        "nop\n\t"
        "nop\n\t"
        :"=r"(entry0),"=r"(entry1),"=r"(entryhi),"=r"(index));
    kernel_printf("entry0:%x\n",entry0);
    kernel_printf("entry1:%x\n",entry1);
    kernel_printf("entryhi:%x\n",entryhi);
    kernel_printf("index:%x\n",index);
    // context *t;
    // kernel_printf("%s\n",pcbs.next->pcb->name);
    // kernel_printf("%x\n",pcbs.next->pcb);
    // kernel_printf("%x\n",pcbs.next->pcb->context);
    // t=(pcbs.next->pcb->context);
    
    // t->at=123;
    // kernel_printf("%d\n",t->at);
    // do_fork(t,pcbs.next->pcb);
    // kernel_printf("%x\n",pcbs.next->next->pcb->context->at);
    // list_pcb *pos;
    // for(pos=pcbs.next;pos!=&pcbs;pos=pos->next)
    //     kernel_printf("pid:%x   name:%s\n",pos->pcb->asid,pos->pcb->name);
    // del_task(0);
    // for(pos=pcbs.next;pos!=&pcbs;pos=pos->next)
    //     kernel_printf("pid:%x   name:%s\n",pos->pcb->asid,pos->pcb->name);
    // task_union *t[10];
    // int i;
    // for(i=0;i<10;i++)
    // {
    //     t[i]=(task_union *)umalloc(PAGE_SIZE);
    //     kernel_printf("t[%x]:%x\n",i,t[i]);
    // }
}
void init_task()
{
    int i=0;
    INIT_LIST_PCB(&pcbs,NULL);  
    
    kernel_memset(idmap,0,16*sizeof(unsigned char));//初始化进程位图
    task_union *init;
    #ifdef TASK_DEBUG_INIT
    kernel_printf("task_union get start\n");
    #endif
    init=(task_union*)kmalloc(PAGE_SIZE);//init进程的地址
    if(!init){
        kernel_printf("failed to get space for init\n");
        return;
    }
    #ifdef TASK_DEBUG_INIT
    kernel_printf("task_union get\n");
    #endif

    //初始化上下文
    //init->pcb.context=(context*)(init+PAGE_SIZE-(sizeof(context)));
    init->pcb.context=(context*)((unsigned int)init+sizeof(PCB));
    // kernel_printf("address of init:%x\n",init);
    // kernel_printf("size of PCB:%x\n",sizeof(PCB));
    // kernel_printf("address of context:%x\n",init->pcb.context);
    //init->pcb.context->at=15;
    //init分配进程号为0
    init->pcb.asid=get_emptypid();
    if(init->pcb.asid<0){
        kernel_printf("failed to get right asid\n");   
        return;
    }
    #ifdef TASK_DEBUG_INIT
    kernel_printf("pid number %x get\n",init->pcb.asid);
    #endif
    init->pcb.pgd=(pgd_term*)kmalloc(PAGE_SIZE);//分配页目录空间
    if(init->pcb.pgd==NULL)
    {
        kernel_printf("failed to kmalloc space for pgd\n");
        return;
    }
    //初始化pgd每一项
    for(i=0;i<PAGE_SIZE>>2;i++)
    {
        (init->pcb.pgd)[i]=0;
    }
    //设置pgd属性为默认属性——可写
    //set_pgd_attr(init->pcb.pgd,Default_attr);
    #ifdef TASK_DEBUG_INIT
    kernel_printf("pgd address:%x\n",init->pcb.pgd);
    #endif
    kernel_strcpy(init->pcb.name, "init");
    init->pcb.parent=0;//init没有父进程
    init->pcb.uid=0;
    init->pcb.counter=DEFAULT_TIMESLICES;
    init->pcb.start_time=0;//get_time();
    init->pcb.state=STATE_WAITTING;
    init->pcb.priority=IDLE_PRIORITY;//设置优先级为最低优先级
    init->pcb.policy=0;//暂未定义调度算法
    init->pcb.shm=NULL; //shared memory

    INIT_LIST_PCB(&init->pcb.sched,&(init->pcb));
    INIT_LIST_PCB(&init->pcb.process,&(init->pcb));
    //暂不考虑线程
   #ifdef TASK_DEBUG_INIT
    kernel_printf("init_list_pcb over\n");
   #endif
    init->pcb.thread_head=NULL;
    init->pcb.num_thread=0;
    
    add_task(&(init->pcb.process));//添加到pcb链表中
    /*
    注册中断和系统调用
    */
    init->pcb.state=STATE_RUNNING;
    #ifdef TASK_DEBUG_INIT
    kernel_printf("init_proc created successfully\n");
    kernel_printf("Proc name:%s\n",init->pcb.name);
    kernel_printf("pid:%d\n",init->pcb.asid);
    kernel_printf("Address: %x\n",init);
    #endif
}

void copy_context(context* src, context* dest) 
{
   
    dest->epc = src->epc;
    dest->at = src->at;
    dest->v0 = src->v0;
    dest->v1 = src->v1;
    dest->a0 = src->a0;
    dest->a1 = src->a1;
    dest->a2 = src->a2;
    dest->a3 = src->a3;
    dest->t0 = src->t0;
    dest->t1 = src->t1;
    dest->t2 = src->t2;
    dest->t3 = src->t3;
    dest->t4 = src->t4;
    dest->t5 = src->t5;
    dest->t6 = src->t6;
    dest->t7 = src->t7;
    dest->s0 = src->s0;
    dest->s1 = src->s1;
    dest->s2 = src->s2;
    dest->s3 = src->s3;
    dest->s4 = src->s4;
    dest->s5 = src->s5;
    dest->s6 = src->s6;
    dest->s7 = src->s7;
    dest->t8 = src->t8;
    dest->t9 = src->t9;
    dest->hi = src->hi;
    dest->lo = src->lo;
    dest->gp = src->gp;
    dest->sp = src->sp;
    dest->fp = src->fp;
    dest->ra = src->ra;
}

unsigned char get_emptypid()
{
    unsigned char number=0;
    unsigned char temp;
    unsigned int index,bits;
    for(index=0;index<16;index++)
    {
        temp=idmap[index];
        for(bits=0;bits<sizeof(unsigned char);bits++)
        {
            if(!(temp&0x01)){
                idmap[index]|=bits_map[bits];
                break;
            }
            temp>>=1;
            number++;
        }
        if(bits<sizeof(unsigned char))
            break;
    }

    return number;
}
void free_pid(unsigned int pid)
{
    unsigned int index=pid/8;
    unsigned int bit_index=pid%8;
    idmap[index]&=(~bits_map[bit_index]);
}
//把一个进程加到进程队列末尾
void add_task(list_pcb* process)
{
    list_pcb_add(process,&pcbs);
}



unsigned int do_fork(context* args,PCB*parent)
{
    #ifdef DO_FORK_DEBUG
    kernel_printf("begin to fork\n");
    #endif

    task_union *new;
    new=(task_union*)kmalloc(sizeof(task_union));
    if(new==NULL)
    {
        kernel_printf("error : failed to allocate space for task_union\n");
        goto error1;
    }
    #ifdef DO_FORK_DEBUG
    kernel_printf("address of new task_union %x\n",new);
    #endif
    //复制上下文
    new->pcb.context=(context*)((unsigned int)new+sizeof(PCB));
    copy_context(args,new->pcb.context);
    
    #ifdef DO_FORK_DEBUG
    kernel_printf("old context->at=%x\n",args->at);
    kernel_printf("new context->at=%x\n",new->pcb.context->at);
    kernel_printf("copy context over\n");
    #endif
    //复制页表
    new->pcb.pgd=copy_pagetables(&(new->pcb),parent);
    if(new->pcb.pgd==NULL)
    {
        kernel_printf("error : failed to copy pages\n");
        goto error2;
    }
    #ifdef DO_FORK_DEBUG
    kernel_printf("copy pagetables over\n");
    #endif
    //复制或是设置新的PCB信息
    kernel_memcpy(new->pcb.name,parent->name,sizeof(char)*32);
    new->pcb.asid=get_emptypid();
    if(new->pcb.asid>255)
    {
        kernel_printf("error : no more pid to allocate\n");
        goto error3;
    }
    new->pcb.parent=parent->asid;
    new->pcb.uid=parent->uid;
    new->pcb.counter=parent->counter;
    new->pcb.start_time=0;//这里记得去完善time函数
    new->pcb.priority=parent->priority;
    new->pcb.policy=parent->priority;

    INIT_LIST_PCB(&(new->pcb.sched),&new->pcb);
    INIT_LIST_PCB(&(new->pcb.process),&new->pcb);

    new->pcb.thread_head=NULL;
    new->pcb.num_thread=0;
    new->pcb.shm=NULL; // shared memory

    new->pcb.file=parent->file;

    new->pcb.state=STATE_READY;
    //添加到进程队列中
    add_task(&(new->pcb.process));
    /*
    加入调度队列
    */
    #ifdef DO_FORK_DEBUG
    kernel_printf("child 's name:%s\n",new->pcb.name);
    kernel_printf("child 's pid : %x\n",new->pcb.asid);
    #endif
    return 0;
    

    error1:
        return 1;
    error2:
        kfree(new);
        return 2;
    error3:
        kfree(new->pcb.pgd);
        kfree(new);
        return 3;
}
 


void inc_refrence_by_pte( unsigned int *pte)
{
	 unsigned int index;

	for (index = 0; index < (PAGE_SIZE >> 2); ++index) {
		if (is_V(&(pte[index]))) {
			inc_ref(pages + (pte[index] >> PAGE_SHIFT), 1);
		}
	}
}

void dec_refrence_by_pte(unsigned int *pte)
{
	unsigned int index;
	for (index = 0; index < (PAGE_SIZE >> 2); ++index) {
		if (pte[index]) {
            //物理页地址
            unsigned int phy_addr=pte[index]&(~OFFSET_MASK);
            struct page *phy_page=pages+(phy_addr>>PAGE_SHIFT);
            //引用次数--
            dec_ref(phy_page,1);
            //如果引用次数为0，则将该页free掉
            if(phy_page->reference==0)
                kfree((void*)phy_addr);
			pte[index] = 0;
		}
	}
}

pgd_term *copy_pagetables(PCB* child,PCB* parent)
{
    pgd_term* old_pgd;
    pgd_term* new_pgd=NULL;
    pte_term* temp_pte;
    pte_term* old_pte;
    unsigned int index,ip;//索引
    unsigned int count=0;//记录pgd中已经成功分配的页数

    //分配一张新的页作为页目录
    old_pgd=parent->pgd;
    new_pgd=(pgd_term*)kmalloc(PAGE_SIZE);
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("new pgd address:%x\n",new_pgd);
    #endif
    if(new_pgd==NULL)
    {
        kernel_printf("copy_pagetables failed : failed to malloc for pgd\n");
        goto error1;
    }
    kernel_memcpy(new_pgd,old_pgd,PAGE_SIZE);
    for(index=0;index<KERNEL_ENTRY>>PGD_SHIFT;index++)
    {
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("pgd index:%x\n",index);
    #endif
        if(old_pgd[index]){
            temp_pte=(pte_term*)kmalloc(PAGE_SIZE);
            if(!temp_pte){
                kernel_printf("copy_pagetables failed : failed to malloc for pgte\n");
                goto error2;
            }
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("new_pte_addr=%x\n",temp_pte);
    #endif
            count++;
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("count:%x\n",count);
    kernel_printf("old_pgd[%x]=%x\n",index,old_pgd[index]);
    #endif
            //将新的pgd的每一项pte设置为新分配的pte页地址
            new_pgd[index] &= OFFSET_MASK;
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("after &=mask: new_pgd[%x]=%x\n",index,new_pgd[index]);
    #endif
            new_pgd[index] |= (unsigned int)temp_pte;
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("new_pgd[%x]=%x\n",index,new_pgd[index]);
    #endif
            old_pte=(pte_term*) (old_pgd[index]&(~OFFSET_MASK));
            kernel_memcpy(temp_pte,old_pte,PAGE_SIZE);
            for(ip=0;ip<(PAGE_SIZE>>2);ip++)
            {
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("ip:%x\n",ip);
    #endif
                if(temp_pte[ip])
                {
                    unsigned int va;
                    va=index<<PGD_SHIFT;
                    va|=(ip<<PTE_SHIFT);
                    //新老页表现在都不能往这个地址上写
                    clean_W(&(old_pte[ip]));
                    clean_W(&(temp_pte[ip]));
                    tlbp(va,parent->asid);
                    unsigned int tlb_index=get_tlb_index();
                    if(tlb_index&(1<<31)==0)//在TLB里存在这项内容，需要将其修改
                    tlbwi(va,parent->asid,old_pte[ip],tlb_index);
                }
            }
        }

    }
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("copy pgd and pte over\n");
    #endif
    for(index=0;index<KERNEL_ENTRY>>PGD_SHIFT;index++){
        if(new_pgd[index])
        {
            //在物理内容中对应的页里增加引用次数
            inc_refrence_by_pte((pte_term*) (new_pgd[index]&(~OFFSET_MASK)));
        }
    }
    return new_pgd;

    error2:
        if(count){
            for(index=0;index<KERNEL_ENTRY>>PGD_SHIFT;index++)
            {
                if(count==0) break;
                if(new_pgd[index])
                {
                    temp_pte=(pte_term*) (new_pgd[index]&(~OFFSET_MASK));
                    old_pte= (pte_term*) ((old_pgd[index]&(~OFFSET_MASK)));
                    
                    if(old_pte==temp_pte){
                        kfree(temp_pte);
                        count--;
                    }
                }
            }     
        }
    error1:
        kfree(new_pgd);
        return NULL;
}


void delete_pages(PCB *task)
{
	pgd_term *pgd = task->pgd;
	pte_term *pte;
	unsigned int index;
	//只删去属于用户空间的页表
	for (index = 0; index < (KERNEL_ENTRY >> PGD_SHIFT); ++index) {
		if (pgd[index]) {
			pte =(pte_term*) (pgd[index] & (~OFFSET_MASK));
			dec_refrence_by_pte(pte);
			kfree(pte);
			pgd[index] = 0;
		}
	}
}

void delete_pagetables(PCB *task)
{
	delete_pages(task);
	kfree(task->pgd);
}
//把一个进程从进程队列中删去
unsigned int del_task(unsigned int pid)
{
    int index=0;
    list_pcb *pos;
    for(pos=pcbs.next;pos!=&pcbs;pos=pos->next)
    {
        if(pos->pcb->asid==pid)
        {
            PCB * task_to_del=pos->pcb;
            kfree(task_to_del->context);//删去上下文
            delete_pages(task_to_del);  //删去页表
            //delete task file待续      //删去文件信息
            free_pid(task_to_del->asid);      //释放进程号
            kfree((task_union*)task_to_del);//删去整个task_union
            list_pcb_del_init(pos);
            //在调度队列中删去它
            return 0;
        }
    }
    kernel_printf("process not found,pid %d",pid);
    return 1;
}


