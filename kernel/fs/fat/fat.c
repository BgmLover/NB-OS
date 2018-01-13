#include "fat.h"
#include <driver/vga.h>
#include <zjunix/log.h>
#include "utils.h"
#include <driver/ps2.h>
#include <zjunix/fs/fat.h>
#include <page.h>

#ifdef FS_DEBUG
#include <intr.h>
#include <zjunix/log.h>
#include "debug.h"
#endif  // ! FS_DEBUG

//文件缓冲块的clock head
u32 fat_clock_head = 0;
BUF_512 fat_buf[FAT_BUF_NUM];

u8 filename11[13];
u8 new_alloc_empty[PAGE_SIZE];

#define DIR_DATA_BUF_NUM 4
BUF_512 dir_data_buf[DIR_DATA_BUF_NUM];
u32 dir_data_clock_head = 0;

struct fs_info fat_info;

u32 init_fat_info() {
    u8 meta_buf[512];

    //初始化文件的缓冲块
    kernel_memset(meta_buf, 0, sizeof(meta_buf));
    kernel_memset(&fat_info, 0, sizeof(struct fs_info));

    if (read_block(meta_buf, 0, 1) == 1) {
        goto init_fat_info_err;
    }
    log(LOG_OK, "Get MBR sector info");
    
    fat_info.base_addr = get_u32(meta_buf + 446 + 8);

    if (read_block(fat_info.BPB.data, fat_info.base_addr, 1) == 1)
        goto init_fat_info_err;
    log(LOG_OK, "Get FAT BPB");
#ifdef FS_DEBUG
    dump_bpb_info(&(fat_info.BPB.attr));
#endif

    //检测文件系统的相关参数是否符合fat32
    //如果不符合fat32的标准，则返回错误信息 
    //检测文件的扇区数是否符合规定
    if (fat_info.BPB.attr.sector_size != SECTOR_SIZE) {
        log(LOG_FAIL, "FAT32 Sector size must be %d bytes, but get %d bytes.", SECTOR_SIZE, fat_info.BPB.attr.sector_size);
        goto init_fat_info_err;
    }
    if (fat_info.BPB.attr.max_root_dir_entries != 0) {
        goto init_fat_info_err;
    }
    if (fat_info.BPB.attr.sectors_per_fat != 0) {
        goto init_fat_info_err;
    }
    if (fat_info.BPB.attr.num_of_small_sectors != 0) {
        goto init_fat_info_err;
    }

    u32 total_sectors = fat_info.BPB.attr.num_of_sectors;
    u32 reserved_sectors = fat_info.BPB.attr.reserved_sectors;
    u32 sectors_per_fat = fat_info.BPB.attr.num_of_sectors_per_fat;
    u32 total_data_sectors = total_sectors - reserved_sectors - sectors_per_fat * 2;
    u8 sectors_per_cluster = fat_info.BPB.attr.sectors_per_cluster;
    fat_info.total_data_clusters = total_data_sectors / sectors_per_cluster;
    if (fat_info.total_data_clusters < 65525) {
        goto init_fat_info_err;
    }
    //找到跟目录的扇区
    fat_info.first_data_sector = reserved_sectors + sectors_per_fat * 2;
    log(LOG_OK, "Partition type determined: FAT32");

    //将fsinfo保存在buffer中
    read_block(fat_info.fat_fs_info, 1 + fat_info.base_addr, 1);
    log(LOG_OK, "Get FSInfo sector");

    return 0;

init_fat_info_err:
    return 1;
}

//初始化文件buffer
void init_fat_buf() {
    int i = 0;
    for (i = 0; i < FAT_BUF_NUM; i++) {
        fat_buf[i].cur = 0xffffffff;
        fat_buf[i].state = 0;
    }
}

//初始化路径buffer
void init_dir_buf() {
    int i = 0;
    for (i = 0; i < DIR_DATA_BUF_NUM; i++) {
        dir_data_buf[i].cur = 0xffffffff;
        dir_data_buf[i].state = 0;
    }
}

