#include <exc.h>
#include <arch.h>
#include <zjunix/fs/fat.h>
#include <zjunix/syscall.h>
#include <zjunix/slub.h>
#include <zjunix/sche.h>
#include <driver/vga.h>
#include <zjunix/utils.h>
#include <zjunix/fs/myvi.h>
#include <zjunix/shm.h>
#include <zjunix/task.h>
#include <zjunix/sche.h>
#include "syscall4.h"

sys_fn syscalls[256];

void init_syscall() {
    register_exception_handler(8, syscall);

    //register all syscalls here
    // register_syscall(4, syscall4);
    register_syscall(0, syscall_putchar_0);
    register_syscall(1, syscall_strcmp_1);
    register_syscall(2, syscall_clear_screen_2);
    register_syscall(3, syscall_printf_3);
    register_syscall(4, syscall_puts_4);
    register_syscall(21, syscall_kmalloc_21);
    register_syscall(22, syscall_kfree_22);
    
    register_syscall(31,syscall_fork_31);
    register_syscall(32,syscall_exec_32);
    register_syscall(33,syscall_kill_33);
    register_syscall(34,syscall_exit_34);
    register_syscall(35,syscall_print_tasks_35);

    register_syscall(36, syscall_shm_get_36);
    register_syscall(37, syscall_shm_mount_37);
    register_syscall(38, syscall_shm_write_38);
    register_syscall(39, syscall_shm_read_39);
    register_syscall(40, syscall_getcurrent_pcb_40);
    register_syscall(41,syscall_shm_test_41);

	register_syscall(51,syscall_fopen_51);
    register_syscall(52,syscall_fclose_52);
    register_syscall(53,syscall_fread_53);
    register_syscall(54,syscall_fwrite_54);
    register_syscall(55,syscall_cat_55);
    register_syscall(56,syscall_ls_56);
    register_syscall(57,syscall_myvi_57);
}

void syscall(unsigned int status, unsigned int cause, context* pt_context) {
    unsigned int code;
    code = pt_context->v0;
    pt_context->epc += 4;
    if (syscalls[code]) {
        syscalls[code](status, cause, pt_context);
    }
}

void register_syscall(int index, sys_fn fn) {
    index &= 255;
    syscalls[index] = fn;
}

// a0 = ch, a1 = fc, a2 = bg
void syscall_putchar_0(unsigned int status, unsigned int cause, context* pt_context){
    unsigned int ch = (unsigned int)pt_context->a0;
    unsigned int fc = (unsigned int)pt_context->a1;
    unsigned int bg = (unsigned int)pt_context->a2;
    kernel_putchar(ch, fc, bg);
}
/*
void putchar(int ch, int fc, int bg){
    unsigned int a0 = (unsigned int)ch;
    unsigned int a1 = (unsigned int)fc;
    unsigned int a2 = (unsigned int)bg;
    asm volatile(
        "move $a0,%0\n\t"
        "move $a1,%1\n\t"
        "move $a2,%2\n\t"
        "addi $v0,$zero,0\n\t"
        "syscall\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0),"r"(a1),"r"(a2)
    );
}
*/
// a0 = dest, a1 = src
void syscall_strcmp_1(unsigned int status, unsigned int cause, context* pt_context) {
    unsigned int res;
    const char* dest = (const char*)pt_context->a0;
    const char* src = (const char*)pt_context->a1;
    res = kernel_strcmp(dest, src);
    pt_context->v0 = res;
}
/*
int strcmp(const char* dest, const char* src) {
    unsigned int a0 = (unsigned int)dest;
    unsigned int a1 = (unsigned int)src;
    unsigned int v0;
    asm volatile(
        "move $a0,%1\n\t"
        "move $a1,%2\n\t"
        "addi $v0,$zero,1\n\t"
        "syscall\n\t"
        "nop\n\t"
        "move %0,$v0\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :"=r"(v0)
        :"r"(a0),"r"(a1)
    );

    return v0;
}
*/
// a0 = scope
void syscall_clear_screen_2(unsigned int status, unsigned int cause, context* pt_context){
    int scope = (int)pt_context->a0;
    kernel_clear_screen(scope);
}
/*
void clear_screen(int scope){
    int a0 = scope;
    asm volatile(
        "move $a0,%0\n\t"
        "addi $v0,$zero,2\n\t"
        "syscall\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0)
    );
}*/

