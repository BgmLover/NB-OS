#include"../ustdio.h"

char ps_buffer[64];
int ps_buffer_index;
// asm volatile(
//     ".global ps\n\t"
//     "ps:\n\t"
// );
void ps() {
    uprintf("Press any key to enter shell.\n");
    ugetchar();
    char c;
    ps_buffer_index = 0;
    ps_buffer[0] = 0;
    uclear_screen(31);
    uputs("PowerShell\n", 0xfff, 0);
    uputs("PS>", 0xfff, 0);
    int *cursor;
    cursor=getvga();
    while (1) {
        c = ugetchar();
        if (c == '\n') {
            ps_buffer[ps_buffer_index] = 0;
            if (ustrcmp(ps_buffer, "exit") == 0) {
                ps_buffer_index = 0;
                ps_buffer[0] = 0;
                uprintf("\nPowerShell exit.\n");
            } else
                parse_cmd();
            ps_buffer_index = 0;
            uputs("PS>", 0xfff, 0);
        } else if (c == 0x08) {
            if (ps_buffer_index) {
                ps_buffer_index--;
                putchar_at(' ', cursor[0], cursor[1] - 1);
                cursor[1]--;
                set_cursor();
            }
        } else {
            if (ps_buffer_index < 63) {
                ps_buffer[ps_buffer_index++] = c;
                uputchar(c, 0xfff, 0);
            }
        }
    }
}

void parse_cmd() {
    unsigned int result = 0;
    char dir[32];
    char c;
    uputchar('\n', 0, 0);
    char sd_buffer[8192];
    int i = 0;
    char *param;
    for (i = 0; i < 63; i++) {
        if (ps_buffer[i] == ' ') {
            ps_buffer[i] = 0;
            break;
        }
    }
    if (i == 63)
        param = ps_buffer;
    else
        param = ps_buffer + i + 1;
    if (ps_buffer[0] == 0) {
        return;
    } else if (ustrcmp(ps_buffer, "clear") == 0) {
        uclear_screen(31);
    } else if (ustrcmp(ps_buffer, "echo") == 0) {
        uprintf("%s\n", param);
    } else if (ustrcmp(ps_buffer, "gettime") == 0) {
        char buf[10];
        get_time(buf, sizeof(buf));
        uprintf("%s\n", buf);
    // } else if (ustrcmp(ps_buffer, "sdwi") == 0) {
    //     for (i = 0; i < 512; i++)
    //         sd_buffer[i] = i;
    //     sd_write_block(sd_buffer, 7, 1);
    //     uputs("sdwi\n", 0xfff, 0);
    // } else if (ustrcmp(ps_buffer, "sdr") == 0) {
    //     sd_read_block(sd_buffer, 7, 1);
    //     for (i = 0; i < 512; i++) {
    //         uprintf("%d ", sd_buffer[i]);
    //     }
    //     uputchar('\n', 0xfff, 0);
    // } else if (ustrcmp(ps_buffer, "sdwz") == 0) {
    //     for (i = 0; i < 512; i++) {
    //         sd_buffer[i] = 0;
    //     }
    //     sd_write_block(sd_buffer, 7, 1);
    //     uputs("sdwz\n", 0xfff, 0);
    } else if (ustrcmp(ps_buffer, "mminfo") == 0) {
        bootmap_info("bootmm");
        buddy_info();
    } else if (ustrcmp(ps_buffer, "mmtest") == 0) {
        uprintf("kmalloc : %x, size = 1KB\n", umalloc(1024));
    } else if (ustrcmp(ps_buffer, "ps") == 0) {
        result = print_proc();
        uprintf("ps return with %d\n", result);
    } else if (ustrcmp(ps_buffer, "kill") == 0) {
        int pid = param[0] - '0';
        uprintf("Killing process %d\n", pid);
        result = ukill(pid);
        uprintf("kill return with %d\n", result);
    } else if (ustrcmp(ps_buffer, "time") == 0) {
        unsigned int init_gp;
        asm volatile("la %0, _gp\n\t" : "=r"(init_gp));
        //pc_create(2, system_time_proc, (unsigned int)kmalloc(4096), init_gp, "time");
    } else if (ustrcmp(ps_buffer, "proc") == 0) {
        demo_create();
        //uprintf("proc return with %d\n", result);
    } else if (ustrcmp(ps_buffer, "cat") == 0) {
        ucat(param);
        //uprintf("cat return with %d\n", result);
    } else if (ustrcmp(ps_buffer, "ls") == 0) {
        ulistfile(param);
        //uprintf("ls return with %d\n", result);
    } else if (ustrcmp(ps_buffer, "vi") == 0) {
        //result = myvi(param);
        uprintf("vi return with %d\n", result);
    } else if (ustrcmp(ps_buffer, "exec") == 0) {
        result = exec(param);
        uprintf("exec return with %d\n", result);
    } else {
        uputs(ps_buffer, 0xfff, 0);
        uputs(": command not found\n", 0xfff, 0);
    }
}
