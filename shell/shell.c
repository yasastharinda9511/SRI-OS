#include "shell.h"
#include "../drivers/uart.h"
#include "../kernel/interrupts.h"
#include "../kernel/fs.h"

// String comparison
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// String starts with
static int startswith(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) {
            return 0;
        }
    }
    return 1;
}

// Skip leading spaces
static const char* skip_spaces(const char* str) {
    while (*str == ' ') {
        str++;
    }
    return str;
}

// Command handlers
static void cmd_help(void) {
    uart_puts("\n");
    uart_puts("Available commands:\n");
    uart_puts("  help     - Show this help\n");
    uart_puts("  info     - System information\n");
    uart_puts("  uptime   - Show system uptime\n");
    uart_puts("  echo     - Echo text (usage: echo hello)\n");
    uart_puts("  clear    - Clear screen\n");
    uart_puts("  reboot   - Reboot system\n");
    uart_puts("\n");
    uart_puts("  Files:\n");
    uart_puts("    ls         - List files\n");
    uart_puts("    touch      - Create file (touch <name>)\n");
    uart_puts("    rm         - Delete file (rm <name>)\n");
    uart_puts("    cat        - Show file contents (cat <name>)\n");
    uart_puts("    write      - Write to file (write <name> <text>)\n");
    uart_puts("    edit       - Edit file interactively (edit <name>)\n");
    uart_puts("\n");
    uart_puts("  Other:\n");
    uart_puts("    echo       - Echo text (echo <text>)\n");
    uart_puts("\n");
}

static void cmd_info(void) {
    uart_puts("\n");
    uart_puts("=== System Info ===\n");
    uart_puts("OS:     SriOS\n");
    uart_puts("CPU:    ARM1176JZF-S\n");
    uart_puts("Board:  Raspberry Pi Zero\n");
    uart_puts("RAM:    512 MB\n");
    uart_puts("\n");
}

static void cmd_uptime(void) {
    uint32_t ticks = timer_ticks;
    uint32_t seconds = ticks;  // Assuming 1 tick = 1 second
    
    uart_puts("Uptime: ");
    
    // Simple number to string conversion
    if (seconds == 0) {
        uart_puts("0");
    } else {
        char buf[16];
        int i = 0;
        while (seconds > 0) {
            buf[i++] = '0' + (seconds % 10);
            seconds /= 10;
        }
        // Print in reverse
        while (i > 0) {
            uart_putc(buf[--i]);
        }
    }
    
    uart_puts(" seconds\n");
}

static void cmd_echo(const char* args) {
    // Skip "echo " prefix
    uart_puts(args);
    uart_puts("\n");
}

static void cmd_clear(void) {
    // ANSI escape sequence to clear screen
    uart_puts("\033[2J\033[H");
}

static void cmd_reboot(void) {
    uart_puts("Rebooting...\n");
    
    // Watchdog reboot (BCM2835)
    #define PM_BASE     0x20100000
    #define PM_RSTC     ((volatile unsigned int*)(PM_BASE + 0x1C))
    #define PM_WDOG     ((volatile unsigned int*)(PM_BASE + 0x24))
    #define PM_PASSWORD 0x5A000000
    #define PM_RSTC_WRCFG_FULL_RESET 0x20
    
    *PM_WDOG = PM_PASSWORD | 1;
    *PM_RSTC = PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET;
    
    // Wait for reboot
    while (1);
}

static void cmd_unknown(const char* cmd) {
    uart_puts("Unknown command: ");
    uart_puts(cmd);
    uart_puts("\nType 'help' for available commands.\n");
}

static void cmd_touch(const char* name) {
    name = skip_spaces(name);
    
    if (name[0] == '\0') {
        uart_puts("Usage: touch <filename>\n");
        return;
    }
    
    int result = fs_create(name);
    if (result == -1) {
        uart_puts("File already exists.\n");
    } else if (result == -2) {
        uart_puts("Error: No space for new files.\n");
    }
}

static void cmd_ls(void) {
    fs_list();
}

