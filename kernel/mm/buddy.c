#include <driver/vga.h>
#include <zjunix/bootmem.h>
#include <zjunix/buddy.h>
#include <zjunix/list.h>
#include <zjunix/lock.h>
#include <zjunix/utils.h>

struct page* pages;
struct buddy_sys buddy;
unsigned int kernel_start_pfn, kernel_end_pfn;

// 页初始化
void buddy_init_pages(unsigned int start_pfn, unsigned int end_pfn) {
    unsigned int i;
    for (i = start_pfn; i < end_pfn; i++) {
        clean_flag(pages + i, -1);
        //*(pages+i).flag = ~(-1);
        set_flag(pages + i, _PAGE_RESERVED);
        (pages + i)->reference = 1;
        (pages + i)->virtual = (void *)(-1);
        (pages + i)->bplevel = (-1);
        (pages + i)->private = 0;  
        INIT_LIST_HEAD(&(pages[i].list));
    }
}

//输出buddy里5个链表的信息，用于debug
void buddy_info() {
    unsigned int index;
    kernel_printf("Buddy-system :\n");
    kernel_printf("\tstart page-frame number : %x\n", buddy.buddy_start_pfn);
    kernel_printf("\tend page-frame number : %x\n", buddy.buddy_end_pfn);
    for (index = 0; index <= MAX_BUDDY_ORDER; ++index) {
        kernel_printf("\t(%x)# : %x frees\n", index, buddy.freelist[index].nr_free);
    }
}

//初始化
void buddy_init_buddy(){
    unsigned int size=sizeof(struct page);
    unsigned char* addr;
    int i;

    //从bootmem里取没用到的页。
    addr=bootmem_alloc_pages(size*mm.max_pfn, _MM_KERNEL, 1<<PAGE_SHIFT);
    if(!addr){
        // fail to allocate memory
        kernel_printf("\nERROR: bootmem_alloc_pages failed!\n");
        while(1){}
    }

    pages=(struct page *)((unsigned int)addr | 0x80000000);

    buddy_init_pages(0, mm.max_pfn);

    //find kernel-used memory
    kernel_start_pfn = 0;
    kernel_end_pfn = 0;
    for (i = 0; i < mm.cnt_infos; ++i) {
        if (mm.info[i].end_pfn > kernel_end_pfn)
            kernel_end_pfn = mm.info[i].end_pfn;
    }
    kernel_end_pfn >>= PAGE_SHIFT;

//计算buddy的起始、结束页框号
    buddy.buddy_start_pfn = (kernel_end_pfn + (1 << MAX_BUDDY_ORDER) - 1) &
                            ~((1 << MAX_BUDDY_ORDER) - 1);              // the pages that bootmm using cannot be merged into buddy_sys
    buddy.buddy_end_pfn = mm.max_pfn & ~((1 << MAX_BUDDY_ORDER) - 1); //2 pages for IO

//初始化链表
    for (i = 0; i < MAX_BUDDY_ORDER + 1; i++) {
        buddy.freelist[i].nr_free = 0;
        INIT_LIST_HEAD(&(buddy.freelist[i].free_head));
    }

    buddy.start_page=pages+buddy.buddy_start_pfn;
    init_lock(&(buddy.lock));

//把每个order=0的块压入链表
    for (i = buddy.buddy_start_pfn; i < buddy.buddy_end_pfn; ++i) {
        buddy_free_pages(pages + i, 0);
    }

    buddy_info();
}

// 释放一个空闲块，压入链表
void buddy_free_pages(struct page* page, unsigned int order){
    unsigned int page_idx, buddy_idx;
    unsigned int combined_idx;
    struct page* buddy_page;

    // clean_flag(page, -1);
    page->flag = 0;

    lockup(&buddy.lock);
/*
    page_idx = page-buddy.start_page; // current page index
    while(order<MAX_BUDDY_ORDER){
        buddy_idx = page_idx^(1<<order); // the index of its buddy
        buddy_page = page + (buddy_idx - page_idx); // get the buddy page
        if(buddy_page->bplevel != order) break; //not same order, stop
        list_del_init(&buddy_page->list);
        --buddy.freelist[order].nr_free;
        set_bplevel(buddy_page, -1);
        combined_idx = buddy_idx & page_idx; // new page addr = small one among two blocks
        page += (combined_idx - page_idx);
        page_idx = combined_idx;
        ++order;
    }*/
    page_idx = page-buddy.start_page; // current page index
    while(order<MAX_BUDDY_ORDER){
        buddy_idx = page_idx^(1<<order); // the index of its buddy
        buddy_page = page + (buddy_idx - page_idx); // get the buddy page
        if(buddy_page->bplevel != order){
            //尝试另一个方向匹配
            if(buddy_idx<page_idx) buddy_idx = page_idx+(1<<order);
            else buddy_idx = page_idx-(1<<order);
            buddy_page = page+(buddy_idx-page_idx);
            if(buddy_page->bplevel != order) break;
        }
        list_del_init(&buddy_page->list);
        --buddy.freelist[order].nr_free;
        set_bplevel(buddy_page, -1);
        combined_idx = buddy_idx & page_idx; // new page addr = small one among two blocks
        page += (combined_idx - page_idx);
        page_idx = combined_idx;
        ++order;
    }
    // set_bplevel(page, order);
    page->bplevel = order;
    list_add(&(page->list), &(buddy.freelist[order].free_head)); //add to the head of list
    ++buddy.freelist[order].nr_free;
    unlock(&buddy.lock);
}

struct page* buddy_alloc_pages(unsigned int order){
    unsigned int current_order, size;
    struct page* page, *buddy_page;
    struct freelist* free;

    lockup(&buddy.lock);

    for(current_order = order; current_order<=MAX_BUDDY_ORDER; ++current_order){
        free = buddy.freelist + current_order;
        if(!list_empty(&free->free_head)) goto found;
    }

    unlock(&buddy.lock);
    return 0;

found:
    page = container_of(free->free_head.next, struct page, list);
    list_del_init(&(page->list));
    set_bplevel(page, order);
    set_flag(page, _PAGE_ALLOCATED);
    --(free->nr_free);

    size = 1<<current_order; // how many pages per block
    while(current_order>order){
        --free;
        --current_order;
        size >>= 1;
        buddy_page = page + size;
        list_add(&(buddy_page->list), &(free->free_head));
        ++(free->nr_free);
        set_bplevel(buddy_page, current_order);
    }

    unlock(&buddy.lock);
    return page;

}
