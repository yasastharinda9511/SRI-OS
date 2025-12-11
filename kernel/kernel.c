#include "../drivers/gpio/gpio.h"
#include "../drivers/uart/uart.h"
#include "./interrupts/interrupts.h"
#include "./scheduler/task.h"

// Task 1 - no yield, just loops
void task_one(void) {
    int count = 0;
    while (1) {
        uart_puts("ONE: ");
        uart_puthex(count++);
        uart_puts("\n");
        
        // No yield! Timer IRQ will preempt this
        for(volatile int i = 0; i < 3000000; i++);
    }
}

// Task 2 - no yield, just loops
void task_two(void) {
    int count = 0;
    while (1) {
        uart_puts("TWO: ");
        uart_puthex(count++);
        uart_puts("\n");
        
        // No yield! Timer IRQ will preempt this
        for(volatile int i = 0; i < 3000000; i++);
    }
}

void kernel_main(void) {
    uart_init();
    
    uart_puts("\n\n");
    uart_puts("================================\n");
    uart_puts("  SriOS - Preemptive Scheduler\n");
    uart_puts("================================\n\n");
    
    // Set VBAR
    extern char _vectors;
    uint32_t vec_addr = (uint32_t)&_vectors;
    __asm__ __volatile__("mcr p15, 0, %0, c12, c0, 0" :: "r"(vec_addr));
    
    scheduler_init();
    task_create("Task ONE", task_one, 1);
    task_create("Task TWO", task_two, 1);
    
    gpio_set_output(23);
    
    interrupts_init();
    timer_init();
    
    uart_puts("Enabling IRQ...\n");
    enable_irq();
    uart_puts("IRQ enabled!\n\n");
    
    uart_puts("Starting preemptive scheduler...\n");
    uart_puts("(Tasks will switch automatically by timer)\n\n");
    
    scheduler_start();
    
    // Should never reach here
    while (1) {}
}