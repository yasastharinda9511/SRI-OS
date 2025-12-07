#include "../drivers/uart.h"
#include "interrupts.h"
#include "shell.h"
#include "scheduler/task.h"

void task_counter_1(void) {
    int count = 0;
    while (1) {
        uart_puts("[Counter 1] ");
        
        // Print number
        char buf[16];
        int i = 0;
        int n = count;
        if (n == 0) buf[i++] = '0';
        else while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
        while (i > 0) uart_putc(buf[--i]);
        uart_puts("\n");
        
        count++;
        task_yield();  // Give other tasks a chance
        
        // Simple delay
        for (volatile int j = 0; j < 1000000; j++);
    }
}

void task_counter_2(void) {
    int count = 0;
    while (1) {
        uart_puts("[Counter 2] ");
        
        // Print number
        char buf[16];
        int i = 0;
        int n = count;
        if (n == 0) buf[i++] = '0';
        else while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
        while (i > 0) uart_putc(buf[--i]);
        uart_puts("\n");
        
        count++;
        task_yield();  // Give other tasks a chance
        
        // Simple delay
        for (volatile int j = 0; j < 1000000; j++);
    }
}

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

    // Initialize scheduler
    scheduler_init();
    scheduler_start();
    uart_puts("Scheduler started.\n");
    // Create sample tasks
    task_create("Counter1", task_counter_1, 1);
    task_create("Counter2", task_counter_2, 1);
    scheduler_start();

    // Start shell
    // shell_init();
    // shell_run();
    
    // Never reaches here
    while (1);
}