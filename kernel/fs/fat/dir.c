#include "dir.h"
#include <zjunix/fs/fscache.h>
#include "fat.h"
#include "utils.h"

/* used to find or create a directory entry */
#define DIR_DATA_BUF_NUM 4
FILE dir_find;
extern BUF_512 dir_data_buf[DIR_DATA_BUF_NUM];
extern u32 dir_data_clock_head;
extern struct fs_info fat_info;

/* open directory */
u32 fs_open_dir(FS_FAT_DIR *dir, u8 *filename) {
    u32 index;
    u32 i;

    if (filename[0] != '/')
        goto fs_open_dir_err;

    //从文件内部当前指向的扇区开始读起
    dir->cur_sector = fs_dataclus2sec(2);
    dir->loc = 0;
    dir->sec = 1;

    //打开根目录
    index = fs_read_512(dir_data_buf, dir->cur_sector, &dir_data_clock_head, DIR_DATA_BUF_NUM);
    if (index == 0xffffffff)
        goto fs_open_dir_err;

    /* if not root directory */
    if (filename[1] != 0) {
        for (i = 0; i < 256; i++)
            dir_find.path[i] = 0;
        for (i = 0; i < 256 && filename[i] != 0; i++)
            dir_find.path[i] = filename[i];

        //如果文件不存在，则返回错误
        if (fs_find(&dir_find) == 1)
            goto fs_open_dir_err;

        if (dir_find.dir_entry_pos == 0xFFFFFFFF)
            goto fs_open_dir_err;

        //如果文件是一个文件夹，返回错误
        if ((dir_find.entry.data[11] & 0x10) == 0)
            goto fs_open_dir_err;

        dir->cur_sector = fs_dataclus2sec(get_start_cluster(&dir_find));

        //打开文件第一个簇
        index = fs_read_512(dir_data_buf, dir->cur_sector, &dir_data_clock_head, DIR_DATA_BUF_NUM);
        if (index == 0xffffffff)
            goto fs_open_dir_err;
    }

    return 0;

fs_open_dir_err:
    return 1;
}

//读文件的路径
u32 fs_read_dir(FS_FAT_DIR *dir, u8 *buf) {
    u32 sec;
    u32 i;
    u32 index;
    u32 k;
    u32 next_clus;

    index = fs_read_512(dir_data_buf, dir->cur_sector, &dir_data_clock_head, DIR_DATA_BUF_NUM);
    if (index == 0xffffffff)
        goto fs_read_dir_err;
    
    //循环读取文件路径
    while (1) {
        for (sec = dir->sec; sec <= fat_info.BPB.attr.sectors_per_cluster; sec++) {
            //在当前簇中查找文件入口
            for (i = dir->loc; i < 512; i += 32) {
                if (*(dir_data_buf[index].buf + i) == 0)
                    goto after_fs_read_dir_nomore;

                //暂不考虑长文件名
                if ((*(dir_data_buf[index].buf + i) != 0xE5) && ((*(dir_data_buf[index].buf + i + 11) & 0x08) == 0)) {
                    dir->loc = i + 32;

                    for (k = 0; k < 32; k++)
                        buf[k] = *(dir_data_buf[index].buf + i + k);

                    goto after_fs_read_dir;
                }
            }

            //当前扇区的下一个簇
            if (sec < fat_info.BPB.attr.sectors_per_cluster) {
                dir->sec = sec + 1;
                dir->loc = 0;
                dir->cur_sector = dir_data_buf[index].cur + sec;

                index = fs_read_512(dir_data_buf, dir->cur_sector, &dir_data_clock_head, DIR_DATA_BUF_NUM);
                if (index == 0xffffffff)
                    goto fs_read_dir_err;
            } else {
                //读取当前目录中的下一个扇区
                if (get_fat_entry_value(dir_data_buf[index].cur - fat_info.BPB.attr.sectors_per_cluster + 1, &next_clus) == 1)
                    goto fs_read_dir_err;

                if (next_clus <= fat_info.total_data_clusters + 1) {
                    dir->sec = 1;
                    dir->loc = 0;
                    dir->cur_sector = fs_dataclus2sec(next_clus);

                    index = fs_read_512(dir_data_buf, dir->cur_sector, &dir_data_clock_head, DIR_DATA_BUF_NUM);
                    if (index == 0xffffffff)
                        goto fs_read_dir_err;
                } else
                    goto after_fs_read_dir_nomore;
            }
        }
    }

after_fs_read_dir:
    return 0;
after_fs_read_dir_nomore:
    return 0xffffffff;
fs_read_dir_err:
    return 1;
}