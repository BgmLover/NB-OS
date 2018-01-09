#include <arch.h>
#include <driver/ps2.h>
#include <driver/vga.h>
#include <exc.h>
#include <intr.h>
#include <page.h>
#include <zjunix/list_pcb.h>
#include <zjunix/bootmem.h>
#include <zjunix/buddy.h>
#include <zjunix/fs/fat.h>
#include <zjunix/log.h>
//#include <zjunix/pc.h>
#include <zjunix/task.h>
#include <zjunix/sche.h>
#include <zjunix/slub.h>
#include <zjunix/syscall.h>
#include <zjunix/time.h>
#include <zjunix/shm.h>
#include "../usr/ps.h"
//#include "../usr/exec.h"

void machine_info() {
    int row;
    int col;
    kernel_printf("\n%s\n", "ZJUNIX V1.0");
    row = cursor_row;
    col = cursor_col;
    cursor_row = 29;
    kernel_printf("%s", "Created by System Interest Group, Zhejiang University.");
    cursor_row = row;
    cursor_col = col;
    kernel_set_cursor();
}

#pragma GCC push_options
#pragma GCC optimize("O0")
/*void create_startup_process() {
    unsigned int init_gp;
    asm volatile("la %0, _gp\n\t" : "=r"(init_gp));
    pc_create(1, ps, (unsigned int)kmalloc(4096) + 4096, init_gp, "powershell");
    log(LOG_OK, "Shell init");
    pc_create(2, system_time_proc, (unsigned int)kmalloc(4096) + 4096, init_gp, "time");
    log(LOG_OK, "Timer init");
}*/
#pragma GCC pop_options

void create_proc()
{
    kernel_printf("create begin\n");
    int i=0;
    unsigned int init_gp;

    task_union* proc2=( task_union*)kmalloc(PAGE_SIZE);

    kernel_printf("stop\n");
    while(1);   
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

    //list_pcb_add_tail(&(proc2->pcb.process),&high_list);
    add_to_foreground_list(&(proc2->pcb.process));
}

void show_status()
{
    unsigned int status,cause;
        asm volatile(
            "mfc0 %0,$12\n\t"
            "mfc0 %1,$13\n\t"
            :"=r"(status),"=r"(cause)
        );
        kernel_printf("status:%x\n",status);
        kernel_printf("cause:%x\n",cause);
}
void init_kernel() {
    void* addr;

    kernel_clear_screen(31);
    // Exception
    init_exception();
    show_status();
    // Page table
    init_pgtable();
    // Drivers
    init_vga();
    init_ps2();
    // Memory management
    log(LOG_START, "Memory Modules.");
    bootmem_init();
    log(LOG_OK, "Bootmem.");
    buddy_init_buddy();
    log(LOG_OK, "Buddy.");
    slub_init();
    log(LOG_OK, "Slab.");

/*test memory*/
    addr=kmalloc(4096);
    kernel_printf("%x\n", (unsigned int)addr);
    kfree(addr);
    addr=kmalloc(4096);
    kernel_printf("%x\n", (unsigned int)addr);
    addr=kmalloc(4096);
    kernel_printf("%x\n", (unsigned int)addr);


    /*end test memory*/

    shm_init();
    log(LOG_OK, "Shm.");
    log(LOG_END, "Memory Modules.");
    // File system
    log(LOG_START, "File System.");
    init_fs();
    log(LOG_END, "File System.");
    // System call
    log(LOG_START, "System Calls.");
    init_syscall();
    log(LOG_END, "System Calls.");
    // Process control
    log(LOG_START, "Process Control Module.");
    //init_pc();
    init_task();
    //create_startup_process();
    //task_test();
    //exec2(pcbs.next->pcb,"/seg.bin");
    exec("/seg.bin","789");
    //exec1("/seg.bin");
    // log(LOG_END, "Process Control Module.");\
    // //shced
    // log(LOG_START, "Sched.");
    // //init_sched();
    // //create_proc();
    // log(LOG_END, "Sched.");
    // // Interrupts
    // log(LOG_START, "Enable Interrupts.");
    // init_interrupts();
    // show_status();
    // log(LOG_END, "Enable Interrupts.");
    // // Init finished
    // machine_info();


    //*GPIO_SEG = 0x78778245;
    
    // Enter shell
    while (1);
}
