#include "../drivers/gpio/gpio.h"
#include "../drivers/uart/uart.h"
#include "./interrupts/interrupts.h"
#include "./scheduler/task.h"
#include "../shell/shell.h"
#include "../drivers/sd/sd.h"
#include "../block/block.h"
#include "../drivers/sd/sd_block.h"

#include "../kernel/fatfs/ff.h"
#include "../kernel/fatfs/diskio.h"

/* Background task - blink LED */
void task_blink(void) {
    gpio_set_output(23);

    while (1) {
        gpio_high(23);
        for (volatile int i = 0; i < 2000000; i++);

        gpio_low(23);
        for (volatile int i = 0; i < 2000000; i++);
    }
}

void kernel_main(void) {
    uart_init();

    uart_puts("\n\n");
    uart_puts("================================\n");
    uart_puts("  SriOS - Pi Zero 2W\n");
    uart_puts("================================\n\n");

    /* -------- SD CARD INIT -------- */
    if (sd_init() != SD_OK) {
        uart_puts("SD Card init failed!\n");
    } else {
        uart_puts("SD Card initialized successfully\n");
    }

    sd_block_init();

    block_device_t *dev = block_get("sd0");
    if (!dev) {
        uart_puts("No block device found\n");
        while (1);
    }

    /* -------- FATFS MOUNT -------- */
    static FATFS fs;
    FRESULT res;

    uart_puts("Mounting FATFS...\n");
    res = f_mount(&fs, "0:", 1);

    uart_puts("f_mount returned: ");
    uart_puthex(res);
    uart_puts("\n");

    if (res != FR_OK) {
        uart_puts("FATFS mount failed\n");
        while (1);
    }

    uart_puts("FATFS mounted successfully\n");

    /* -------- SET VECTOR BASE -------- */
    extern char _vectors;
    uint32_t vec_addr = (uint32_t)&_vectors;
    __asm__ __volatile__("mcr p15, 0, %0, c12, c0, 0" :: "r"(vec_addr));

    /* -------- SCHEDULER -------- */
    scheduler_init();

    task_create("Shell", shell_task, 1);
    task_create("Blink", task_blink, 1);

    /* -------- INTERRUPTS -------- */
    interrupts_init();
    timer_init();

    uart_puts("Enabling IRQ...\n");
    enable_irq();
    uart_puts("IRQ enabled!\n\n");

    /* -------- START OS -------- */
    scheduler_start();

    while (1);
}