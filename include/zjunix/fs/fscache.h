#ifndef _ZJUNIX_FSCACHE_H
#define _ZJUNIX_FSCACHE_H

#include <zjunix/type.h>

//4K缓存块
typedef struct buf_4k {
    //用于存储数据
    unsigned char buf[4096];
    //当前块对应当前磁盘上的第几个sector
    unsigned long cur;
    //reference位和dirty位
    unsigned long state;
} BUF_4K;

//512缓存块
typedef struct buf_512 {
    //用于存储数据
    unsigned char buf[512];
    //当前块对应当前磁盘上的第几个sector
    unsigned long cur;
    //reference位和dirty位
    unsigned long state;
} BUF_512;

u32 fs_victim_4k(BUF_4K *buf, u32 *clock_head, u32 size);
u32 fs_write_4k(BUF_4K *f);
u32 fs_read_4k(BUF_4K *f, u32 FirstSectorOfCluster, u32 *clock_head, u32 size);
u32 fs_clr_4k(BUF_4K *buf, u32 *clock_head, u32 size, u32 cur);

u32 fs_victim_512(BUF_512 *buf, u32 *clock_head, u32 size);
u32 fs_write_512(BUF_512 *f);
u32 fs_read_512(BUF_512 *f, u32 FirstSectorOfCluster, u32 *clock_head, u32 size);
u32 fs_clr_512(BUF_512 *buf, u32 *clock_head, u32 size, u32 cur);


#endif // ! _ZJUNIX_FSCACHE_H