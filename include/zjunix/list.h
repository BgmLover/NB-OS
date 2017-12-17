#ifndef _LIST_H
#define _LIST_H

struct list_head {
    struct list_head *prev;
    struct list_head *next;
};

#define LIST_POISON1 (void *)0x10101010
#define LIST_POISON2 (void *)0x20202020


#define LIST_HEAD_INIT(name) \
    { &(name), &(name) }

#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list) {
    list->prev = list;
    list->next = list;
}

static inline void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next) {
    new->next = next;
    new->prev = prev;
    prev->next = new;
    next->prev = new;
}
//把节点插入到另一个节点后
static inline void list_add(struct list_head *new, struct list_head *head) { __list_add(new, head, head->next); }
//节点插入到另一个节点前
static inline void list_add_tail(struct list_head *new, struct list_head *head) { __list_add(new, head->prev, head); }
//删除节点
static inline void __list_del(struct list_head *prev, struct list_head *next) {
    prev->next = next;
    next->prev = prev;
}

static inline void list_del(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
    entry->prev = LIST_POISON1;
    entry->next = LIST_POISON2;
}
//删除并初始化该节点
static inline void list_del_init(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
    INIT_LIST_HEAD(entry);
}
//把节点移动到另一个节点后
static inline void list_move(struct list_head *entry, struct list_head *head) {
    __list_del(entry->prev, entry->next);
    list_add(entry, head);
}
//把节点移动到另一个节点前
static inline void list_move_tail(struct list_head *entry, struct list_head *head) {
    __list_del(entry->prev, entry->next);
    list_add_tail(entry, head);
}
//查看该节点是否是链表中唯一节点
static inline unsigned int list_empty(struct list_head *head) { return head->next == head; }

//查找节点是否在链表中
static inline unsigned int list_find(struct list_head *node,struct list_head* head)
{
    struct list_head *pos;
    int index=1;
    for(pos=head->next;pos!=head;pos=pos->next)
    {
        if(pos==node)
            return index;
        else(index++);
    }
    return 0;
}
#define list_entry(ptr, type, member) container_of(ptr, type, member)
//遍历
#define list_for_each(pos, head) for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

#endif
