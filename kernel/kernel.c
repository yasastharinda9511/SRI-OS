#include "../drivers/gpio/gpio.h"
#include "../drivers/uart/uart.h"
#include "./interrupts/interrupts.h"
#include "./scheduler/task.h"
#include "../shell/shell.h"
#include "../drivers/sd/sd.h"

// Background task - blinks LED
void task_blink(void) {
    gpio_set_output(23);
    
    while (1) {
        gpio_high(23);
        for(volatile int i = 0; i < 2000000; i++);
        
        gpio_low(23);
        for(volatile int i = 0; i < 2000000; i++);
    }
}

// Background task - counter (optional, for demo)
void task_counter(void) {
    int count = 0;
    while (1) {
        // Runs silently in background
        count++;
        for(volatile int i = 0; i < 5000000; i++);
    }
}

void kernel_main(void) {
    uart_init();
    
    uart_puts("\n\n");
    uart_puts("================================\n");
    uart_puts("  SriOS - Pi Zero 2W\n");
    uart_puts("================================\n\n");

    sd_init();

    test_sd_read();
    test_sd_write();
    // Set VBAR
    extern char _vectors;
    uint32_t vec_addr = (uint32_t)&_vectors;
    __asm__ __volatile__("mcr p15, 0, %0, c12, c0, 0" :: "r"(vec_addr));
    
    // Initialize scheduler
    scheduler_init();
    
    // Create tasks
    task_create("Shell", shell_task, 1);      // Interactive shell
    task_create("Blink", task_blink, 1);      // LED blinker
    // task_create("Counter", task_counter, 1);  // Optional
    
    // Initialize interrupts
    gpio_set_output(23);
    interrupts_init();
    timer_init();
    
    uart_puts("Enabling IRQ...\n");
    enable_irq();
    uart_puts("IRQ enabled!\n\n");
    
    // Start scheduler - shell will appear
    scheduler_start();
    
    // Never reaches here
    while (1) {}
}