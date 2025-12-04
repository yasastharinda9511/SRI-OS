#include "uart.h"
#include "interrupts.h"
#include "shell.h"

void kernel_main(void) {
    uart_init();

    uart_puts("\n");
    uart_puts("================================\n");
    uart_puts("  Pi Zero Bare Metal OS\n");
    uart_puts("================================\n\n");

    // Initialize interrupts and timer
    interrupts_init();
    timer_init();
    enable_irq();

    uart_puts("System initialized.\n");

    // Start shell
    shell_init();
    shell_run();
    
    // Never reaches here
    while (1);
}