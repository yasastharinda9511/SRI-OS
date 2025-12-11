#include "../drivers/gpio/gpio.h"
#include "../drivers/uart/uart.h"
#include "./interrupts/interrupts.h"
#include "./scheduler/task.h"

void counter_one_sec(void) {
    while (1)
    {
        uart_puts("Task: One second passed!\n");
        task_yield();
        for(volatile int i = 0; i < 2000000; i++);
    }
}

void counter_two_sec(void) {
    while (1)
    {
        uart_puts("Task: two second passed!\n");
        task_yield();
        for(volatile int i = 0; i < 2000000; i++);
    }
}

extern char _vectors;

void kernel_main(void) {
    uart_init();
    
    uart_puts("\n\n=== IRQ TEST ===\n\n");

    scheduler_init();

    task_create("Counter 1s", counter_one_sec, 1);
    task_create("Counter 2s", counter_two_sec, 1);
    gpio_set_output(23);
    
    interrupts_init();
    timer_init();
    
    uart_puts("Enabling IRQ...\n");
    enable_irq();
    uart_puts("IRQ enabled!\n\n");

    scheduler_start();
    
    while (1) {}
}