//文件目录项初始化
u32 init_fs() {
    u32 succ = init_fat_info();
    if (0 != succ)
        goto fs_init_err;
    init_fat_buf();
    init_dir_buf();
    return 0;

fs_init_err:
    log(LOG_FAIL, "File system init fail.");
    return 1;
}

//写当前的目录项扇区
u32 write_fat_sector(u32 index) {
    if ((fat_buf[index].cur != 0xffffffff) && (((fat_buf[index].state) & 0x02) != 0)) {
        //写fat同时复制fat，备份
        if (write_block(fat_buf[index].buf, fat_buf[index].cur, 1) == 1)
            goto write_fat_sector_err;
        if (write_block(fat_buf[index].buf, fat_info.BPB.attr.num_of_sectors_per_fat + fat_buf[index].cur, 1) == 1)
            goto write_fat_sector_err;
        fat_buf[index].state &= 0x01;
    }
    return 0;
write_fat_sector_err:
    return 1;
}

//读fat扇区
u32 read_fat_sector(u32 ThisFATSecNum) {
    u32 index;
    //先在buffer中寻找buffer
    for (index = 0; (index < FAT_BUF_NUM) && (fat_buf[index].cur != ThisFATSecNum); index++)
        ;

    //second chance算法缓冲机制
    //如果没有找到，那么就利用fs_victim_512函数来寻找空闲的buffer，否则设置reference位
    if (index == FAT_BUF_NUM) {
        index = fs_victim_512(fat_buf, &fat_clock_head, FAT_BUF_NUM);

        if (write_fat_sector(index) == 1)
            goto read_fat_sector_err;

        if (read_block(fat_buf[index].buf, ThisFATSecNum, 1) == 1)
            goto read_fat_sector_err;

        fat_buf[index].cur = ThisFATSecNum;
        fat_buf[index].state = 1;
    } else
        fat_buf[index].state |= 0x01;

    return index;
read_fat_sector_err:
    return 0xffffffff;
}

/* path convertion */
u32 fs_next_slash(u8 *f) {
    u32 i, j, k;
    u8 chr11[13];
    for (i = 0; (*(f + i) != 0) && (*(f + i) != '/'); i++)
        ;

    for (j = 0; j < 12; j++) {
        chr11[j] = 0;
        filename11[j] = 0x20;
    }
    for (j = 0; j < 12 && j < i; j++) {
        chr11[j] = *(f + j);
        if (chr11[j] >= 'a' && chr11[j] <= 'z')
            chr11[j] = (u8)(chr11[j] - 'a' + 'A');
    }
    chr11[12] = 0;

    for (j = 0; (chr11[j] != 0) && (j < 12); j++) {
        if (chr11[j] == '.')
            break;

        filename11[j] = chr11[j];
    }

    if (chr11[j] == '.') {
        j++;
        for (k = 8; (chr11[j] != 0) && (j < 12) && (k < 11); j++, k++) {
            filename11[k] = chr11[j];
        }
    }

    filename11[11] = 0;

    return i;
}

//文件名比较函数
u32 fs_cmp_filename(const u8 *f1, const u8 *f2) {
    u32 i;
    for (i = 0; i < 11; i++) {
        if (f1[i] != f2[i])
            return 1;
    }

    return 0;
}

