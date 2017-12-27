#include <arch.h>
#include <driver/vga.h>
#include <zjunix/slub.h>
#include <zjunix/utils.h>

struct kmem_cache kmalloc_caches[PAGE_SHIFT];

static unsigned int size_kmem_cache[PAGE_SHIFT] = {96, 192, 8, 16, 32, 64, 128, 256, 512, 1024, 1536, 2048};

void slub_init_kmem_node(struct kmem_cache_node* knode){
    INIT_LIST_HEAD(&(knode->full));
    INIT_LIST_HEAD(&(knode->partial));
}

void slub_init_kmem_cpu(struct kmem_cache_cpu* kcpu){
    kcpu->page = 0;
    kcpu->freeobj = 0;
}

void slub_init_each_slub(struct kmem_cache* cache, unsigned int size){
    cache->objsize = size;
    cache->objsize += (SIZE_INT - 1); // align by 0x100
    cache->objsize &= -(SIZE_INT - 1);
    cache->offset = cache->objsize;
    cache->size = cache->objsize + sizeof(void*);
    slub_init_kmem_cpu(&(cache->cpu));
    slub_init_kmem_node(&(cache->node));
}


void slub_init(){
    unsigned int order;
    // init caches
    slub_init_each_slub(&(kmalloc_caches[1]), 96);
    slub_init_each_slub(&(kmalloc_caches[1]), 192);

    for(order=3; order<=11; ++order){
        slub_init_each_slub(&kmalloc_caches[order], 1<<order);
    }

    // output information
    kernel_printf("Setup Slub ok :\n");
    kernel_printf("\tcurrent slab cache size list:\n\t");
    for (order = 0; order < PAGE_SHIFT; order++) {
        kernel_printf("%x %x ", kmalloc_caches[order].objsize, (unsigned int)(&(kmalloc_caches[order])));
    }
    kernel_printf("\n");
}

void slub_format_slubpage(struct kmem_cache* cache, struct page* page){
    unsigned char* m = (unsigned char*)((page-pages)<<PAGE_SHIFT); // physical addr
    struct slub_head *s_head = (struct slub_head*)m;
    unsigned int remaining = 1<<PAGE_SHIFT;
    unsigned int *ptr;

    set_flag(page, _PAGE_SLUB);
    s_head->nr_objs = 0;
    do{
        ptr = (unsigned int*)(m+cache->offset);
        m += cache->size;
        *ptr = (unsigned int)m;
        remaining -= cache->size;
    }while(remaining >= cache->size);

    *ptr = (unsigned int)m & ~((1<<PAGE_SHIFT) - 1); // end position
    s_head->end_ptr = ptr;
    s_head->nr_objs = 0;
    cache->cpu.page = page;
    cache->cpu.freeobj = (void**)(*ptr + cache->offset);
    page->private = (unsigned int)(*(cache)->cpu.freeobj);
    page->virtual = (void*)cache;
}

void* slub_alloc(struct kmem_cache* cache){
    struct slub_head* s_head;
    void* object = 0;
    struct page* new;

    if(cache->cpu.freeobj) object = *(cache->cpu.freeobj);

check:
    if(is_bound((unsigned int)object, 1<<PAGE_SHIFT)){
        if(cache->cpu.page){
            // slub full
            list_add_tail(&(cache->cpu.page->list), &(cache->node.full));
        }
        if(list_empty(&(cache->node.partial))) goto new_slub;// find a new slub
        cache->cpu.page = container_of(cache->node.partial.next, struct page, list);
        list_del(cache->node.partial.next);
        object = (void*)(cache->cpu.page->private);
        cache->cpu.freeobj = (void**)((unsigned char*)object + cache->offset);
        goto check;
    }

    cache->cpu.freeobj = (void**)((unsigned char*)object + cache->offset);
    cache->cpu.page->private = (unsigned int)(*(cache->cpu.freeobj));
    s_head = (struct slub_head*)((cache->cpu.page - pages)<<PAGE_SHIFT);
    ++(s_head->nr_objs);

    if(is_bound(cache->cpu.page->private, 1<<PAGE_SHIFT)){
        list_add_tail(&(cache->cpu.page->list), &(cache->node.full));
        slub_init_kmem_cpu(&(cache->cpu));
    }
    return object;

new_slub:
    new = buddy_alloc_pages(0);
    if(!new){
        kernel_printf("ERROR: slub_alloc error!\n");
        while(1){}
    }
    kernel_printf("\n *** %x\n", new-pages);

    slub_format_slubpage(cache, new);
    object = *(cache->cpu.freeobj);
    goto check;
}

struct kmem_cache* slub_get_slub(unsigned int size) {
    unsigned int itop = PAGE_SHIFT;
    unsigned int i;
    unsigned int bf_num = (1 << (PAGE_SHIFT - 1));  // half page
    unsigned int bf_index = PAGE_SHIFT;             // record the best fit num & index

    // best fit
    for (i = 0; i < itop; i++) {
        if ((kmalloc_caches[i].objsize >= size) && (kmalloc_caches[i].objsize < bf_num)) {
            bf_num = kmalloc_caches[i].objsize;
            bf_index = i;
        }
    }
    return &(kmalloc_caches[bf_index]);
}

void* kmalloc(unsigned int size){
    struct kmem_cache* cache;
    struct page* page;

    if(!size) return 0;

    // if size > max size of slub, then use buddy
    if(size > kmalloc_caches[PAGE_SHIFT-1].objsize){
        // e.g. size=5k -> size=8k
        size += ((1<<PAGE_SHIFT)-1);
        size &= ~((1<<PAGE_SHIFT)-1);
        page = buddy_alloc_pages(size>>PAGE_SHIFT);
        return ;
    }

    cache = slub_get_slub(size);
    if(!cache){
        kernel_printf("ERROR:kmalloc error!\n");
        while(1){}
    }
    return slub_alloc(cache);
}

void slub_free(struct kmem_cache* cache, void* obj){
    struct page* page = pages + ((unsigned int)obj >> PAGE_SHIFT);
    struct slub_head* s_head = (struct slub_head*)((page - pages)<<PAGE_SHIFT);

    unsigned int* ptr;

    // check if s_head has objects
    if(!(s_head->nr_objs)){
        kernel_printf("ERROR:slub_free error!\n");
        while(1){}
    }

    ptr = (unsigned int*)((unsigned char*)obj+cache->offset);
    *ptr = *((unsigned int*)(s_head->end_ptr));
    *((unsigned int*)(s_head->end_ptr)) = (unsigned int)obj;
    --(s_head->nr_objs);

    if(list_empty(&(page->list))) return;

    if(!(s_head->nr_objs)){
        buddy_free_pages(page, 0);
        return;
    }

    list_del_init(&(page->list));
    list_add_tail(&(page->list), &(cache->node.partial));

}

void kfree(void* obj){
    struct page* page;

    page = pages + ((unsigned int)obj >> PAGE_SHIFT);
    if(!has_flag(page, _PAGE_SLUB)){
        return buddy_free_pages(page, page->bplevel);
    }
    return slub_free(page->virtual, obj);
}

/*
// syscall kmalloc
// parameter a0=size, return v0 = start addr
void syscall20(unsigned int status, unsigned int cause, context* pt_context){
    unsigned int size;
    void* addr;

    size = pt_context->a0;
    addr = kmalloc(size);
    pt_context->v0 = addr;

}

// syscall kree
// parameter a0=start
void syscall21(unsigned int status, unsigned int cause, context* pt_context){
    void* obj;
    obj=pt_context->a0;
    kfree(obj);
}
*/
