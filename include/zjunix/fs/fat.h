#ifndef _ZJUNIX_FS_FAT_H
#define _ZJUNIX_FS_FAT_H

#include <zjunix/type.h>
#include <zjunix/fs/fscache.h>

//每个文件控制块中有一个4K大小的缓冲区
#define LOCAL_DATA_BUF_NUM 4

#define SECTOR_SIZE 512
#define CLUSTER_SIZE 4096


struct __attribute__((__packed__)) dir_entry_attr {
    u8 name[8];                   //文件名
    u8 ext[3];                    //文件扩展名
    u8 attr;                      //文件属性位
    u8 lcase;                     //保留位
    u8 ctime_cs;                  //创建时间，精确到十分之一秒
    u16 ctime;                    //创建时间
    u16 cdate;                    //创建日期
    u16 adate;                    //上次访问时间
    u16 starthi;                  //起始簇高16位
    u16 time;                     //上次修改时间
    u16 date;                     //上次修改日期
    u16 startlow;                 //其实簇低16位
    u32 size;                     //文件大小
};

union dir_entry {
    u8 data[32];
    struct dir_entry_attr attr;
};

//文件控制块
typedef struct fat_file {
    unsigned char path[256];
    //当前读取的文件内部指针
    unsigned long loc;
    //当前目录入口位置
    unsigned long dir_entry_pos;
    unsigned long dir_entry_sector;
    //当前目录入口
    union dir_entry entry;
    //缓存块的clock head
    unsigned long clock_head;
    //文件控制块中有一个4K的buffer
    BUF_4K data_buf[LOCAL_DATA_BUF_NUM];
} FILE;

typedef struct fs_fat_dir {
    unsigned long cur_sector;
    unsigned long loc;
    unsigned long sec;
} FS_FAT_DIR;

struct __attribute__((__packed__)) BPB_attr {
    // 0x00 ~ 0x0f
    u8 jump_code[3];
    u8 oem_name[8];
    u16 sector_size;
    u8 sectors_per_cluster;
    u16 reserved_sectors;
    // 0x10 ~ 0x1f
    u8 number_of_copies_of_fat;
    u16 max_root_dir_entries;
    u16 num_of_small_sectors;
    u8 media_descriptor;
    u16 sectors_per_fat;
    u16 sectors_per_track;
    u16 num_of_heads;
    u32 num_of_hidden_sectors;
    // 0x20 ~ 0x2f
    u32 num_of_sectors;
    u32 num_of_sectors_per_fat;
    u16 flags;
    u16 version;
    u32 cluster_number_of_root_dir;
    // 0x30 ~ 0x3f
    u16 sector_number_of_fs_info;
    u16 sector_number_of_backup_boot;
    u8 reserved_data[12];
    // 0x40 ~ 0x51
    u8 logical_drive_number;
    u8 unused;
    u8 extended_signature;
    u32 serial_number;
    u8 volume_name[11];
    // 0x52 ~ 0x1fe
    u8 fat_name[8];
    u8 exec_code[420];
    u8 boot_record_signature[2];
};

union BPB_info {
    u8 data[512];
    struct BPB_attr attr;
};

struct fs_info {
    u32 base_addr;
    u32 sectors_per_fat;
    u32 total_sectors;
    u32 total_data_clusters;
    u32 total_data_sectors;
    u32 first_data_sector;
    union BPB_info BPB;
    u8 fat_fs_info[SECTOR_SIZE];
};

unsigned long fs_find(FILE *file);

unsigned long init_fs();

unsigned long fs_open(FILE *file, unsigned char *filename);

unsigned long fs_close(FILE *file);

unsigned long fs_read(FILE *file, unsigned char *buf, unsigned long count);

unsigned long fs_write(FILE *file, const unsigned char *buf, unsigned long count);

unsigned long fs_fflush();

void fs_lseek(FILE *file, unsigned long new_loc);

unsigned long fs_create(unsigned char *filename);

unsigned long fs_mkdir(unsigned char *filename);

unsigned long fs_rm(unsigned char *filename);

unsigned long fs_mv(unsigned char *src, unsigned char *dest);

unsigned long fs_open_dir(FS_FAT_DIR *dir, unsigned char *filename);

unsigned long fs_read_dir(FS_FAT_DIR *dir, unsigned char *buf);

unsigned long fs_cat(unsigned char * path);

int ls(char *para);

void get_filename(unsigned char *entry, unsigned char *buf);

u32 read_block(u8 *buf, u32 addr, u32 count);

u32 write_block(u8 *buf, u32 addr, u32 count);

u32 get_entry_filesize(u8 *entry);

u32 get_entry_attr(u8 *entry);

void append_dir(char *nowdir,char *newdir,char *param);

u32 fs_touch(u8 *filename);
    
u32 fs_makedir(u8 *filename);

u32 fs_remove(u8 *filename);
    
u32 fs_changedir(u8 *newdir,u8 *nowdir,u8 *param);

u32 fs_prev_dir(u8 *nowdir);

#endif  // !_ZJUNIX_FS_FAT_H