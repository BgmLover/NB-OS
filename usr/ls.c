#include <driver/vga.h>
#include <zjunix/fs/fat.h>

//删除掉空格
char *cut_front_blank(char *str) {
    char *s = str;
    unsigned int index = 0;

    while (*s == ' ') {
        ++s;
        ++index;
    }

    if (!index)
        return str;

    while (*s) {
        //覆盖掉前面的空格
        *(s - index) = *s;
        ++s;
    }

    --s;
    *s = 0;//最后一位写成0

    return str;
}

unsigned int strlen(unsigned char *str) {
    unsigned int len = 0;
    while (str[len])
        ++len;
    return len;
}

unsigned int each_param(char *para, char *word, unsigned int off, char ch) {
    int index = 0;

    while (para[off] && para[off] != ch) {
        word[index] = para[off];
        ++index;
        ++off;
    }

    word[index] = 0;

    return off;
}

int ls(char *para) {
    char pwd[128];
    struct dir_entry_attr entry;
    char name[32];
    char *p = para;
    unsigned int next;
    unsigned int r;
    unsigned int p_len;
    FS_FAT_DIR dir;

    p = cut_front_blank(p);
    p_len = strlen(p);
    next = each_param(p, pwd, 0, ' ');

    if (fs_open_dir(&dir, pwd)) {
        kernel_printf("open dir(%s) failed : No such directory!\n", pwd);
        return 1;
    }

readdir:
    r = fs_read_dir(&dir, (unsigned char *)&entry);
    if (1 != r) {
        if (-1 == r) {
            kernel_printf("\n");
        } else {
            get_filename((unsigned char *)&entry, name);
            if (entry.attr == 0x10)  // sub dir
                //kernel_printf("%s/", name);
                {
                    kernel_puts(name,VGA_BLUE,VGA_BLACK);
                    kernel_printf("/   ");
                }

            else if(entry.attr == 0xE5)
            {
                kernel_printf("has deleted");
            } 
            else
                kernel_printf("%s   ", name);
            kernel_printf(" ");
            goto readdir;
        }
    } else
        return 1;

    return 0;
}