#ifndef _ZJUNIX_SLUB_H
#define _ZJUNIX_SLUB_H

#include <zjunix/list.h>
#include <zjunix/buddy.h>
#include <zjunix/task.h>

#define SIZE_INT 4
#define SLAB_AVAILABLE 0x0
#define SLAB_USED 0xff

struct slub_head{
    void* end_ptr;
    unsigned int nr_objs;
};

struct kmem_cache_node{
    struct list_head partial;
    struct list_head full;
};

struct kmem_cache_cpu{
    void** freeobj;
    struct page* page;
};

struct kmem_cache{
    unsigned int size;
    unsigned int objsize;
    unsigned int offset;
    struct kmem_cache_node node;
    struct kmem_cache_cpu cpu;
    unsigned char name[16];
};

#define KMEM_ADDR(PAGE, BASE) ((((PAGE) - (BASE)) << PAGE_SHIFT) | 0x80000000);

extern struct kmem_cache kmalloc_caches[PAGE_SHIFT];

extern void slub_init();
extern void slub_init_each_slub(struct kmem_cache* cache, unsigned int size);
extern void slub_init_kmem_node(struct kmem_cache_node* knode);
extern void slub_init_kmem_cpu(struct kmem_cache_cpu* kcpu);

extern void* slub_alloc(struct kmem_cache* cache);
extern void slub_format_slubpage(struct kmem_cache* cache, struct page* page);
extern struct kmem_cache* slub_get_slub(unsigned int size);
extern void* kmalloc(unsigned int size);
extern void slub_free(struct kmem_cache* cache, void* obj);
extern void kfree(void* obj);

/* syscall*/
void syscall_kmalloc_21(unsigned int status, unsigned int cause, context* pt_context);
void syscall_kfree_22(unsigned int status, unsigned int cause, context* pt_context);
/*
void* malloc(unsigned int size);
void free(void* obj);
*/

#endif 
