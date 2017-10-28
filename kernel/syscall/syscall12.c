#include <driver/vga.h>
//#include <syscall11.h>
#include <arch.h>
#include <zjunix/syscall.h>

int fs_global;
int bg_global;
void syscall12(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3) {
    kernel_snow();
}