//寻找一个文件，绝对路径
u32 fs_find(FILE *file) {
    u8 *f = file->path;
    u32 next_slash;
    u32 i, k;
    u32 next_clus;
    u32 index;
    u32 sec;

    if (*(f++) != '/')
        goto fs_find_err;

    index = fs_read_512(dir_data_buf, fs_dataclus2sec(2), &dir_data_clock_head, DIR_DATA_BUF_NUM);
    
    if (index == 0xffffffff)
        goto fs_find_err;

    //循环寻找目录项
    while (1) {
        file->dir_entry_pos = 0xFFFFFFFF;

        next_slash = fs_next_slash(f);

        while (1) {
            for (sec = 1; sec <= fat_info.BPB.attr.sectors_per_cluster; sec++) {
                //在当前簇中寻找目录项
                for (i = 0; i < 512; i += 32) {
                    if (*(dir_data_buf[index].buf + i) == 0)
                        goto after_fs_find;

                    if ((fs_cmp_filename(dir_data_buf[index].buf + i, filename11) == 0) &&
                        ((*(dir_data_buf[index].buf + i + 11) & 0x08) == 0)) {
                        file->dir_entry_pos = i;
                        file->dir_entry_sector = dir_data_buf[index].cur - fat_info.base_addr;

                        for (k = 0; k < 32; k++)
                            file->entry.data[k] = *(dir_data_buf[index].buf + i + k);

                        goto after_fs_find;
                    }
                }
                //当前簇读取结束，换取当前扇区的下一个簇
                if (sec < fat_info.BPB.attr.sectors_per_cluster) {
                    index = fs_read_512(dir_data_buf, dir_data_buf[index].cur + 1, &dir_data_clock_head, DIR_DATA_BUF_NUM);
                    if (index == 0xffffffff)
                        goto fs_find_err;
                } else {
                    if (get_fat_entry_value(dir_data_buf[index].cur - fat_info.BPB.attr.sectors_per_cluster + 1, &next_clus) == 1)
                        goto fs_find_err;

                    if (next_clus <= fat_info.total_data_clusters + 1) {
                        index = fs_read_512(dir_data_buf, fs_dataclus2sec(next_clus), &dir_data_clock_head, DIR_DATA_BUF_NUM);
                        if (index == 0xffffffff)
                            goto fs_find_err;
                    } else
                        goto after_fs_find;
                }
            }
        }

    after_fs_find:
        //没有找到文件
        if (file->dir_entry_pos == 0xFFFFFFFF)
            goto fs_find_ok;

        //如果路径分析完成了
        if (f[next_slash] == 0)
            goto fs_find_ok;

        //如果不是子目录
        if ((file->entry.data[11] & 0x10) == 0)
            goto fs_find_err;

        f += next_slash + 1;

        //打开子目录
        next_clus = get_start_cluster(file);

        if (next_clus <= fat_info.total_data_clusters + 1) {
            index = fs_read_512(dir_data_buf, fs_dataclus2sec(next_clus), &dir_data_clock_head, DIR_DATA_BUF_NUM);
            if (index == 0xffffffff)
                goto fs_find_err;
        } else
            goto fs_find_err;
    }
fs_find_ok:
    return 0;
fs_find_err:
    return 1;
}

//打开文件，具体执行找到文件并且初始化buffer
u32 fs_open(FILE *file, u8 *filename) {
    u32 i;

    //局部变量的初始化
    for (i = 0; i < LOCAL_DATA_BUF_NUM; i++) {
        file->data_buf[i].cur = 0xffffffff;
        file->data_buf[i].state = 0;
    }

    file->clock_head = 0;

    for (i = 0; i < 256; i++)
        file->path[i] = 0;
    for (i = 0; i < 256 && filename[i] != 0; i++)
        file->path[i] = filename[i];

    file->loc = 0;

    if (fs_find(file) == 1)
        goto fs_open_err;

    //如果文件不存在，打开失败
    if (file->dir_entry_pos == 0xFFFFFFFF)
        goto fs_open_err;

    return 0;
fs_open_err:
    return 1;
}

//将全局buffer写到sd卡上
u32 fs_fflush() {
    u32 i;

    if (write_block(fat_info.fat_fs_info, 1 + fat_info.base_addr, 1) == 1)
        goto fs_fflush_err;

    if (write_block(fat_info.fat_fs_info, 7 + fat_info.base_addr, 1) == 1)
        goto fs_fflush_err;

    for (i = 0; i < FAT_BUF_NUM; i++)
        if (write_fat_sector(i) == 1)
            goto fs_fflush_err;

    for (i = 0; i < DIR_DATA_BUF_NUM; i++)
        if (fs_write_512(dir_data_buf + i) == 1)
            goto fs_fflush_err;

    return 0;

fs_fflush_err:
    return 1;
}

