#include "../drivers/gpio/gpio.h"
#include "../drivers/uart/uart.h"
#include "./interrupts/interrupts.h"
#include "./scheduler/task.h"
#include "../shell/shell.h"
#include "../drivers/sd/sd.h"
#include "../block/block.h"
#include "../drivers/sd/sd_block.h"

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

    if (sd_init() != SD_OK)
    {
        uart_puts("SD Card init failed!\n");
    }

    test_sd_read();
    sd_block_init();      // registers sd0

    block_device_t *dev = block_get("sd0");
    if (!dev) {
        uart_puts("No block device\n");
        return;
    }

    uint32_t sector_count = dev->sector_count();
    uart_puts("Sector count: ");
    uart_puthex(sector_count);
    uart_puts("\n");

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