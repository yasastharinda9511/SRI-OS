#include "../../kernel/fatfs/ff.h"
#include "../../utils/string_utils.h"
#include "commands.h"
#include "../../drivers/uart/uart.h"
#include "../../utils/string_utils.h"

void cmd_ls(const char* args) {
    (void)args;

    DIR dir;
    FILINFO fno;
    FRESULT res;

    res = f_opendir(&dir, "/");
    if (res != FR_OK) {
        uart_puts("ls: failed to open directory\n");
        return;
    }

    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) break;

        if (fno.fattrib & AM_DIR)
            uart_puts("[DIR]  ");
        else
            uart_puts("       ");

        uart_puts(fno.fname);
        uart_puts("\n");
    }

    f_closedir(&dir);
}

void cmd_cat(const char* args) {
    if (!args || args[0] == 0) {
        uart_puts("Usage: cat <file>\n");
        return;
    }

    FIL file;
    char buf[64];
    UINT br;

    if (f_open(&file, args, FA_READ) != FR_OK) {
        uart_puts("cat: cannot open file\n");
        return;
    }

    while (f_read(&file, buf, sizeof(buf)-1, &br) == FR_OK && br > 0) {
        buf[br] = 0;
        uart_puts(buf);
    }

    uart_puts("\n");
    f_close(&file);
}

void cmd_touch(const char* args) {
    if (!args || args[0] == 0) {
        uart_puts("Usage: touch <file>\n");
        return;
    }

    FIL file;
    if (f_open(&file, args, FA_CREATE_ALWAYS) == FR_OK) {
        f_close(&file);
        uart_puts("File created\n");
    } else {
        uart_puts("touch: failed\n");
    }
}

void cmd_write(const char* args) {
    if (!args) {
        uart_puts("Usage: write <file> <text>\n");
        return;
    }

    char filename[32];
    const char* text;

    // Split args
    int i = 0;
    while (args[i] && args[i] != ' ') {
        filename[i] = args[i];
        i++;
    }
    filename[i] = 0;

    text = args + i + 1;
    if (!*text) {
        uart_puts("write: no text\n");
        return;
    }

    FIL file;
    UINT bw;

    if (f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
        uart_puts("write: open failed\n");
        return;
    }

    f_write(&file, text, str_len(text), &bw);
    f_close(&file);

    uart_puts("Written OK\n");
}

void cmd_rm(const char* args) {
    if (!args || args[0] == 0) {
        uart_puts("Usage: rm <file>\n");
        return;
    }

    if (f_unlink(args) == FR_OK)
        uart_puts("Deleted\n");
    else
        uart_puts("rm: failed\n");
}

void cmd_mkdir(const char* args) {
    if (!args || args[0] == 0) {
        uart_puts("Usage: mkdir <dir>\n");
        return;
    }

    if (f_mkdir(args) == FR_OK)
        uart_puts("Directory created\n");
    else
        uart_puts("mkdir: failed\n");
}


void cmd_fs_init(){
    
    register_command("ls",    "ls",    "List files",        cmd_ls);
    register_command("cat",   "cat",   "Show file content", cmd_cat);
    register_command("touch", "touch", "Create empty file", cmd_touch);
    register_command("write", "write", "Write text to file",cmd_write);
    register_command("rm",    "rm",    "Delete file",       cmd_rm);
    register_command("mkdir", "mkdir", "Create directory", cmd_mkdir);
}