//关闭文件，具体执行文件的全局和局部buffer写到sd卡
u32 fs_close(FILE *file) {
    u32 i;
    u32 index;

    //写文件的目录项
    index = fs_read_512(dir_data_buf, file->dir_entry_sector, &dir_data_clock_head, DIR_DATA_BUF_NUM);
    if (index == 0xffffffff)
        goto fs_close_err;

    dir_data_buf[index].state = 3;

    for (i = 0; i < 32; i++)
        *(dir_data_buf[index].buf + file->dir_entry_pos + i) = file->entry.data[i];
    //写全局buffer到sd卡
    if (fs_fflush() == 1)
        goto fs_close_err;
    //写局部buffer到sd卡
    for (i = 0; i < LOCAL_DATA_BUF_NUM; i++)
        if (fs_write_4k(file->data_buf + i) == 1)
            goto fs_close_err;

    return 0;
fs_close_err:
    return 1;
}

//读取文件，count为读取的字节数
u32 fs_read(FILE *file, u8 *buf, u32 count) {
    u32 start_clus, start_byte;
    u32 end_clus, end_byte;
    u32 filesize = file->entry.attr.size;
    u32 clus = get_start_cluster(file);
    u32 next_clus;
    u32 i;
    u32 cc;
    u32 index;

    //如果目标文件为空
    if (clus == 0)
        return 0;

    //如果当前读到的位置加上要读的字节数大于文件的大小，那么只能读取文件大小的内容
    if (file->loc + count > filesize)
        count = filesize - file->loc;

    //如果读取0字节的内容，直接返回0
    if (count == 0)
        return 0;

    start_clus = file->loc >> fs_wa(fat_info.BPB.attr.sectors_per_cluster << 9);
    start_byte = file->loc & ((fat_info.BPB.attr.sectors_per_cluster << 9) - 1);
    end_clus = (file->loc + count - 1) >> fs_wa(fat_info.BPB.attr.sectors_per_cluster << 9);
    end_byte = (file->loc + count - 1) & ((fat_info.BPB.attr.sectors_per_cluster << 9) - 1);

    //打开第一个簇
    for (i = 0; i < start_clus; i++) {
        if (get_fat_entry_value(clus, &next_clus) == 1)
            goto fs_read_err;

        clus = next_clus;
    }

    cc = 0;
    while (start_clus <= end_clus) {
        index = fs_read_4k(file->data_buf, fs_dataclus2sec(clus), &(file->clock_head), LOCAL_DATA_BUF_NUM);
        if (index == 0xffffffff)
            goto fs_read_err;

        //如果要读的内容起始位置和终止位置都在同一个簇内，那么直接读
        if (start_clus == end_clus) {
            for (i = start_byte; i <= end_byte; i++)
                buf[cc++] = file->data_buf[index].buf[i];
            goto fs_read_end;
        }
        //如果不在同一个簇中，那么就顺着每个簇读
        else {
            for (i = start_byte; i < (fat_info.BPB.attr.sectors_per_cluster << 9); i++)
                buf[cc++] = file->data_buf[index].buf[i];

            start_clus++;
            start_byte = 0;

            if (get_fat_entry_value(clus, &next_clus) == 1)
                goto fs_read_err;

            clus = next_clus;
        }
    }
fs_read_end:

    //修改文件当前读到的位置指针
    file->loc += count;
    return cc;
fs_read_err:
    return 0xFFFFFFFF;
}

//寻找一个空的数据块
u32 fs_next_free(u32 start, u32 *next_free) {
    u32 clus;
    u32 ClusEntryVal;

    *next_free = 0xFFFFFFFF;

    for (clus = start; clus <= fat_info.total_data_clusters + 1; clus++) {
        if (get_fat_entry_value(clus, &ClusEntryVal) == 1)
            goto fs_next_free_err;

        if (ClusEntryVal == 0) {
            *next_free = clus;
            break;
        }
    }

    return 0;
fs_next_free_err:
    return 1;
}

