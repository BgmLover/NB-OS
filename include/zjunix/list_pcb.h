#ifndef _LIST_PCB_H
#define _LIST_PCB_H

#include "task.h"
#include <driver/vga.h>
#include <debug.h>
static inline void INIT_LIST_PCB(struct list_pcb *list)
{
    #ifdef LIST_PCB_DEBUG
    kernel_printf("address of list:%x\n",list);
    #endif
    list->prev=list;
    list->next=list;
   
}
static inline void __list_pcb_add(list_pcb *new, list_pcb *prev, list_pcb *next) {
    new->next = next;
    new->prev = prev;
    prev->next = new;
    next->prev = new;
}
//把节点插入到另一个节点后
static inline void list_pcb_add(list_pcb *new, list_pcb *head) { __list_pcb_add(new, head, head->next); }
//节点插入到另一个节点前
static inline void list_pcb_add_tail(list_pcb *new, list_pcb *head) { __list_pcb_add(new, head->prev, head); }
//删除节点
static inline void __list_pcb_del(list_pcb *prev, list_pcb *next) {
    prev->next = next;
    next->prev = prev;
}
static inline void list_pcb_del_init(list_pcb *entry) {
    __list_pcb_del(entry->prev, entry->next);
    INIT_LIST_PCB(entry);
}
static inline void list_pcb_move(list_pcb *entry, list_pcb *head) {
    __list_pcb_del(entry->prev, entry->next);
    list_pcb_add(entry, head);
}
//把节点移动到另一个节点前
static inline void list_pcb_move_tail(list_pcb *entry, list_pcb *head) {
    __list_pcb_del(entry->prev, entry->next);
    list_pcb_add_tail(entry, head);
}
//查看该节点是否是链表中唯一节点
static inline unsigned int list_pcb_empty(list_pcb *head) { return head->next == head; }

#endif