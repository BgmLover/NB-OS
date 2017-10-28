#include <driver/vga.h>
//#include <syscall10.h>
#include <arch.h>
#include <zjunix/syscall.h>

int fs_global;
int bg_global;
void syscall10(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3) {
    fs_global=VGA_YELLOW;
    bg_global=VGA_BLACK;
}