//分配一块新的空闲数据块
u32 fs_alloc(u32 *new_alloc) {
    u32 clus;
    u32 next_free;

    clus = get_u32(fat_info.fat_fs_info + 492) + 1;

    /* If FSI_Nxt_Free is illegal (> FSI_Free_Count), find a free data cluster
     * from beginning */
    if (clus > get_u32(fat_info.fat_fs_info + 488) + 1) {
        if (fs_next_free(2, &clus) == 1)
            goto fs_alloc_err;

        if (fs_modify_fat(clus, 0xFFFFFFFF) == 1)
            goto fs_alloc_err;
    }

    /* FAT allocated and update FSI_Nxt_Free */
    if (fs_modify_fat(clus, 0xFFFFFFFF) == 1)
        goto fs_alloc_err;

    if (fs_next_free(clus, &next_free) == 1)
        goto fs_alloc_err;

    /* no available free cluster */
    if (next_free > fat_info.total_data_clusters + 1)
        goto fs_alloc_err;

    set_u32(fat_info.fat_fs_info + 492, next_free - 1);

    *new_alloc = clus;

    /* Erase new allocated cluster */
    if (write_block(new_alloc_empty, fs_dataclus2sec(clus), fat_info.BPB.attr.sectors_per_cluster) == 1)
        goto fs_alloc_err;

    return 0;
fs_alloc_err:
    return 1;
}

//写入文件
u32 fs_write(FILE *file, const u8 *buf, u32 count) {

    //如果要写入的0字节的内容，则直接返回
    if (count == 0) {
        return 0;
    }

    u32 start_clus = file->loc >> fs_wa(fat_info.BPB.attr.sectors_per_cluster << 9);
    u32 start_byte = file->loc & ((fat_info.BPB.attr.sectors_per_cluster << 9) - 1);
    u32 end_clus = (file->loc + count - 1) >> fs_wa(fat_info.BPB.attr.sectors_per_cluster << 9);
    u32 end_byte = (file->loc + count - 1) & ((fat_info.BPB.attr.sectors_per_cluster << 9) - 1);

    //如果文件是空的，那么为文件分配数据块
    u32 curr_cluster = get_start_cluster(file);
    if (curr_cluster == 0) {
        if (fs_alloc(&curr_cluster) == 1) {
            goto fs_write_err;
        }
        file->entry.attr.starthi = (u16)(((curr_cluster >> 16) & 0xFFFF));
        file->entry.attr.startlow = (u16)((curr_cluster & 0xFFFF));
        if (fs_clr_4k(file->data_buf, &(file->clock_head), LOCAL_DATA_BUF_NUM, fs_dataclus2sec(curr_cluster)) == 1)
            goto fs_write_err;
    }

    //打开并读取第一个簇
    u32 next_cluster;
    for (u32 i = 0; i < start_clus; i++) {
        if (get_fat_entry_value(curr_cluster, &next_cluster) == 1)
            goto fs_write_err;

        //如果当前写入的是文件的最后一个簇，但是还需要读取下一个簇，那么为文件再分配数据块
        if (next_cluster > fat_info.total_data_clusters + 1) {
            if (fs_alloc(&next_cluster) == 1)
                goto fs_write_err;

            if (fs_modify_fat(curr_cluster, next_cluster) == 1)
                goto fs_write_err;

            if (fs_clr_4k(file->data_buf, &(file->clock_head), LOCAL_DATA_BUF_NUM, fs_dataclus2sec(next_cluster)) == 1)
                goto fs_write_err;
        }

        curr_cluster = next_cluster;
    }

    u32 cc = 0;
    u32 index = 0;
    while (start_clus <= end_clus) {
        index = fs_read_4k(file->data_buf, fs_dataclus2sec(curr_cluster), &(file->clock_head), LOCAL_DATA_BUF_NUM);
        if (index == 0xffffffff)
            goto fs_write_err;

        file->data_buf[index].state = 3;

        /* If in same cluster, just write */
        if (start_clus == end_clus) {
            for (u32 i = start_byte; i <= end_byte; i++)
                file->data_buf[index].buf[i] = buf[cc++];
            goto fs_write_end;
        }
        /* otherwise, write clusters one by one */
        else {
            for (u32 i = start_byte; i < (fat_info.BPB.attr.sectors_per_cluster << 9); i++)
                file->data_buf[index].buf[i] = buf[cc++];

            start_clus++;
            start_byte = 0;

            if (get_fat_entry_value(curr_cluster, &next_cluster) == 1)
                goto fs_write_err;

            //如果当前写入的是文件的最后一个簇，但是还需要读取下一个簇，那么为文件再分配数据块
            if (next_cluster > fat_info.total_data_clusters + 1) {
                if (fs_alloc(&next_cluster) == 1)
                    goto fs_write_err;

                if (fs_modify_fat(curr_cluster, next_cluster) == 1)
                    goto fs_write_err;

                if (fs_clr_4k(file->data_buf, &(file->clock_head), LOCAL_DATA_BUF_NUM, fs_dataclus2sec(next_cluster)) == 1)
                    goto fs_write_err;
            }

            curr_cluster = next_cluster;
        }
    }

fs_write_end:

    //修改文件的大小
    if (file->loc + count > file->entry.attr.size)
        file->entry.attr.size = file->loc + count;

    //修改当权读取的位置
    file->loc += count;

    return cc;
fs_write_err:
    return 0xFFFFFFFF;
}