// a0 = const char* format, a1 = ap
void syscall_printf_3(unsigned int status, unsigned int cause, context* pt_context){
    int cnt = 0;
    const char* format = (const char*)pt_context->a0;
    va_list ap = (va_list)pt_context->a1;

// kernel_printf("format=%x,ap=%x\n",format,ap);
    cnt = kernel_vprintf(format, ap);
    va_end(ap);
}

/*
void printf(const char *format, ...){
    unsigned int a0;
    unsigned int a1;
    va_list ap;
    va_start(ap, format);
    a0 = (unsigned int)format;
    a1 = (unsigned int)ap;
    asm volatile(
        "move $a0,%0\n\t"
        "move $a1,%1\n\t"
        "addi $v0,$zero,3\n\t"
        "syscall\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0),"r"(a1)
    );
}
*/

 //a0 = s, a1 = fc, a2 = bg
void syscall_puts_4(unsigned int status, unsigned int cause, context* pt_context){
    const char* s = (const char*)pt_context->a0;
    int fc = (int)pt_context->a1;
    int bg = (int)pt_context->a2;
    kernel_puts(s, fc, bg);

}
/*
void puts(const char *s, int fc, int bg) {
    unsigned int a0, a1, a2;
    a0 = (unsigned int)s;
    a1 = (unsigned int)fc;
    a2 = (unsigned int)bg;
    asm volatile(
        "move $a0,%0\n\t"
        "move $a1,%1\n\t"
        "move $a2,%2\n\t"
        "addi $v0,$zero,4\n\t"
        "syscall\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0),"r"(a1),"r"(a2)
    );
}
*/

void syscall_fopen_51(unsigned int status, unsigned int cause, context* pt_context) 
{
    //a0存放文件指针（FILE *），a1存放文件名指针（unsigned long *）
    FILE *file=(FILE*)pt_context->a0;
    unsigned char *filename=(unsigned char*)pt_context->a1;
    fs_open(file,filename);
}

void syscall_fclose_52(unsigned int status, unsigned int cause, context* pt_context)
{
    //a0存放文件指针
    FILE *file=(FILE*)pt_context->a0;
    fs_close(file);
}

void syscall_fread_53(unsigned int status, unsigned int cause, context* pt_context)
{
    //a0存放文件指针，a1存放buffer指针（unsigned char*），a2存放要读取的字节数（unsigned long *）
    FILE *file=(FILE*)pt_context->a0;
    unsigned char *buffer=(unsigned char*)pt_context->a1;
    unsigned long count=(unsigned long)pt_context->a2;
    fs_read(file,buffer,count);
}

void syscall_fwrite_54(unsigned int status, unsigned int cause, context* pt_context)
{
    //a0存放文件指针。a1存放buffer指针，a2存放要写的字节数
    FILE *file=(FILE*)pt_context->a0;
    unsigned char *buffer=(unsigned char*)pt_context->a1;
    unsigned long count=(unsigned long)pt_context->a2;
    fs_write(file,buffer,count);
}

void syscall_cat_55(unsigned int status, unsigned int cause, context* pt_context)
{
    //a0存放要显示的文件的绝对路径
    unsigned char *path=(unsigned char*)pt_context->a0;
    fs_cat(path);
}

void syscall_ls_56(unsigned int status, unsigned int cause, context* pt_context)
{
    //a0存放目标文件夹
    char *para=(char*)pt_context->a0;
    ls(para);
}

void syscall_myvi_57(unsigned int status, unsigned int cause, context* pt_context)
{
    //a0存放要编辑的文件名(char*)
    char *filename=(char*)pt_context->a0;
    // myvi(filename);
}

void syscall_fork_31(unsigned int status, unsigned int cause, context* pt_context)
{
    context*args=(context*)(pt_context->a0);
    PCB*parent=(PCB*)(pt_context->a1);
    do_fork(args,parent);
}
void syscall_exec_32(unsigned int status, unsigned int cause, context* pt_context)
{
    char*filename=(char*)(pt_context->a0);
    char*taskname=(char*)(pt_context->a1);
    exec(filename,taskname);
}
void syscall_kill_33(unsigned int status, unsigned int cause, context* pt_context)
{
    unsigned int asid=pt_context->a0;
    del_task(asid);
}
void syscall_exit_34(unsigned int status, unsigned int cause, context* pt_context)
{

}
void syscall_print_tasks_35(unsigned int status, unsigned int cause, context* pt_context)
{
    print_tasks();
}

void syscall_getcurrent_pcb_40(unsigned int status, unsigned int cause, context* pt_context)
{
    PCB *current=get_current_pcb();
    pt_context->v0 = (unsigned int)current;
}
