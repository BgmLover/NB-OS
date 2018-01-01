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



#endif // ! _ZJUNIX_SYSCALL_H