//当前读取位置的重定位
void fs_lseek(FILE *file, u32 new_loc) {
    u32 filesize = file->entry.attr.size;

    if (new_loc < filesize)
        file->loc = new_loc;
    else
        file->loc = filesize;
}

/* find an empty directory entry */
u32 fs_find_empty_entry(u32 *empty_entry, u32 index) {
    u32 i;
    u32 next_clus;
    u32 sec;

    while (1) {
        for (sec = 1; sec <= fat_info.BPB.attr.sectors_per_cluster; sec++) {
            /* Find directory entry in current cluster */
            for (i = 0; i < 512; i += 32) {
                /* If entry is empty */
                if ((*(dir_data_buf[index].buf + i) == 0) || (*(dir_data_buf[index].buf + i) == 0xE5)) {
                    *empty_entry = i;
                    goto after_fs_find_empty_entry;
                }
            }

            if (sec < fat_info.BPB.attr.sectors_per_cluster) {
                index = fs_read_512(dir_data_buf, dir_data_buf[index].cur + sec, &dir_data_clock_head, DIR_DATA_BUF_NUM);
                if (index == 0xffffffff)
                    goto fs_find_empty_entry_err;
            } else {
                /* Read next cluster of current directory */
                if (get_fat_entry_value(dir_data_buf[index].cur - fat_info.BPB.attr.sectors_per_cluster + 1, &next_clus) == 1)
                    goto fs_find_empty_entry_err;

                /* need to alloc a new cluster */
                if (next_clus > fat_info.total_data_clusters + 1) {
                    if (fs_alloc(&next_clus) == 1)
                        goto fs_find_empty_entry_err;

                    if (fs_modify_fat(fs_sec2dataclus(dir_data_buf[index].cur - fat_info.BPB.attr.sectors_per_cluster + 1), next_clus) == 1)
                        goto fs_find_empty_entry_err;

                    *empty_entry = 0;

                    if (fs_clr_512(dir_data_buf, &dir_data_clock_head, DIR_DATA_BUF_NUM, fs_dataclus2sec(next_clus)) == 1)
                        goto fs_find_empty_entry_err;
                }

                index = fs_read_512(dir_data_buf, fs_dataclus2sec(next_clus), &dir_data_clock_head, DIR_DATA_BUF_NUM);
                if (index == 0xffffffff)
                    goto fs_find_empty_entry_err;
            }
        }
    }

after_fs_find_empty_entry:
    return index;
fs_find_empty_entry_err:
    return 0xffffffff;
}

