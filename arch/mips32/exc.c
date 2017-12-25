#include "exc.h"

#include <driver/vga.h>
#include <zjunix/task.h>
#include <zjunix/sche.h>
#include <page.h>
#pragma GCC push_options
#pragma GCC optimize("O0")

exc_fn exceptions[32];

void do_exceptions(unsigned int status, unsigned int cause, context* pt_context) {
    int index = cause >> 2;
    index &= 0x1f;
    if (exceptions[index]) {
        exceptions[index](status, cause, pt_context);
    } 
    // else {
    //     task_struct* pcb;
    //     unsigned int badVaddr;
    //     asm volatile("mfc0 %0, $8\n\t" : "=r"(badVaddr));
    //     pcb = get_curr_pcb();
    //     kernel_printf("\nProcess %s exited due to exception cause=%x;\n", pcb->name, cause);
    //     kernel_printf("status=%x, EPC=%x, BadVaddr=%x\n", status, pcb->context.epc, badVaddr);
    //     pc_kill_syscall(status, cause, pt_context);
    //     while (1)
    //         ;
    // }
}

void register_exception_handler(int index, exc_fn fn) {
    index &= 31;
    exceptions[index] = fn;
}

void init_exception() {
    // status 0000 0000 0000 0000 0000 0000 0000 0000
    // cause 0000 0000 1000 0000 0000 0000 0000 0000
    asm volatile(
        "mtc0 $zero, $12\n\t"
        "li $t0, 0x800000\n\t"
        "mtc0 $t0, $13\n\t");
}

void tlb_refill(){
    pgd_term *pgd;
    pte_term *pte;
    unsigned int badVaddr,phy_addr;
    unsigned int pgd_index,pte_index,*entry0;
    asm volatile(
        "mfc0 %0, $8\n\t" : "=r"(badVaddr)
    );
    PCB *current=get_current_pcb();
    pgd=current->pgd;
    pgd_index=badVaddr>>PGD_SHIFT;
    pgd_index&=INDEX_MASK;
    pte_index=badVaddr>>PTE_SHIFT;
    pte_index&=INDEX_MASK;
    pte=(pte_term*)pgd[pgd_index];
    phy_addr=pte[pte_index];
    __EntryLo L0;
    L0.PFN=phy_addr>>12;
    L0.G=is_G(&phy_addr);
    L0.V=is_V(&phy_addr);
    L0.D=is_W(&phy_addr);
    L0.C=0;
    entry0=(unsigned int*)(&L0);
    //填入EntryLo0的值即可
    asm volatile(
        "mtc0 %0, $2\n\t" 
        "mtc0 $zero,$3\n\t"
        : "=r"(*entry0)
    );
}
#pragma GCC pop_options