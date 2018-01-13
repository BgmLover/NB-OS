#ifndef _ZJUNIX_FSCACHE_H
#define _ZJUNIX_FSCACHE_H

#include <zjunix/type.h>

//4K的buffer
typedef struct buf_4k {
    unsigned char buf[4096];
    unsigned long cur;
    unsigned long state;
} BUF_4K;

//512b的buffer
typedef struct buf_512 {
    unsigned char buf[512];
    unsigned long cur;
    unsigned long state;
} BUF_512;

//寻找空闲的4K大小的buffer块
u32 fs_victim_4k(BUF_4K *buf, u32 *clock_head, u32 size);
//写4K大小的buffer
u32 fs_write_4k(BUF_4K *f);
//读4K大小的buffer
u32 fs_read_4k(BUF_4K *f, u32 FirstSectorOfCluster, u32 *clock_head, u32 size);
//清空4Kbuffer
u32 fs_clr_4k(BUF_4K *buf, u32 *clock_head, u32 size, u32 cur);

//寻找空闲的512b大小的buffer块
u32 fs_victim_512(BUF_512 *buf, u32 *clock_head, u32 size);
//写512b大小的buffer块
u32 fs_write_512(BUF_512 *f);
//读512b大小的buffer块
u32 fs_read_512(BUF_512 *f, u32 FirstSectorOfCluster, u32 *clock_head, u32 size);
//清空512b的buffer
u32 fs_clr_512(BUF_512 *buf, u32 *clock_head, u32 size, u32 cur);


#endif // ! _ZJUNIX_FSCACHE_H