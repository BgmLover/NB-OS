#include "task.h"
#include <zjunix/utils.h>
#include <zjunix/sche.h>
#include <zjunix/shm.h>
#include <arch.h>
#include <intr.h>
#include <driver/vga.h>
#include <zjunix/time.h>
#include <zjunix/slub.h>
#include <zjunix/list_pcb.h>
#include <zjunix/buddy.h>
#include <zjunix/bootmem.h>
#include <debug.h>
#include <page.h>

extern struct page *pages;      
list_pcb pcbs;                  //进程队列，连接所有PCB
unsigned char idmap[32];        //设置256个进程id
unsigned char bits_map[8]={1,2,4,8,16,32,64,128};


/*该函数为init进程指定执行主体*/
void init_code(){
    while(1);
}

/*用于创建一个task_union并对其进行基本的初始化*/
task_union *create_task_union(){
    task_union *new=(task_union*)kmalloc(PAGE_SIZE);            //分配空间
    if(!new){
        kernel_printf("failed to get space for init\n");
        return NULL;
    }
    /*初始化上下文*/
    new->pcb.context=(context*)((unsigned int)new+sizeof(PCB)); //把context放到PCB上
    clean_context(new->pcb.context);                            //初始化context
    new->pcb.context->sp=(unsigned int)new+PAGE_SIZE;           //设置栈指针
    unsigned int init_gp;
    asm volatile("la %0, _gp\n\t" : "=r"(init_gp));
    new->pcb.context->gp=init_gp;                               //设置全局指针
    new->pcb.asid=get_emptypid();                               //分配进程号
    if(new->pcb.asid<0){
        kernel_printf("failed to get right asid\n");   
        return NULL;
    }
    new->pcb.pgd=(pgd_term*)kmalloc(PAGE_SIZE);                 //分配页目录空间
    if(new->pcb.pgd==NULL)
    {
        kernel_printf("failed to kmalloc space for pgd\n");
        return NULL;
    }
    clean_page(new->pcb.pgd);                                   //初始化页目录
    /*初始化PCB链表节点和调度队列节点*/
    INIT_LIST_PCB(&new->pcb.sched,&(new->pcb));
    INIT_LIST_PCB(&new->pcb.process,&(new->pcb));

    /*对其它属性的初始化*/
    new->pcb.parent=0;
    new->pcb.counter=DEFAULT_TIMESLICES;
    new->pcb.state=STATE_WAITTING;
    new->pcb.priority=IDLE_PRIORITY;                           //设置优先级为最低优先级
    new->pcb.shm=NULL;             
    new->pcb.file=NULL;
}
/*init进程，主要用于初始化进程模块，创建init进程（代码中附带着相关debug代码）*/
void init_task()
{
    INIT_LIST_PCB(&pcbs,NULL);                          //初始化pcbs双向链表表头
    kernel_memset(idmap,0,32*sizeof(unsigned char));    //初始化进程位图
    idmap[0]|=1;                                        //进程不使用idmap的 0号id
    #ifdef TASK_DEBUG_INIT
    kernel_printf("task_union get start\n");
    #endif

    task_union *init=create_task_union();               //创建init进程的task_union

    #ifdef TASK_DEBUG_INIT
    kernel_printf("pid number %x get\n",init->pcb.asid);
    kernel_printf("pgd address:%x\n",init->pcb.pgd);
    #endif

    kernel_strcpy(init->pcb.name, "init");    
    add_task(&(init->pcb.process));                     //添加到PCB链表中

    #ifdef TASK_DEBUG_INIT
    kernel_printf("init_proc created successfully\n");
    kernel_printf("Proc name:%s\n",init->pcb.name);
    kernel_printf("pid:%d\n",init->pcb.asid);
    kernel_printf("Address: %x\n",init);
    #endif
}

