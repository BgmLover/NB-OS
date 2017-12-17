#include "page.h"
#include <zjunix/utils.h>
#include "arch.h"
#include <driver/vga.h>
#include <zjunix/slab.h>
#pragma GCC push_options
#pragma GCC optimize("O0")

void set_pgd_attr(pgd_term *pgd,unsigned int attr)
{
    (*pgd)&=(~ATTR_MASK);
    attr&=ATTR_MASK;
    (*pgd)|=attr;
}
void set_pte_attr(pte_term *pte,unsigned int attr)
{
    (*pte)&=(~ATTR_MASK);
    attr&=ATTR_MASK;
    (*pte)|=attr;
}
unsigned int do_one_mapping(pgd_term*pgd,unsigned int va,unsigned int pa,unsigned int attr)
{
    
    unsigned int pgd_index=(va>>PGD_SHIFT)&INDEX_MASK;//取低10位作为页目录索引
    unsigned int pte_index=(va>>PTE_SHIFT)&INDEX_MASK;//取低10位作为页表索引
    //如果该页已经被分配空间了
    if(pgd[pgd_index]!=0)
    {
        kernel_printf("The page %x has been allocated",pgd[pgd_index]);
        return 2;
    }
    pte_term *pte;
    pte=(pte_term *)kmalloc(PAGE_SIZE);
    if(pte==NULL){
        kernel_printf("failed to malloc space for pte");
        return 1;//error
    }
    //设置pgd项属性，但是这里有个问题，如果分配的第二级页表地址不是页对齐的，那这里可能发生地址错乱？
    unsigned int pgd_term=(unsigned int)pte;
    pgd_term&=(~OFFSET_MASK);
    *pgd=pgd_term;
    set_pgd_attr(pgd,Default_attr);

    pa &= (~OFFSET_MASK);
    pa |= attr;
    *pte =pa;
    return 0;

}
void init_pgtable() {
    //TLB硬件初始化
    asm volatile(
        "mtc0 $zero, $2\n\t"    //EntryLo0
        "mtc0 $zero, $3\n\t"    //EntryLo1
        "mtc0 $zero, $5\n\t"    //PageMask
        "mtc0 $zero, $10\n\t"   //EntryHi

        "move $v0, $zero\n\t"
        "li $v1, 32\n"

        "init_pgtable_L1:\n\t"
        "mtc0 $v0, $0\n\t"      //Index
        "addi $v0, $v0, 1\n\t"
        "bne $v0, $v1, init_pgtable_L1\n\t"
        "tlbwi\n\t"
        "nop");
}

#pragma GCC pop_options