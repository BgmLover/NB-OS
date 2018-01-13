#ifndef _ZJUNIX_BUDDY_H
#define _ZJUNIX_BUDDY_H

#include <zjunix/list.h>
#include <zjunix/lock.h>

#define _PAGE_RESERVED (1<<31)//page free
#define _PAGE_ALLOCATED (1<<30)//page busy
#define _PAGE_SLUB (1 << 29)//page allocated by slub
#define PAGE_SHIFT 12
#define MAX_BUDDY_ORDER 4

// 页结构
struct page{
    unsigned int flag;//状态
    unsigned int reference;
    struct list_head list;空闲链表
    void* virtual;//指向自己所在的cache
    unsigned int private; // 
    unsigned int bplevel; // = order
};

// 空闲链表
struct freelist{
    unsigned int nr_free; //number of elements
    struct list_head free_head;// 左右指针
};

// buddy结构
struct buddy_sys{
    unsigned int buddy_start_pfn, buddy_end_pfn;//起始、末尾页框地址
    struct page* start_page;
    struct lock_t lock;
    struct freelist freelist[MAX_BUDDY_ORDER];// 5个空闲链表
};

#define _is_same_bpgroup(page, bage) (((*(page)).bplevel == (*(bage)).bplevel))
#define _is_same_bplevel(page, lval) ((*(page)).bplevel == (lval))
#define set_bplevel(page, lval) ((*(page)).bplevel = (lval))
#define set_flag(page, val) ((*(page)).flag |= (val))
#define clean_flag(page, val) ((*(page)).flag &= ~(val))
#define has_flag(page, val) ((*(page)).flag & val)
#define set_ref(page, val) ((*(page)).reference = (val))
#define inc_ref(page, val) ((*(page)).reference += (val))
#define dec_ref(page, val) ((*(page)).reference -= (val))

extern struct page* pages;
extern struct buddy_sys buddy;

extern void buddy_init_pages();
extern void buddy_info();
extern void buddy_init_buddy();
extern void buddy_free_pages(struct page* page, unsigned int order);
extern struct page* buddy_alloc_pages(unsigned int order);


#endif
