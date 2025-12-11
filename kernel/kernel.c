#include "../drivers/gpio/gpio.h"
#include "../drivers/uart/uart.h"
#include "./interrupts/interrupts.h"

extern char _vectors;

void kernel_main(void) {
    uart_init();
    
    uart_puts("\n\n=== IRQ TEST ===\n\n");
    
    // Set VBAR
    uint32_t vec_addr = (uint32_t)&_vectors;
    __asm__ __volatile__("mcr p15, 0, %0, c12, c0, 0" :: "r"(vec_addr));
    
    gpio_set_output(23);
    
    interrupts_init();
    timer_init();
    
    uart_puts("Enabling IRQ...\n");
    __asm__ __volatile__("cpsie i" ::: "memory");
    uart_puts("IRQ enabled!\n\n");
    
    while (1) {
        uart_putc('.');
        for (volatile int i = 0; i < 2000000; i++);
    }
}