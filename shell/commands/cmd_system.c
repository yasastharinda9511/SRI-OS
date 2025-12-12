#include "cmd_system.h"
#include "commands.h"
#include "../../drivers/uart/uart.h"
#include "../../kernel/interrupts/interrupts.h"
#include "../../utils/string_utils.h"

// ============== HELP ==============
void cmd_help(const char* args) {
    (void)args;
    
    uart_puts("\nAvailable commands:\n");
    uart_puts("─────────────────────────────────\n");
    
    for (int i = 0; i < command_count; i++) {
        uart_puts("  ");
        uart_puts(command_table[i].name);
        
        // Pad for alignment
        int len = str_len(command_table[i].name);
        int pad = 12 - len;
        while (pad-- > 0) uart_putc(' ');
        
        uart_puts(command_table[i].description);
        uart_puts("\n");
    }
    
    uart_puts("─────────────────────────────────\n\n");
}

// ============== INFO ==============
void cmd_info(const char* args) {
    (void)args;
    
    uart_puts("\n");
    uart_puts("╔══════════════════════════════╗\n");
    uart_puts("║       System Info            ║\n");
    uart_puts("╠══════════════════════════════╣\n");
    uart_puts("║  OS:     SriOS               ║\n");
    uart_puts("║  CPU:    Cortex-A53 (32-bit) ║\n");
    uart_puts("║  Board:  Pi Zero 2W          ║\n");
    uart_puts("║  RAM:    512 MB              ║\n");
    uart_puts("╚══════════════════════════════╝\n");
    uart_puts("\n");
}

// ============== UPTIME ==============
void cmd_uptime(const char* args) {
    (void)args;
    
    uint32_t total = timer_ticks;
    uint32_t hours = total / 3600;
    uint32_t mins = (total % 3600) / 60;
    uint32_t secs = total % 60;
    
    uart_puts("Uptime: ");
    
    // Print hours
    if (hours > 0) {
        if (hours >= 10) uart_putc('0' + hours / 10);
        uart_putc('0' + hours % 10);
        uart_puts("h ");
    }
    
    // Print minutes
    if (mins > 0 || hours > 0) {
        if (mins >= 10) uart_putc('0' + mins / 10);
        uart_putc('0' + mins % 10);
        uart_puts("m ");
    }
    
    // Print seconds
    if (secs >= 10) uart_putc('0' + secs / 10);
    uart_putc('0' + secs % 10);
    uart_puts("s\n");
}

// ============== CLEAR ==============
void cmd_clear(const char* args) {
    (void)args;
    uart_puts("\033[2J\033[H");
}

// ============== REBOOT ==============
void cmd_reboot(const char* args) {
    (void)args;
    uart_puts("Rebooting...\n");
    
    #define PM_BASE     0x3F100000
    #define PM_RSTC     ((volatile unsigned int*)(PM_BASE + 0x1C))
    #define PM_WDOG     ((volatile unsigned int*)(PM_BASE + 0x24))
    #define PM_PASSWORD 0x5A000000
    #define PM_RSTC_WRCFG_FULL_RESET 0x20
    
    *PM_WDOG = PM_PASSWORD | 1;
    *PM_RSTC = PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET;
    
    while (1);
}

// Register all system commands
void cmd_system_init(void) {
    register_command("help",   "help",   "Show available commands", cmd_help);
    register_command("info",   "info",   "System information",      cmd_info);
    register_command("uptime", "uptime", "Show system uptime",      cmd_uptime);
    register_command("clear",  "clear",  "Clear screen",            cmd_clear);
    register_command("reboot", "reboot", "Reboot system",           cmd_reboot);
}
