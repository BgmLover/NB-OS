#include"vma.h"
#include<zjunix/slab.h>
#include<page.h>
#include<driver/vga.h>
struct vma *new_vma(unsigned int va, unsigned int vend)
{
	struct vma *tmp;

	tmp = kmalloc(sizeof(struct vma));
	if (!tmp) {
		return NULL;
	}

	INIT_LIST_HEAD(&(tmp->node));
	tmp->start = va;
	tmp->vend = vend;
	tmp->cnt = ((vend - va) >> PAGE_SHIFT) + 1;

	return tmp;
}

void del_vma(struct vma *vma)
{
	list_del_init(&(vma->node));
	kfree(vma);
}

/*
* 将vma分成： vma->start~(va-1) 和 va~vma->vend 两个部分
* caller做检查
*/
struct vma * seprate_vma(struct vma *vma, unsigned int va)
{
	struct vma *new;

	if (!(new = new_vma(va, vma->vend))) {
		kernel_printf("seprate_vma : new_vma failed!\n");
		return NULL;
	}

	vma->vend = va - 1;
	list_add(&(new->node), vma->node.next);

	return new;
}

unsigned int verify_vma(PCB *task, unsigned int *va, unsigned int *vend, struct list_head **target)
{
	*target = NULL;
	(*va) &= ~(PAGE_SIZE - 1);
	(*vend) += (PAGE_SIZE - 1);
	(*vend) &= ~(PAGE_SIZE - 1);
	(*vend)--;

	if ((*va) < _KERNEL_VIRT_END) {
		kernel_printf("add_vmas : va(%x) inavailed (kernel space)!\n");
		return 1;
	} else if ((*va) < _TASK_CODE_START) {
		if ((*vend) >= _TASK_CODE_START) {
			kernel_printf("add_vmas : vma(%x:%x) exceed!\n",(*va), (*vend));
			return 1;
		} else {
			*target = &(task->stack_vma_head);
		}
	} else if ((*va) < _TASK_HEAP_END) {
		if ((*vend) >= _TASK_HEAP_END) {
			kernel_printf("add_vmas : vma(%x:%x) exceed!\n",(*va), (*vend));
			return 1;
		} else {
			*target = &(task->user_vma_head);
		}
	} else if ((*va) < ROM_START) {
		if ((*vend) >= ROM_START) {
			kernel_printf("add_vmas : vma(%x:%x) exceed!\n",(*va), (*vend));
			return 1;
		} else {
			*target = &(task->heap_vma_head);
		}
	} else { // >= ROM_START
		kernel_printf("add_vmas : va(%x) inavailed (rom/io space)!\n");
		return 1;
	}

	return 0;
}

/*
* return 1 on error, 0 success
*/
unsigned int add_vmas(PCB*task, unsigned int va, unsigned int size)
{
	struct list_head *target;
	struct list_head *pos;
	struct list_head *n;
	struct vma *tmp;
	struct list_head *prev;
	unsigned int vend = va + size;

	if (verify_vma(task, &va, &vend, &target))
		return 1;

	prev = target;
	list_for_each_safe(pos, n, target) {
		tmp = container_of(pos, struct vma, node);
		if ((va==tmp->start) && (vend==tmp->vend))
			goto ok;
		if (va < tmp->start) {
			if (vend < tmp->start) {
				if ((vend + 1) == tmp->start) {
					tmp->start = va;
					tmp->cnt = ((tmp->vend - tmp->start) >> PAGE_SHIFT) + 1;
					goto ok;
				} else {
					prev = tmp->node.prev;
					goto new;
				}
			} else if (vend >= tmp->vend) {
				del_vma(tmp);
				continue;
			} else {
				tmp->start = va;
				tmp->cnt = ((tmp->vend - tmp->start) >> PAGE_SHIFT) + 1;
				goto ok;
			}
		} else if (va < tmp->vend) {
			if (vend <= tmp->vend) {
				goto ok;
			} else {
				//va = tmp->vend + 1; // vend的低12bit全为1
				va = tmp->start;
				del_vma(tmp);
				continue;
			}
		} else {
			if ((va - 1) == tmp->vend) {
				va = tmp->start;
				del_vma(tmp);
				continue;
			} else {
				prev = &(tmp->node);
				continue;
			}
		}
	}

new:
	if (!(tmp = new_vma(va, vend))) {
		kernel_printf("insert_vma : new_vma failed!\n");
		return 1;
	}
	
	list_add(&(tmp->node), prev);
ok:
	return 0;
}