static void cmd_rm(const char* name) {
    name = skip_spaces(name);
    
    if (name[0] == '\0') {
        uart_puts("Usage: rm <filename>\n");
        return;
    }
    
    int result = fs_delete(name);
    if (result == -1) {
        uart_puts("File not found.\n");
    }
}


static void cmd_cat(const char* name) {
    name = skip_spaces(name);
    
    if (name[0] == '\0') {
        uart_puts("Usage: cat <filename>\n");
        return;
    }
    
    char buf[MAX_FILESIZE];
    int result = fs_read(name, buf, sizeof(buf));
    
    if (result < 0) {
        uart_puts("File not found.\n");
    } else if (result == 0) {
        uart_puts("(empty file)\n");
    } else {
        uart_puts(buf);
        uart_puts("\n");
    }
}

static void cmd_write(const char* args) {
    args = skip_spaces(args);
    
    // Parse: <filename> <content>
    char name[MAX_FILENAME];
    int i = 0;
    
    // Get filename
    while (args[i] && args[i] != ' ' && i < MAX_FILENAME - 1) {
        name[i] = args[i];
        i++;
    }
    name[i] = '\0';
    
    if (name[0] == '\0') {
        uart_puts("Usage: write <filename> <text>\n");
        return;
    }
    
    // Get content (skip space after filename)
    const char* content = skip_spaces(args + i);
    
    if (fs_write(name, content) < 0) {
        uart_puts("Error writing file.\n");
    }
}

static void cmd_edit(const char* name) {
    name = skip_spaces(name);
    
    if (name[0] == '\0') {
        uart_puts("Usage: edit <filename>\n");
        return;
    }
    
    // Create file if doesn't exist
    if (!fs_exists(name)) {
        fs_create(name);
    }
    
    // Show current contents
    char buf[MAX_FILESIZE];
    int len = fs_read(name, buf, sizeof(buf));
    
    uart_puts("\n");
    uart_puts("=== Editing: ");
    uart_puts(name);
    uart_puts(" ===\n");
    
    if (len > 0) {
        uart_puts("Current contents:\n");
        uart_puts(buf);
        uart_puts("\n");
    }
    
    uart_puts("\nEnter new contents (single line, Enter to save):\n");
    uart_puts("> ");
    
    char newdata[MAX_FILESIZE];
    uart_readline(newdata, sizeof(newdata));
    
    fs_write(name, newdata);
    uart_puts("File saved.\n");
}

// Process a command
static void process_command(char* cmd) {
    // Skip empty commands
    if (cmd[0] == '\0') {
        return;
    }
    
    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    }
    else if (strcmp(cmd, "info") == 0) {
        cmd_info();
    }
    else if (strcmp(cmd, "uptime") == 0) {
        cmd_uptime();
    }
    else if (startswith(cmd, "echo ")) {
        cmd_echo(cmd + 5);  // Skip "echo "
    }
    else if (strcmp(cmd, "echo") == 0) {
        uart_puts("\n");  // Empty echo
    }
    else if (strcmp(cmd, "clear") == 0) {
        cmd_clear();
    }
    else if (strcmp(cmd, "reboot") == 0) {
        cmd_reboot();
    }
    else if (strcmp(cmd, "ls") == 0) {
        cmd_ls();
    }
    else if (startswith(cmd, "touch ")) {
        cmd_touch(cmd + 6);  
    }else if (startswith(cmd, "rm ")) {
        cmd_rm(cmd + 3);
    }
    else if (startswith(cmd, "cat ")) {
        cmd_cat(cmd + 4);
    }
    else if (startswith(cmd, "write ")) {
        cmd_write(cmd + 6);
    }
    else if (startswith(cmd, "edit ")) {
        cmd_edit(cmd + 5);
    }
    else {
        cmd_unknown(cmd);
    }
}

void shell_init(void) {
    uart_puts("\n");
    uart_puts("================================\n");
    uart_puts("  SriOS Shell v1.0\n");
    uart_puts("  Type 'help' for commands\n");
    uart_puts("================================\n");
    uart_puts("\n");
}

void shell_run(void) {
    char cmd[128];
    
    while (1) {
        uart_puts("sri> ");
        uart_readline(cmd, sizeof(cmd));
        process_command(cmd);
    }
}