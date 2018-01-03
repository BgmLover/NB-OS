#ifndef _USERS_USTDIO_H
#define _USERS_USTDIO_H

#include"fat.h"

typedef unsigned char* va_list;
#define va_start(ap, v) (ap = (va_list)&v + _INTSIZEOF(v))
#define _INTSIZEOF(n) ((sizeof(n) + sizeof(unsigned int) - 1) & ~(sizeof(unsigned int) - 1))


void uputchar(int ch, int fc, int bg);
void putchar_at(int ch,int row,int col);
int ugetchar();
void set_cursor();
int ustrcmp(const char* dest, const char* src) ;
void uclear_screen(int scope);
void uprintf(const char *format, ...);
void uputs(const char *s, int fc, int bg);
void* umalloc(unsigned int size);
void ufree(void* obj);

void ufopen(FILE *file,unsigned char *filename);
void ufclose(FILE *file);
void ufread(FILE *file,unsigned char *buffer,unsigned long count);
void ufwrite(FILE *file,unsigned char *buffer,unsigned long count);
void ucat(unsigned char *path);
void ulistfile(char *para);
void uvi(char *filename);


int ukill();
int exec();
int print_proc();

int* getvga();
void demo_create();

void parse_cmd();


int bootmap_info();
int buddy_info();
int get_time();

#endif