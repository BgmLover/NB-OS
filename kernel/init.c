#include <arch.h>
#include <driver/ps2.h>
#include <driver/vga.h>
#include <exc.h>
#include <intr.h>
#include <page.h>
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
#include "../usr/exec.h"

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
    task_test();
<<<<<<< HEAD
    log(LOG_END, "Process Control Module.");\
    //shced
    //log(LOG_START, "Sched.");
    //init_sched();
    //log(LOG_END, "Sched.");
=======
    kernel_printf("ready to exec\n");
    //exec("/s/seg.bin");
    log(LOG_END, "Process Control Module.");
>>>>>>> bc207f82139b422832eb46ab658517d412f5405b
    // Interrupts
    log(LOG_START, "Enable Interrupts.");
    init_interrupts();
    show_status();
    log(LOG_END, "Enable Interrupts.");
    // Init finished
    machine_info();


    *GPIO_SEG = 0x78778245;
    int flag=0;
    // Enter shell
<<<<<<< HEAD
    int flag=0;
    while (1)

        ;
}
=======
    while (1);
}
>>>>>>> bc207f82139b422832eb46ab658517d412f5405b
