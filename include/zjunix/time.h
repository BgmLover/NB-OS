#ifndef _ZJUNIX_TIME_H
#define _ZJUNIX_TIME_H

// Put current time into buffer, at least 8 char size
void get_time(char* buf, int len);
void set_second(unsigned int second);
void init_time();
void set_date(char *date);

#endif // ! _ZJUNIX_TIME_H
