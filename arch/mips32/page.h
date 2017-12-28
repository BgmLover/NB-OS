#ifndef _PAGE__H
#define _PAGE__H

/*
页表结构采用两级页表来实现
页大小为4KB
虚拟地址32位：
0-11：页内偏移
12-21：第二级页表索引
22-31：第一级页表索引

第一级页表大小为1个页大小
第二级页表大小为1个页大小
*/
#define PAGE_SIZE 4096   //4KB
#define PAGE_SHIFT 12    

#define PGD_SHIFT 22    
#define PTE_SHIFT 12
#define INDEX_MASK 0x3ff  //10 bit 1
#define OFFSET_MASK 0xfff //12 bit 1
#define ATTR_MASK 0x1f
#define NULL (void *)0

#define Default_attr 0x4//可写
typedef unsigned int pgd_term;
typedef unsigned int pte_term;
#define set_G(addr) ((*addr)|=0x00000001)
#define clean_G(addr) ((*addr)&=0xfffffffe)
#define is_G(addr) ((*addr)&0x00000001)

#define set_V(addr) ((*addr)|=0x00000002)
#define clean_V(addr) ((*addr)&=0xfffffffd)
#define is_V(addr) ((*addr)&0x00000002)

#define set_W(addr) ((*addr)|=0x00000004)
#define clean_W(addr) ((*addr)&=0xfffffffb)
#define is_W(addr) ((*addr)&0x0000000b)

#define set_D(addr) ((*addr)|=0x00000008)
#define clean_D(addr) ((*addr)&=0xfffffff7)
#define is_D(addr) ((*addr)&0x00000008)

#define set_A(addr) ((*addr)|=0x00000010)
#define clean_A(addr) ((*addr)&=0xffffffef)
#define is_A(addr) ((*addr)&0x00000010)
/*
typedef struct{
    unsigned int G : 1;     //全局位
    unsigned int V : 1;     //有效位
    unsigned int W : 1;     //是否可写
    unsigned int D : 1;     //dirty位
    unsigned int A : 1;     //访问位
    unsigned int reserved :7;//保留
    unsigned int PFN :20;   //虚拟地址物理帧号
} pte_term;

typedef struct{
    unsigned int G : 1;     //全局位
    unsigned int V : 1;     //有效位
    unsigned int W : 1;     //是否可写
    unsigned int D : 1;     //dirty位
    unsigned int A : 1;     //访问位
    unsigned int reserved :7;//保留
    unsigned int pte_addr :20;   //二级页表物理帧号
} pgd_term;
*/
void init_pgtable();
void set_pgd_attr(pgd_term *pgd,unsigned int attr);
void set_pte_attr(pte_term *pte,unsigned int attr);
//分配一张页的空间，并把物理页与虚拟页做一次映射
unsigned int do_one_mapping(pgd_term*pgd,unsigned int va,unsigned int pa,unsigned int attr);
void tlbwi(unsigned int virtual_addr,unsigned int asid,unsigned int pte_con,unsigned int index);
void tlbwr(unsigned int virtual_addr,unsigned int asid,unsigned int pte_con);
void tlbp(unsigned int virtual_addr,unsigned int asid );
unsigned int get_tlb_index();
<<<<<<< HEAD
//申请一个物理页的空间，从指定位置从文件读取连续内容，直到满一个页大小，返回页地址
unsigned int read_file_to_page(FILE*file,unsigned int start);
void clean_page(unsigned int *page);
=======


>>>>>>> 010d33362424dcf079b7ea294e814d2fdf22c585

//About TLB
typedef struct {
    unsigned int reserved1 : 12;
    unsigned int Mask : 16;
    unsigned int reserved0 : 4;
} PageMask;

typedef struct {
    unsigned int ASID : 8;
    unsigned int reserved : 5;
    unsigned int VPN2 : 19;
} EntryHi;

typedef struct {
    unsigned int G : 1;
    unsigned int V : 1;
    unsigned int D : 1;
    unsigned int C : 3;
    unsigned int PFN : 24;
    unsigned int reserved : 2;
} EntryLo;

typedef struct {
    EntryLo EntryLo0;
    EntryLo EntryLo1;
    EntryHi EntryHi;
    PageMask PageMask;//页大小为4KB，因此PageMask的值为0
} PageTableEntry;


#endif