/*复制上下文*/
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
/*清空上下文*/
void clean_context(context* dest)
{
    dest->epc = 0;
    dest->at = 0;
    dest->v0 = 0;
    dest->v1 = 0;
    dest->a0 = 0;
    dest->a1 = 0;
    dest->a2 = 0;
    dest->a3 = 0;
    dest->t0 = 0;
    dest->t1 = 0;
    dest->t2 = 0;
    dest->t3 = 0;
    dest->t4 = 0;
    dest->t5 = 0;
    dest->t6 = 0;
    dest->t7 = 0;
    dest->s0 = 0;
    dest->s1 = 0;
    dest->s2 = 0;
    dest->s3 = 0;
    dest->s4 = 0;
    dest->s5 = 0;
    dest->s6 = 0;
    dest->s7 = 0;
    dest->t8 = 0;
    dest->t9 = 0;
    dest->hi = 0;
    dest->lo = 0;
    dest->gp = 0;
    dest->sp = 0;
    dest->fp = 0;
    dest->ra = 0;
}
/*获得一个未被使用的最小的pid*/
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
/*进程在被删除后需要把其占有的pid号释放出来*/
void free_pid(unsigned int pid)
{
    unsigned int index=pid/8;
    unsigned int bit_index=pid%8;
    idmap[index]&=(~bits_map[bit_index]);
}
/*遍历PCB链表，如果pid匹配，则返回PCB指针*/
PCB *get_pcb_by_pid(unsigned int pid){
    int index=0;
    list_pcb *pos;
    for(pos=pcbs.next;;pos=pos->next)
    {
        if(pos==&pcbs)
        {
            #ifdef PRINT_TASK_DEBUG
            kernel_printf("over\n");
            #endif
            return NULL;
        }
        else if(pos->pcb->asid==pid)
        {
            PCB * task=pos->pcb;
            return task;
        }
    }
}
/*将一个PCB链表节点加入到PCB链表最末尾*/
void add_task(list_pcb* process)
{
    list_pcb_add_tail(process,&pcbs);
}
/*fork，子进程复制父进程的相关属性信息*/
int do_fork(context* args,PCB*parent)
{
    #ifdef DO_FORK_DEBUG
    kernel_printf("begin to fork\n");
    #endif

    task_union *new=(task_union*)kmalloc(sizeof(task_union));           //分配空间

    if(new==NULL){
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
    #ifdef DO_FORK_DEBUG
    kernel_printf("copy name over\n");
    #endif   

    new->pcb.asid=get_emptypid();           //申请新的pid
    if(new->pcb.asid>255)
    {
        kernel_printf("error : no more pid to allocate\n");
        goto error3;
    }
    //复制文件信息
    if(parent->file==NULL)
        new->pcb.file=NULL;
    else{
        new->pcb.file=(FILE*)kmalloc(sizeof(FILE));
        if(new==NULL){
            kernel_printf("error in do_fork:failed to malloc for FILE\n");
            goto error4;
        }
        kernel_memcpy(new->pcb.file,parent->file,sizeof(FILE));
    }
    
    new->pcb.parent=parent->asid;
    new->pcb.counter=parent->counter;
    new->pcb.priority=parent->priority;

    //初始化节点
    INIT_LIST_PCB(&(new->pcb.sched),&new->pcb);
    INIT_LIST_PCB(&(new->pcb.process),&new->pcb);

    new->pcb.shm=NULL; 

    
    //添加到进程队列中
    add_task(&(new->pcb.process));
    add_to_foreground_list(&(new->pcb.sched));
    new->pcb.state=STATE_READY;

    #ifdef DO_FORK_DEBUG
    kernel_printf("child 's name:%s\n",new->pcb.name);
    kernel_printf("child 's pid : %x\n",new->pcb.asid);
    #endif
    
    return new->pcb.asid;       //返回子进程的进程号
    

    error1:
        return -1;
    error2:
        kfree(new);
        return -2;
    error3:
        kfree(new->pcb.pgd);
        kfree(new);
        return -3;
    error4:
        kfree(new->pcb.pgd);
        kfree(new);
        free_pid(new->pcb.asid);
        return -4;
}

/*复制页表*/
pgd_term *copy_pagetables(PCB* child,PCB* parent)
{
    pgd_term* old_pgd;
    pgd_term* new_pgd=NULL;
    pte_term* temp_pte;
    pte_term* old_pte;
    unsigned int index,ip;                      //索引
    unsigned int count=0;                       //记录pgd中已经成功分配的页数

    old_pgd=parent->pgd;
    new_pgd=(pgd_term*)kmalloc(PAGE_SIZE);      //分配一张新的页作为页目录

    #ifdef COPY_PAGE_DEBUG
    kernel_printf("new pgd address:%x\n",new_pgd);
    #endif

    if(new_pgd==NULL){
        kernel_printf("copy_pagetables failed : failed to malloc for pgd\n");
        goto error1;
    }

    kernel_memcpy(new_pgd,old_pgd,PAGE_SIZE);   //pgd的复制

    /*下面是针对父进程每一个存在的pte的复制*/
    for(index=0;index<KERNEL_ENTRY>>PGD_SHIFT;index++){
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
                if(temp_pte[ip]){
                    unsigned int va;
                    va=index<<PGD_SHIFT;
                    va|=(ip<<PTE_SHIFT);
                    /*新老页表现在都不能往这个地址上写*/
                    clean_W(&(old_pte[ip]));
                    clean_W(&(temp_pte[ip]));
                    tlbp(va,parent->asid);
                    unsigned int tlb_index=get_tlb_index();
                    if(tlb_index&(1<<31)==0)        //在TLB里存在这项内容，需要将其修改
                    tlbwi(va,parent->asid,old_pte[ip],tlb_index);
                }
            }
        }

    }
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("copy pgd and pte over\n");
    #endif
    for(index=0;index<KERNEL_ENTRY>>PGD_SHIFT;index++){
        if(new_pgd[index]){
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
/*删除进程的页表，保留pgd*/
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

/*包含pgd，删除进程的全部页表内容*/
void delete_pagetables(PCB *task)
{
	delete_pages(task);
	kfree(task->pgd);
}

/*把一个进程从进程队列中删去*/
unsigned int del_task(unsigned int pid)
{
    int index=0;
    list_pcb *pos;
    PCB * task_to_del=get_pcb_by_pid(pid);
    if(task_to_del==NULL){
        kernel_printf("process not found,pid %d\n",pid);
        return 1;
    }
    kfree(task_to_del->context);                    //删去上下文
    delete_pagetables(task_to_del);                 //删去页表
    kfree(task_to_del->file);                       //删去文件信息
    free_pid(task_to_del->asid);                    //释放进程号
    list_pcb_del_init(&(task_to_del->process));     //PCB链表中删除
    list_pcb_del_init(&(task_to_del->sched));       //调度队列中删除
    kfree((task_union*)task_to_del);                //删去整个task_union
    return 0;
    
}
/*最简单的外部加载函数，主要用于测试*/
int exec1(char* filename) {
    /*基本的设置*/
    FILE file;
    const unsigned int CACHE_BLOCK_SIZE = 64;
    unsigned char buffer[512];
    #ifdef EXEC_DEBUG
    kernel_printf("begin to exec\n");
    kernel_printf("filename:%s\n",filename);
    #endif
    
    int result = fs_open(&file, filename);          //打开文件
    if (result != 0) {
        kernel_printf("File %s not exist\n", filename);
        return 1;
    }
    unsigned int size = get_entry_filesize(file.entry.data);
    unsigned int n = size / CACHE_BLOCK_SIZE + 1;
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int ENTRY = (unsigned int)kmalloc(4096);//申请页来保存文件内容
    /*文件内容读取*/
    for (j = 0; j < n; j++) {
        fs_read(&file, buffer, CACHE_BLOCK_SIZE);
        kernel_memcpy((void*)(ENTRY + j * CACHE_BLOCK_SIZE), buffer, CACHE_BLOCK_SIZE);
        kernel_cache(ENTRY + j * CACHE_BLOCK_SIZE);
    }
    /*根据申请的页地址来写TLB*/
    unsigned int cp0EntryLo0 = ((ENTRY >> 6) & 0x01ffffc0) | 0x1e;
    asm volatile(
        "li $t0, 1\n\t"
        "mtc0 $t0, $10\n\t"
        "mtc0 $zero, $5\n\t"
        "move $t0, %0\n\t"
        "mtc0 $t0, $2\n\t"
        "mtc0 $zero, $3\n\t"
        "mtc0 $zero, $0\n\t"
        "nop\n\t"
        "nop\n\t"
        "tlbwi"
        :
        : "r"(cp0EntryLo0));
    
    int (*f)() = (int (*)())(0);            //设置函数指针，使这个函数从地址0开始执行
#ifdef EXEC_DEBUG
    kernel_printf("Exec load at: 0x%x\n", ENTRY);
#endif  // ! EXEC_DEBUG
    unsigned int s1=*(unsigned int*)0;
    kernel_printf("The first instruction =%x\n",s1);

    int r = f();                            //具体运行这个函数
    kernel_printf("run the program over\n");
    // while(1);
    kfree((void*)ENTRY);                    //释放空间
    return 0;
}

/*结合进程部分使用的外部加载程序*/
int exec2(PCB *task,char* filename){
    #ifdef EXEC_DEBUG
    kernel_printf("begin to exec\n");
    kernel_printf("task name:%s\n",task->name);
    kernel_printf("task pid:%x\n",task->asid);
    #endif
    //清除PCB的页表并分配第一张页
    delete_pagetables(task);
    task->pgd=(pgd_term*)kmalloc(PAGE_SIZE);
    pgd_term*pgd=task->pgd;
    set_V(&pgd[0]);
    set_W(&pgd[0]);
    pte_term*pte=(pte_term*)kmalloc(PAGE_SIZE);
    pgd[0]|=(unsigned int)pte;
    
    //申请文件控制块的大小
    if(task->file==NULL)
        task->file=(FILE*)kmalloc(sizeof(FILE));
    else{
        kfree(task->file);
        task->file=(FILE*)kmalloc(sizeof(FILE));
    }
    
    #ifdef EXEC_DEBUG
    kernel_printf("filename:%s\n",filename);
    kernel_printf("address of task:%x\n",task);
    kernel_printf("address of task_file:%x\n",task->file);
    #endif

    // fopen操作
    int result = fs_open(task->file, filename);
    kernel_printf("result=%d\n",result);
    if (result != 0) {
        kfree(task->file);
        delete_pagetables(task);
        clean_context(task->context);
        kernel_printf("File %s not exist\n", filename);
        return -1;
    }
    #ifdef EXEC_DEBUG
    kernel_printf("fopen over\n");
    #endif

    //把文件的第一张页大小的内容读取到内存的一张页中
    unsigned int phy_addr;
    phy_addr=read_file_to_page(task->file,0);
    if(phy_addr==0){
        kfree(task->file);
        delete_pagetables(task);
        clean_context(task->context);
        return -1;
    }
    #ifdef EXEC_DEBUG
    kernel_printf("read file over\n");
    #endif
    //清除PCB的上下文并重新设置
    if(task->context!=NULL){
        task->context=(context*)((unsigned int)task+sizeof(PCB));
        task->context->epc=0;           //新进程从0地址开始执行
        task->context->sp=(unsigned int)task+PAGE_SIZE;
        unsigned int init_gp;
        asm volatile("la %0, _gp\n\t" : "=r"(init_gp));
        task->context->gp=init_gp;
    }
    else{
        kernel_printf("error in exec2: context dosen't exist\n");
        return -1;
    }
    #ifdef EXEC_DEBUG
    kernel_printf("%x\n",sizeof(PCB));
    kernel_printf("%x\n",task);
    kernel_printf("%x\n",task->context);
    kernel_printf("epc=%x\n",task->context->epc);
    kernel_printf("pid=%x\n",task->asid);
    #endif

    //物理地址转化为EntryLo0的值
    unsigned int cp0EntryLo0=va2pfn(phy_addr);
    /*向TLB中写入对应项*/
    unsigned int asid=task->asid;
    asm volatile(
        "mtc0 %0, $10\n\t"
        "mtc0 $zero, $5\n\t"
        "mtc0 %1, $2\n\t"
        "mtc0 $zero, $3\n\t"
        "nop\n\t"
        "nop\n\t"
        "tlbwr"
        :
        : "r"(asid),"r"(cp0EntryLo0));

    int (*f)() = (int (*)())(0);
    unsigned int s1=*(unsigned int*)0;
#ifdef EXEC_DEBUG
    kernel_printf("Exec load at: 0x%x\n", phy_addr);
    kernel_printf("The first instruction is %x\n",s1);
#endif  // ! EXEC_DEBUG

    /*把刚申请到的物理帧地址存入到pte中*/
    set_V(&pte[0]);
    set_W(&pte[0]);
    pte[0]|=phy_addr;
    return 0;
}

/*完整的一个外部加载函数，包含了fork和exec两部分*/
int exec(char *filename,char* taskname)
{
    if(filename==NULL||taskname==NULL){
        kernel_printf("error :filename or taskname is not initialized!\n");
        return -1;
    }
    PCB *current=get_current_pcb();         //获得当前正在运行的PCB

    unsigned int child_pid=do_fork(current->context,current);
    PCB *child=get_pcb_by_pid(child_pid);
    kernel_printf("old name:%s\n",child->name);
    kernel_memcpy(child->name,taskname,sizeof(char)*32);
    kernel_printf("new name:%s\n",child->name);
    if(child_pid<0){
        kernel_printf("error! failed to do_fork\n");
        return -1;
    }
    
    //从文件中读取数据并替换
    if(exec1(filename)!=0){
        kernel_printf("error! failed to exec\n");
        return -1;
    }
    // exec2(child,filename);
    //加入到前台队列

    add_to_foreground_list(&(child->sched));
    return child->asid;
}

void print_tasks(){
    int i=1;
    while(1)
    {
        #ifdef PRINT_TASK_DEBUG
        kernel_printf("now i=%x\n",i);
        #endif
        PCB* task=get_pcb_by_pid(i);
        if(task==NULL){
            kernel_printf("print tasks over\n");
            break;}
        else{
            //kernel_printf("ready to print\n");
            i++;
            kernel_printf("pid: %d  ",task->asid);
            kernel_printf("task name: %s  ",task->name);
            kernel_printf("priority: %d  ",task->priority);
            kernel_printf("state: %d",task->state);
            kernel_printf("\n");
        }
    }
}


