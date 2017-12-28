#include "exc.h"

#include <driver/vga.h>
#include <zjunix/task.h>
#include <zjunix/sche.h>
#include <zjunix/buddy.h>
#include <zjunix/slub.h>
#include <page.h>
#include <debug.h>
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

void die(){while(1);}

void tlb_modified_exception(unsigned int status,unsigned int cause, context* pt_context)
{
    EntryLo L0;
    unsigned int *entry0;
    unsigned int badVaddr;
    asm volatile("mfc0 %0, $8\n\t" : "=r"(badVaddr));

    PCB* current =get_current_pcb();
    pgd_term *pgd=current->pgd;
    unsigned int pgd_index=badVaddr>>PGD_SHIFT;
    pgd_index&=INDEX_MASK;
    unsigned int pte_index=badVaddr>>PTE_SHIFT;
    pte_index&=INDEX_MASK;

    if(!pgd[pgd_index])
    {
        kernel_printf("tlb_modified error: The pte equals to 0\n");
        #ifdef EXCEPTION_DEBUG
        kernel_printf("die in the tlb_modified\n");
        die();
        #endif
        goto kill;
    }
    else if(!is_V(&pgd[pgd_index]))
    {
        kernel_printf("tlb_modified error: The pte is invalid\n");
        #ifdef EXCEPTION_DEBUG
        kernel_printf("die in the tlb_modified\n");
        die();
        #endif
        goto kill;
    }
    else if(!is_W(&pgd[pgd_index]))
    {
        kernel_printf("tlb_modified error: The pte can't be written\n");
        #ifdef EXCEPTION_DEBUG
        kernel_printf("die in the tlb_modified\n");
        die();
        #endif
        goto kill;
    }
    //读取二级页表项中的对应index的内容
    pte_term *pte=(pte_term*) (pgd[pgd_index]&(~OFFSET_MASK));
    if(!pte[pte_index])
    {
        kernel_printf("tlb_modified error: The phy_addr equals to 0\n");
        #ifdef EXCEPTION_DEBUG
        kernel_printf("die in the tlb_modified\n");
        die();
        #endif
        goto kill;
    }
    else if(!is_V(&pte[pte_index]))
    {
        kernel_printf("tlb_modified error: The phy_addr is invalid\n");
        #ifdef EXCEPTION_DEBUG
        kernel_printf("die in the tlb_modified\n");
        die();
        #endif
        goto kill;
    }
    pte_term phy_addr=pte[pte_index];
    struct page *old;
    old=pages+(phy_addr>>PAGE_SHIFT);
    if(old->reference==1)
    {
        //如果该页只被引用了一次，则将其设置为可写
        set_W(&phy_addr);
        goto ok;
    }
    else {
        //重新分配一个物理帧给进程
        unsigned int *new=kmalloc(PAGE_SIZE);
        if(!new){
            kernel_printf("tlb_modified error : failed to malloc for a new page\n");
        #ifdef EXCEPTION_DEBUG
        kernel_printf("die in the tlb_modified\n");
        die();
        #endif            
            goto kill;
        }
        dec_ref(old,1);
        kernel_memcpy(new,(void*)(phy_addr&(~OFFSET_MASK)),PAGE_SIZE);
        pte[pte_index]&=OFFSET_MASK;
        pte[pte_index]|=(unsigned int )new;
        set_W(&pte[pte_index]);
        goto ok;
    }

    kill:
        //kill current pc
        #ifdef EXCEPTION_DEBUG
        kernel_printf("die in the tlb_invalid\n");
        die();
        #endif
        del_task(current->asid);
        return ;
    ok:
        
        L0.PFN=phy_addr>>12;
        L0.G=is_G(&phy_addr);
        L0.V=is_V(&phy_addr);
        L0.D=is_W(&phy_addr);
        L0.C=0;
    //传入对应内容
        entry0=(unsigned int*)(&L0);
        asm volatile(
        "mtc0 %0, $2\n\t"
        "nop\n\t"
        "nop\n\t"
        "tlbwi\n\t"
        : "=r"(entry0));
}
//page fault
void tlb_invalid_exception(unsigned int status, unsigned int cause, context* pt_context)
{
/*  
    if addr>=8000,0000   error   kernel addr can't cause page fault
    if addr<8000,0000    load from file
*/  pgd_term *pgd;
    pte_term *pte;
    unsigned int badVaddr,phy_addr;
    unsigned int pgd_index,pte_index;
    asm volatile("mfc0 %0, $8\n\t" : "=r"(badVaddr));
    PCB * current =get_current_pcb();
    pgd=current->pgd;
    pgd_index=badVaddr>>PGD_SHIFT;
    pgd_index&=INDEX_MASK;
    pte_index=badVaddr>>PTE_SHIFT;
    pte_index&=INDEX_MASK;
    if(badVaddr>=0x80000000)
    {
        kernel_printf("error: task %x  access to address:%x",current->asid,badVaddr);
        goto kill;
    }
    else{
        /*
        1.swap
        */
        if(current->file==NULL)
        {
            kernel_printf("tlb invalid error: the task has no file information\n");
            goto kill;
        }
        //start的值为该虚拟地址所在的页的基地址
        unsigned int start=badVaddr&(~OFFSET_MASK);
        unsigned int new_phy_addr;
        //从文件里读取内容到新的一张物理页中
        new_phy_addr=read_file_to_page(current->file,start);
        if(new_phy_addr==0){
            kernel_printf("tlb invalid error: failed to read info to page from file\n");
            goto kill;
        }
        //填充页表对应项，并设置为有效，可写
        pte=(pte_term*) (pgd[pgd_index]&(~OFFSET_MASK));
        set_V(&new_phy_addr);
        set_W(&new_phy_addr);
        pte[pte_index]=new_phy_addr;
        asm volatile(
        "mtc0 %0, $2\n\t"
        "nop\n\t"
        "nop\n\t"
        "tlbwi\n\t"
        : "=r"(new_phy_addr));
    }
    kill:
        #ifdef EXCEPTION_DEBUG
        kernel_printf("die in the tlb_invalid\n");
        die();
        #endif
        del_task(current->asid);
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
    //读取二级页表项中的对应index的内容
    pte=(pte_term*) (pgd[pgd_index]&(~OFFSET_MASK));
    phy_addr=pte[pte_index];
    //设置EntryLo0的内容
    EntryLo L0;
    L0.PFN=phy_addr>>12;
    L0.G=is_G(&phy_addr);
    L0.V=is_V(&phy_addr);
    L0.D=is_W(&phy_addr);
    L0.C=0;
    //传入对应内容
    entry0=(unsigned int*)(&L0);
    //填入EntryLo0的值即可
    asm volatile(
        "mtc0 %0, $2\n\t" 
        "mtc0 $zero,$3\n\t"
        : "=r"(*entry0)
    );
}

void register_exception_handler(int index, exc_fn fn) {
    index &= 31;
    exceptions[index] = fn;
}

void init_exception() {
    // status 0000 0000 0000 0000 0000 0000 0000 0000
    // cause 0000 0000 1000 0000 0000 0000 0000 0000
    asm volatile(
<<<<<<< HEAD
        
        "mtc0 $zero, $12\n\t"
        "li $t0, 0x800000\n\t"
        "mtc0 $t0, $13\n\t");
    // register_exception_handler(1,tlb_modified_exception);
    // register_exception_handler(2,tlb_invalid_exception);
    // register_exception_handler(3,tlb_invalid_exception);
}
#pragma GCC pop_options