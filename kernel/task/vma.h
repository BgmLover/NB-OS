#ifndef _VMA_H
#define _VMA_H

#include <zjunix/list.h>
#include "task.h"

struct vma {
	struct list_head node;
	 unsigned int start; // page-aligned
	 unsigned int cnt; // nr of pages
	 unsigned int vend; // auto calculate: start + cnt<<PAGE_SIZE -1;
};

extern unsigned int add_vmas(PCB *task, unsigned int va, unsigned int size);
extern unsigned int delete_vmas(PCB *task, unsigned int va, unsigned int vend);
extern struct vma * find_vmas(PCB *task, unsigned int va); 
extern unsigned int copy_vmas(PCB *old, PCB *new);
extern void delete_task_vmas(PCB *task);


#endif
