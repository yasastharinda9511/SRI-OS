#include "shell.h"
#include "uart.h"
#include "interrupts.h"

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