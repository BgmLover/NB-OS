#ifndef _ZJUNIX_BOOTMEM_H
#define _ZJUNIX_BOOTMEM_H

#define PAGE_FREE 0
#define PAGE_USED 0Xff
#define MAX_INFO 10
#define PAGE_SHIFT 12
#define PAGE_ALIGN (~((1<<PAGE_SHIFT)-1))

enum mm_usage{
    _MM_KERNEL,
    _MM_MMMAP,
    _MM_VGABUFF, 
    _MM_PDTABLE, 
    _MM_PTABLE, 
    _MM_DYNAMIC, 
    _MM_RESERVED, 
    _MM_COUNT
};

// info结构体，管理分配的空间
struct bootmem_info{
    unsigned int start_pfn, end_pfn, type;
};

// bootmem结构体
struct bootmem{
    unsigned int phymm;
    unsigned int max_pfn;
    unsigned char* s_map, *e_map;//位图起始、末尾指针
    unsigned int last_alloc;//最近一次分配的地址
    unsigned int cnt_infos;
    struct bootmem_info info[MAX_INFO];//info数组
};

extern struct bootmem mm;

extern void bootmem_set_mminfo(struct bootmem_info *info, unsigned int start, unsigned int end, unsigned int type);
extern void bootmem_insert_mminfo(unsigned int start, unsigned int end, unsigned int type);
extern void bootmem_remove_mminfo(unsigned int index);
extern void bootmem_init();
extern void bootmem_bootmap_info(unsigned char *msg);
extern unsigned char* bootmem_find_pages(unsigned int count, unsigned int s_pfn, unsigned int e_pfn, unsigned int align_pfn);
extern unsigned int bootmem_set_map(unsigned int start, unsigned int end, unsigned int typ);
extern unsigned int bootmem_split_mminfo(unsigned int index, unsigned int split_start);
extern unsigned char *bootmem_alloc_pages(unsigned int size, unsigned int type, unsigned int align);



#endif