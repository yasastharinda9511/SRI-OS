#include "../drivers/gpio/gpio.h"
#include "../drivers/uart/uart.h"
#include "./interrupts/interrupts.h"
#include "./scheduler/task.h"
#include "../shell/shell.h"
#include "../drivers/sd/sd.h"
#include "../block/block.h"
#include "../drivers/sd/sd_block.h"

// Include FATFS headers
#include "../kernel/fatfs/ff.h"
#include "../kernel/fatfs/diskio.h"

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

    // Initialize SD card
    if (sd_init() != SD_OK) {
        uart_puts("SD Card init failed!\n");
    } else {
        uart_puts("SD Card initialized successfully\n");
    }
    
    sd_block_init();
    block_device_t *dev = block_get("sd0");
    if (!dev) {
        uart_puts("No block device\n");
        return;
    }
    // Mount FAT32
    static FATFS fs;
    FRESULT res;

    uart_puts("Mounting FATFS...\n");

    res = f_mount(&fs, "0:", 1);
    uart_puts("f_mount returned: ");
    uart_puthex(res);
    uart_puts("\n");

    if (res == FR_OK) {
        FIL file;
        UINT bw;

        res = f_open(&file, "0:/test.txt", FA_CREATE_ALWAYS | FA_WRITE);
        if (res == FR_OK) {
            f_write(&file, "Hello FATFS\n", 12, &bw);
            f_close(&file);
            uart_puts("File written OK\n");
        } else {
            uart_puts("f_open failed: ");
            uart_puthex(res);
            uart_puts("\n");
        }
    }else {
        uart_puts("f_mount failed\n");
    }

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
