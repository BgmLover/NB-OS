#ifndef _USERS_USTDIO_H
#define _USERS_USTDIO_H

void putchar(int ch, int fc, int bg);
int strcmp(const char* dest, const char* src) ;
void clear_screen(int scope);
void printf(const char *format, ...);
void puts(const char *s, int fc, int bg);
void* malloc(unsigned int size);
void free(void* obj);

void fopen(FILE *file,unsigned char *filename);
void fclose(FILE *file);
void fread(FILE *file,unsigned char *buffer,unsigned long count);
void fwrite(FILE *file,unsigned char *buffer,unsigned long count);
void cat(unsigned char *path);
void listfile(char *para);
void vi(char *filename);

#endif