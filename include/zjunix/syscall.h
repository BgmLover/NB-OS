#ifndef _ZJUNIX_SYSCALL_H
#define _ZJUNIX_SYSCALL_H

#include <zjunix/task.h>

typedef void (*sys_fn)(unsigned int status, unsigned int cause, context* pt_context);

extern sys_fn syscalls[256];

void init_syscall();
void syscall(unsigned int status, unsigned int cause, context* pt_context);
void register_syscall(int index, sys_fn fn);
void syscall_putchar_0(unsigned int status, unsigned int cause, context* pt_context);
// void putchar(int ch, int fc, int bg);
void syscall_strcmp_1(unsigned int status, unsigned int cause, context* pt_context);
// int strcmp(const char* dest, const char* src) ;
void syscall_clear_screen_2(unsigned int status, unsigned int cause, context* pt_context);
// void clear_screen(int scope);
void syscall_printf_3(unsigned int status, unsigned int cause, context* pt_context);
// void printf(const char *format, ...);
void syscall_puts_4(unsigned int status, unsigned int cause, context* pt_context);
// void puts(const char *s, int fc, int bg);
void syscall_fork_31(unsigned int status, unsigned int cause, context* pt_context);
void syscall_exec_32(unsigned int status, unsigned int cause, context* pt_context);
void syscall_kill_33(unsigned int status, unsigned int cause, context* pt_context);
void syscall_exit_34(unsigned int status, unsigned int cause, context* pt_context);
void syscall_print_tasks_35(unsigned int status, unsigned int cause, context* pt_context);

void syscall_shm_get_36(unsigned int status, unsigned int cause, context* pt_context);
void syscall_shm_mount_37(unsigned int status, unsigned int cause, context* pt_context);
void syscall_shm_write_38(unsigned int status, unsigned int cause, context* pt_context);
void syscall_shm_read_39(unsigned int status, unsigned int cause, context* pt_context);
void syscall_getcurrent_pcb_40(unsigned int status, unsigned int cause, context* pt_context);
void syscall_shm_test_41(unsigned int status, unsigned int cause, context* pt_context);

void syscall_fopen_51(unsigned int status, unsigned int cause, context* pt_context);
void syscall_fclose_52(unsigned int status, unsigned int cause, context* pt_context);
void syscall_fread_53(unsigned int status, unsigned int cause, context* pt_context);
void syscall_fwrite_54(unsigned int status, unsigned int cause, context* pt_context);
void syscall_cat_55(unsigned int status, unsigned int cause, context* pt_context);
void syscall_ls_56(unsigned int status, unsigned int cause, context* pt_context);
void syscall_myvi_57(unsigned int status, unsigned int cause, context* pt_context);



#endif // ! _ZJUNIX_SYSCALL_H
