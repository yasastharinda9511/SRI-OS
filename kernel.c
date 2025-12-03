#include "uart.h"
#include "interrupts.h"

void kernel_main(void) {
    uart_init();

    uart_puts("\n");
    uart_puts("================================\n");
    uart_puts("  Pi Zero Bare Metal OS\n");
    uart_puts("================================\n\n");

    interrupts_init();
    timer_init();

    // Enable IRQs (will work on real hardware)
    uart_puts("Enabling IRQs...\n");
    enable_irq();

    uart_puts("\nEntering main loop...\n\n");

    while (1) {
        // // Poll timer (works on QEMU)
        // timer_poll();
    }
}