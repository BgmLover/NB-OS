#include"ustdio.h"
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

void* malloc(unsigned int size){
    unsigned int a0 = size;
    unsigned int v0;
    asm volatile(
        "move $a0,%1\n\t"
        "addi $v0,$zero,21\n\t"
        "syscall\n\t"
        "nop\n\t"
        "move %0,$v0\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :"=r"(v0)
        :"r"(a0)
    );
    v0 &= 0x7fffffff;
    return (void*)v0;
}

void free(void* obj){
    unsigned int a0 = (unsigned int)obj;
    a0 |= 0x80000000;
    asm volatile(
        "move $a0,%0\n\t"
        "addi $v0,$zero,22\n\t"
        "syscall\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0)
    );
}

void fopen(FILE *file,unsigned char *filename)
{
    unsigned int a0=(unsigned int)file;
    unsigned int a1=(unsigned int)filename;

    asm volatile(
        "move $a0,%0\n\t"
        "move $a1,%1\n\t"
        "addi $v0,$zero,51\n\t"
        "syscall 51\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0),"r"(a1)
    );
}

void fclose(FILE *file)
{
    unsigned int a0=(unsigned int)file;

    asm volatile(
        "move $a0,%0\n\t"
        "addi $v0,$zero,52\n\t"
        "syscall 52\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0)
    );
}

void fread(FILE *file,unsigned char *buffer,unsigned long count)
{
    unsigned int a0=(unsigned int)file;
    unsigned int a1=(unsigned int)buffer;
    unsigned int a2=(unsigned int)count;

    asm volatile(
        "move $a0,%0\n\t"
        "move $a1,%1\n\t"
        "move $a2,%2\n\t"
        "addi $v0,$zero,53\n\t"
        "syscall 53\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0),"r"(a1),"r"(a2)
    );
}

void fwrite(FILE *file,unsigned char *buffer,unsigned long count)
{
    unsigned int a0=(unsigned int)file;
    unsigned int a1=(unsigned int)buffer;
    unsigned int a2=(unsigned int)count;

    asm volatile(
        "move $a0,%0\n\t"
        "move $a1,%1\n\t"
        "move $a2,%2\n\t"
        "addi $v0,$zero,54\n\t"
        "syscall 54\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0),"r"(a1),"r"(a2)
    );
}

void cat(unsigned char *path)
{
    unsigned int a0=(unsigned int)path;

    asm volatile(
        "move $a0,%0\n\t"
        "addi $v0,$zero,55\n\t"
        "syscall 55\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0)
    );
}

void listfile(char *para)
{
    unsigned int a0=(unsigned int)para;

    asm volatile(
        "move $a0,%0\n\t"
        "addi $v0,$zero,56\n\t"
        "syscall 56\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0)
    );
}

void vi(char *filename)
{
    unsigned int a0=(unsigned int)filename;

    asm volatile(
        "move $a0,%0\n\t"
        "addi $v0,$zero,57\n\t"
        "syscall 57\n\t"
        "nop\n\t"
        "jr $ra\n\t"
        "nop\n\t"
        :
        :"r"(a0)
    );
}
