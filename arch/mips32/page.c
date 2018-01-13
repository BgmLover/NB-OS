#include "page.h"
#include <zjunix/utils.h>
#include "arch.h"
#include <driver/vga.h>
#include <zjunix/slub.h>

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
    
unsigned int read_file_to_page(FILE*file,unsigned int start){
    unsigned char buffer[512];
    const unsigned int CACHE_BLOCK_SIZE = 64;
    unsigned int j;
    unsigned int new=(unsigned int )kmalloc(PAGE_SIZE);
    kernel_printf("new page address:%x\n",new);
    if(!new){
        kernel_printf("read_file_to_page error: failed to malloc for a page\n");
        return 0;
    }

    unsigned int size = get_entry_filesize(file->entry.data);
    unsigned int n = size / CACHE_BLOCK_SIZE + 1;
    fs_lseek(file,start);
    for (j = 0; j < n; j++) {
        fs_read(file, buffer, CACHE_BLOCK_SIZE);
        kernel_memcpy((void*)(new + j * CACHE_BLOCK_SIZE), buffer, CACHE_BLOCK_SIZE);
        //kernel_cache(new + j * CACHE_BLOCK_SIZE);
        //kernel_cache(new + j * CACHE_BLOCK_SIZE);
        if((j+1)==PAGE_SIZE/CACHE_BLOCK_SIZE){
            break;//已经读满了一个页的内容
        }
    }
    return new;
}  
void clean_page(unsigned int *page)
{
    int i;
    for(i=0;i<PAGE_SIZE>>2;i++)
        page[i]=0;
}
/*增加页表对应物理帧的引用次数*/
void inc_refrence_by_pte( unsigned int *pte)
{
	 unsigned int index;

	for (index = 0; index < (PAGE_SIZE >> 2); ++index) {
		if (is_V(&(pte[index]))) {
			inc_ref(pages + (pte[index] >> PAGE_SHIFT), 1);
		}
	}
}
/*减少页表对应物理帧的引用次数*/
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
void tlbwi(unsigned int virtual_addr,unsigned int asid,unsigned int pte_con,unsigned int index)
{
    EntryHi entryhi;
    EntryLo L0;
    entryhi.VPN2=virtual_addr>>13;
    entryhi.ASID=asid;
    L0.PFN=pte_con>>PAGE_SHIFT;
    L0.G=is_G(&pte_con);
    L0.V=is_V(&pte_con);
    L0.D=is_W(&pte_con);
    L0.C=0;
    unsigned int hi,en0;
    hi=*((unsigned int *)&entryhi);
    en0=*((unsigned int *)&L0);
    asm volatile(
        "mtc0 %0, $2\n\t" 
        "mtc0 %1, $10\n\t"
        "mtc0 %2, $0\n\t"
        "mtc0 $zero,$3\n\t"
        "tlbwi\n\t"
        : "=r"(en0),"=r"(hi),"=r"(index)
    );
}
void tlbwr(unsigned int virtual_addr,unsigned int asid,unsigned int pte_con)
{
    EntryHi entryhi;
    EntryLo L0;
    entryhi.VPN2=virtual_addr>>13;
    entryhi.ASID=asid;
    L0.PFN=pte_con>>PAGE_SHIFT;
    L0.G=is_G(&pte_con);
    L0.V=is_V(&pte_con);
    L0.D=is_W(&pte_con);
    L0.C=0;
    unsigned int hi,en0;
    hi=*((unsigned*)&entryhi);
    en0=*((unsigned*)&L0);
    asm volatile(
        "mtc0 %0, $2\n\t" 
        "mtc0 %1, $10\n\t"
        "mtc0 $zero,$3\n\t"
        "tlbwr\n\t"
        : "=r"(en0),"=r"(hi)
    );
}
void tlbp(unsigned int virtual_addr,unsigned int asid)
{
    EntryHi entryhi;
    entryhi.VPN2=virtual_addr>>13;
    entryhi.ASID=asid;
    unsigned int hi;
    hi=*((unsigned*)&entryhi);
    asm volatile(
        "mtc0 %0, $10\n\t"
        "tlbp\n\t"
        : "=r"(hi)
    );
}
unsigned int get_tlb_index(){
    unsigned int index;
    asm volatile(
        "mfc0 %0, $0\n\t" 
        : "=r"(index)
    );
    return index;
}
void set_default_attr(EntryLo *entry)
{
    entry->G=0;
    entry->V=1;
    entry->D=1;
    entry->C=3;
}
unsigned int va2pfn(unsigned int vaddr)
{
    EntryLo entry;
    set_default_attr(&entry);
    entry.PFN=get_pfn(vaddr);
    unsigned int res;
    res=*((unsigned int *)&entry);
    return res;
}
unsigned int pt2pfn(pte_term pt){
    EntryLo entry;
    entry.G=is_G(&pt);
    entry.V=is_V(&pt);
    entry.C=3;
    entry.D=is_W(&pt);
    entry.PFN=get_pfn(pt);
    unsigned int res;
    res=*((unsigned int *)&entry);
    return res;
}
#pragma GCC pop_options