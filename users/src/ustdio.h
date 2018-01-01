#ifndef _USERS_USTDIO_H
#define _USERS_USTDIO_H

void putchar(int ch, int fc, int bg);
int strcmp(const char* dest, const char* src) ;
void clear_screen(int scope);
void printf(const char *format, ...);
void puts(const char *s, int fc, int bg);
void* malloc(unsigned int size);
void free(void* obj);


#endif