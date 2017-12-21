#ifndef _ARCH_H
#define _ARCH_H

//	machine params

#define MACHINE_MMSIZE 512 * 1024 * 1024     // 512MB
#define MACHINE_SDSIZE 16 * 1024 * 1024 * 2  // 32M Sectors
#define CHAR_VRAM_SIZE 128 * 32 * 4          // 128*32*4
#define PAGE_TABLE_SIZE 256 * 1024           // 4MB
#define GRAPHIC_VRAM_SIZE 1024 * 512 * 4     // 1024*512*4 b-g-r

/*
用户态进程虚拟地址划分
0 ~ 0x8000,0000-1  user部分
0x8000,0000 ~ 0xA000,0000-1 kernel Unmapped    这一部分直接映射物理内存对应位置，预留16M空间
0xA000,0000 ~ 0xC000,0000-1 kernel Unmapped\Uncache\Rom\IO
0xC000,0000 ~ 0x10000,0000-1 kernel Mapped


*/

//	Virtual Memory
#define BIOS_ENTRY 0xbfc00000
#define KERNEL_STACK_BOTTOM 0x81000000//内核部分预分配16M空间，用户进程不能访问该部分的物理内存
#define KERNEL_CODE_ENTRY 0x80001000
#define KERNEL_ENTRY 0x80000000
#define USER_ENTRY 0x00000000
#define USER_CODE_ENTRY 0x00001000
extern unsigned int* const CHAR_VRAM;
extern unsigned int* const GRAPHIC_VRAM;
extern unsigned int* const GPIO_SWITCH;     // switch read-only
extern unsigned int* const GPIO_BUTTON;     // button read-only
extern unsigned int* const GPIO_SEG;        // Seg R/W
extern unsigned int* const GPIO_LED;        // LED R/W
extern unsigned int* const GPIO_PS2_DATA;   // PS/2 data register, R/W
extern unsigned int* const GPIO_PS2_CTRL;   // PS/2 control register, R/W
extern unsigned int* const GPIO_UART_DATA;  // UART data register, R/W
extern unsigned int* const GPIO_UART_CTRL;  // UART control register, R/W
extern unsigned int* const GPIO_CURSOR;     // Cursor 8-bit frequency 8-bit row 8-bit col
extern unsigned int* const VGA_MODE;        // enable graphic mode

// kernel sp
extern volatile unsigned int kernel_sp;

// PS/2 control register:
//  [5:0]: RX buffer load(R)
//  [13:0]:TX buffer load(R)
//  [18:16]: Error code(R)
//  [31]:  Interrupt enable(RW)

//  UART control register:
//  [7:0]: RX buffer load(R)
//  [15:8]:TX buffer load(R)
//  [18:16]: baud rate(RW)
//  [31]:  Interrupt enable(RW)

unsigned int get_phymm_size();

#endif