// 0成功   1失败
unsigned int delete_vmas(PCB*task, unsigned int va, unsigned int vend)

{
	struct list_head *pos, *p;
	struct vma *tmp, *n;
	struct list_head *list;

	if (verify_vma(task, &va, &vend, &list))
		return 1;

	list_for_each_safe(pos, p, list) {
		tmp = container_of(pos, struct vma, node);
		if (va < tmp->start) {
			return 1;
		} else if (va == tmp->start) {
			if (vend == tmp->vend) {
				del_vma(tmp);
				return 0;
			} else if (vend < tmp->vend) {
				tmp->start = vend + 1;
				return 0;
			} else {
				return 1;
			}
		} else if (va < tmp->vend) {
			if (vend == tmp->vend) {
				tmp->vend = va - 1;
				return 0;
			} else if (vend < tmp->vend) {
				n = seprate_vma(tmp, va);
				if (!n) {
					kernel_printf("delete_vmas : seprate_vma failed!\n");
					return 1;
				}
				tmp = n;
				n = seprate_vma(n, vend + 1);
				if (!n) {
					kernel_printf("delete_vmas : seprate_vma failed!\n");
					return 1;
				}
				del_vma(tmp);
				return 0;
			} else {
				return 1;
			}
		} else {
			continue;
		}
	}

	return 0;
}

/*
* 判断va所在的一个页面是否在task的vma链表中
* NULL 没找到 
*/
struct vma * find_vmas(PCB *task, unsigned int va)
{
	struct vma *v = NULL;
	struct vma *tmp;
	struct list_head *list;
	struct list_head *pos;
	unsigned int vend = va + PAGE_SIZE - 1;

	vend &= ~(PAGE_SIZE - 1);
	if (verify_vma(task, &va, &vend, &list))
		return NULL;

	list_for_each(pos, list) {
		tmp = container_of(pos, struct vma, node);
		if ((tmp->start <= va) && (tmp->vend >= vend)) {
			v = tmp;
			break;
		}
	}

	return v;
}

static void delete_each_vma(struct list_head *list) {
	struct list_head *pos, *n;
	struct vma *tmp;
	list_for_each_safe(pos, n, list) {
		tmp = container_of(pos, struct vma, node);
		del_vma(tmp);
	}
}

static unsigned int  copy_each_vma(struct list_head *old, struct list_head *new) {
	struct list_head *pos;
	struct vma *tmp;
	struct vma *t;
	INIT_LIST_HEAD(new);
	list_for_each(pos, old) {
		tmp = container_of(pos, struct vma, node);
		
		t = kmalloc(sizeof(struct vma));
		if (!t) {
			goto failed;
		}
		
		*t = *tmp;
		INIT_LIST_HEAD(&(t->node));
		list_add_tail(&(t->node), new);
	}
	return 0;

failed:
	delete_each_vma(new);
	return 1;
}

unsigned int copy_vmas(PCB *old, PCB *new) 
{
	if (copy_each_vma(&(old->user_vma_head), &(new->user_vma_head)))
		goto failed1;

	if (copy_each_vma(&(old->stack_vma_head), &(new->stack_vma_head)))
		goto failed2;

	if (copy_each_vma(&(old->heap_vma_head), &(new->heap_vma_head)))
		goto failed3;

	return 0;
failed3:
	delete_each_vma(&(new->stack_vma_head));
failed2:
	delete_each_vma(&(new->user_vma_head));
failed1:
	kernel_printf("copy_vmas failed!\n");
	return 1;
}

void delete_task_vmas(PCB *task)
{
	delete_each_vma(&(task->heap_vma_head));
	delete_each_vma(&(task->stack_vma_head));
	delete_each_vma(&(task->user_vma_head));
}