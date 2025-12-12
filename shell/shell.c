/*
 * shell.c - SriOS Shell
 * 
 * To add a new command module:
 * 1. Create cmd_xxx.h and cmd_xxx.c in commands/
 * 2. Add #include "commands/cmd_xxx.h" below
 * 3. Add cmd_xxx_init() call in all_commands_init()
 */

#include "shell.h"
#include "../drivers/uart/uart.h"
#include "commands/commands.h"
#include <stdint.h>
#include "../kernel/scheduler/task.h"

// Include command modules here
#include "commands/cmd_system.h"
// #include "commands/cmd_files.h"    // Add when ready
// #include "commands/cmd_task.h"     // Add when ready

/*
 * Register all commands
 * ADD NEW COMMAND MODULES HERE!
 */
static void all_commands_init(void) {
    cmd_system_init();      // help, info, uptime, clear, reboot
    // cmd_files_init();    // ls, cat, touch, rm, write, edit
    // cmd_task_init();     // ps, kill
}

void shell_init(void) {
    // Register all commands first
    all_commands_init();
    
    // Print banner
    uart_puts("\n");
    uart_puts("================================\n");
    uart_puts("  SriOS Shell v1.0\n");
    uart_puts("  Type 'help' for commands\n");
    uart_puts("================================\n");
    uart_puts("\n");
}

void shell_run(void) {
    char cmd[128];
    int index = 0;
    
    while (1) {
        uart_puts("sri> ");
        index = 0;
        uart_readline(cmd, sizeof(cmd));
        
        process_command(cmd);
    }
}

void shell_task(void) {
    shell_init();
    shell_run();
}