//创建一个空的文件
u32 fs_create_with_attr(u8 *filename, u8 attr) {
    u32 i;
    u32 l1 = 0;
    u32 l2 = 0;
    u32 empty_entry;
    u32 clus;
    u32 index;
    FILE file_creat;
    /* If file exists */
    if (fs_open(&file_creat, filename) == 0)
        goto fs_creat_err;

    for (i = 255; i >= 0; i--)
        if (file_creat.path[i] != 0) {
            l2 = i;
            break;
        }

    for (i = 255; i >= 0; i--)
        if (file_creat.path[i] == '/') {
            l1 = i;
            break;
        }

    /* If not root directory, find that directory */
    if (l1 != 0) {
        for (i = l1; i <= l2; i++)
            file_creat.path[i] = 0;

        if (fs_find(&file_creat) == 1)
            goto fs_creat_err;

        /* If path not found */
        if (file_creat.dir_entry_pos == 0xFFFFFFFF)
            goto fs_creat_err;

        clus = get_start_cluster(&file_creat);
        /* Open that directory */
        index = fs_read_512(dir_data_buf, fs_dataclus2sec(clus), &dir_data_clock_head, DIR_DATA_BUF_NUM);
        if (index == 0xffffffff)
            goto fs_creat_err;

        file_creat.dir_entry_pos = clus;
    }
    /* otherwise, open root directory */
    else {
        index = fs_read_512(dir_data_buf, fs_dataclus2sec(2), &dir_data_clock_head, DIR_DATA_BUF_NUM);
        if (index == 0xffffffff)
            goto fs_creat_err;

        file_creat.dir_entry_pos = 2;
    }

    /* find an empty entry */
    index = fs_find_empty_entry(&empty_entry, index);
    if (index == 0xffffffff)
        goto fs_creat_err;

    for (i = l1 + 1; i <= l2; i++)
        file_creat.path[i - l1 - 1] = filename[i];

    file_creat.path[l2 - l1] = 0;
    fs_next_slash(file_creat.path);

    dir_data_buf[index].state = 3;

    /* write path */
    for (i = 0; i < 11; i++)
        *(dir_data_buf[index].buf + empty_entry + i) = filename11[i];

    /* write file attr */
    *(dir_data_buf[index].buf + empty_entry + 11) = attr;

    /* other should be zero */
    for (i = 12; i < 32; i++)
        *(dir_data_buf[index].buf + empty_entry + i) = 0;

    if (fs_fflush() == 1)
        goto fs_creat_err;

    return 0;
fs_creat_err:
    return 1;
}

u32 fs_create(u8 *filename) {
    return fs_create_with_attr(filename, 0x20);
}

void get_filename(u8 *entry, u8 *buf) {
    u32 i;
    u32 l1 = 0, l2 = 8;

    for (i = 0; i < 11; i++)
        buf[i] = entry[i];

    if (buf[0] == '.') {
        if (buf[1] == '.')
            buf[2] = 0;
        else
            buf[1] = 0;
    } else {
        for (i = 0; i < 8; i++)
            if (buf[i] == 0x20) {
                buf[i] = '.';
                l1 = i;
                break;
            }

        if (i == 8) {
            for (i = 11; i > 8; i--)
                buf[i] = buf[i - 1];

            buf[8] = '.';
            l1 = 8;
            l2 = 9;
        }

        for (i = l1 + 1; i < l1 + 4; i++) {
            if (buf[l2 + i - l1 - 1] != 0x20)
                buf[i] = buf[l2 + i - l1 - 1];
            else
                break;
        }

        buf[i] = 0;

        if (buf[i - 1] == '.')
            buf[i - 1] = 0;
    }
}
