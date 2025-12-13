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


void read_sd_card(){
    uart_puts("\n=== Reading MBR (sector 0) ===\n");
    
    uint8_t buffer[512];
    
    // Clear buffer first
    for (int i = 0; i < 512; i++) {
        buffer[i] = 0;
    }
    
    if (sd_read(0, 1, buffer) == SD_OK) {
        uart_puts("Read OK!\n\n");
        
        // Print first 32 bytes as HEX (byte by byte)
        uart_puts("First 32 bytes:\n");
        for (int i = 0; i < 32; i++) {
            // Print byte as 2 hex digits
            const char hex[] = "0123456789ABCDEF";
            uart_putc(hex[(buffer[i] >> 4) & 0x0F]);
            uart_putc(hex[buffer[i] & 0x0F]);
            uart_putc(' ');
            if ((i + 1) % 16 == 0) uart_puts("\n");
        }
        
        // Check MBR signature
        uart_puts("\nMBR signature: ");
        const char hex[] = "0123456789ABCDEF";
        uart_putc(hex[(buffer[510] >> 4) & 0x0F]);
        uart_putc(hex[buffer[510] & 0x0F]);
        uart_putc(' ');
        uart_putc(hex[(buffer[511] >> 4) & 0x0F]);
        uart_putc(hex[buffer[511] & 0x0F]);
        
        if (buffer[510] == 0x55 && buffer[511] == 0xAA) {
            uart_puts(" - VALID!\n");
        } else {
            uart_puts(" - INVALID\n");
        }
        
    } else {
        uart_puts("Read FAILED!\n");
    }
}

void kernel_main(void) {
    uart_init();
    
    uart_puts("\n\n");
    uart_puts("================================\n");
    uart_puts("  SriOS - Pi Zero 2W\n");
    uart_puts("================================\n\n");

    sd_init();

    read_sd_card();
    
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