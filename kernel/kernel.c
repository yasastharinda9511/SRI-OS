// #include "../drivers/uart/uart.h"
// #include "./interrupts/interrupts.h"
// #include "shell.h"
// #include "scheduler/task.h"
// #include "../drivers/gpio/gpio.h"

// void delay(int count) {
//     for (volatile int i = 0; i < count; i++){
//         // uart_putc('.');
//     }
//     // uart_putc('\n');
// }

// void task_counter_1(void) {
//     int count = 0;
//     while (1) {
//         uart_puts("[Counter 1] ");
        
//         // Print number
//         char buf[16];
//         int i = 0;
//         int n = count;
//         if (n == 0) buf[i++] = '0';
//         else while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
//         while (i > 0) uart_putc(buf[--i]);
//         uart_puts("\n");
        
//         count++;
//         task_yield();  // Give other tasks a chance
        
//         // Simple delay
//         for (volatile int j = 0; j < 1000000; j++);
//     }
// }

// void task_counter_2(void) {
//     int count = 0;
//     while (1) {
//         uart_puts("[Counter 2] ");
        
//         // Print number
//         char buf[16];
//         int i = 0;
//         int n = count;
//         if (n == 0) buf[i++] = '0';
//         else while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
//         while (i > 0) uart_putc(buf[--i]);
//         uart_puts("\n");
        
//         count++;
//         task_yield();  // Give other tasks a chance
        
//         // Simple delay
//         for (volatile int j = 0; j < 1000000; j++);
//     }
// }

// void finite_interation(void){
//     for(int i=0;i<5000;i++){
//         uart_puts("Finite Task Iteration: ");
//         uart_putc('0' + i);
//         uart_puts("\n");
//         task_yield();
//     }
// }

// void kernel_main(void) {
//     // uart_init();

//     // uart_puts("\n");
//     // uart_puts("================================\n");
//     // uart_puts("  Pi Zero Bare Metal OS\n");
//     // uart_puts("================================\n\n");

//     // Initialize interrupts and timer
//     // interrupts_init();
//     // timer_init();
//     // enable_irq();

//     // uart_puts("System initialized.\n");

//     // // Initialize scheduler
//     // scheduler_init();
//     // scheduler_start();
//     // uart_puts("Scheduler started.\n");
//     // // Create sample tasks
//     // task_create("Counter1", task_counter_1, 1);
//     // task_create("Counter2", task_counter_2, 1);
//     // task_create("FiniteTask", finite_interation, 1);
//     // scheduler_start();

//     // Start shell
//     // shell_init();
//     // shell_run();
    
//     // Never reaches here
//     int count = 0;
//     gpio_set_output(23);
//     while (1) {
        
//         // Toggle GPIO 23
//         if (count % 2 == 0) {
//             gpio_high(23);
//             // uart_puts("HIGH\n");
//         } else {
//             gpio_low(23);
//             // uart_puts("LOW\n");
//         }
        
//         count++;
//         delay(3000000);
//     }
// }


// Test kernel - Blinks ACT LED AND GPIO 23
// This will prove if kernel is running

#define PERIPHERAL_BASE 0x3F000000
#define GPIO_BASE       (PERIPHERAL_BASE + 0x200000)

// GPIO 23 (Pin 16)
#define GPFSEL2     (*(volatile unsigned int*)(GPIO_BASE + 0x08))
#define GPSET0      (*(volatile unsigned int*)(GPIO_BASE + 0x1C))
#define GPCLR0      (*(volatile unsigned int*)(GPIO_BASE + 0x28))

// GPIO 47 (ACT LED on board)
#define GPFSEL4     (*(volatile unsigned int*)(GPIO_BASE + 0x10))
#define GPSET1      (*(volatile unsigned int*)(GPIO_BASE + 0x20))
#define GPCLR1      (*(volatile unsigned int*)(GPIO_BASE + 0x2C))

void delay(int count) {
    for (volatile int i = 0; i < count; i++);
}

void kernel_main(void) {
    // Setup GPIO 23 as output
    unsigned int val2 = GPFSEL2;
    val2 &= ~(7 << 9);
    val2 |= (1 << 9);
    GPFSEL2 = val2;
    
    // Setup GPIO 47 (ACT LED) as output
    unsigned int val4 = GPFSEL4;
    val4 &= ~(7 << 21);
    val4 |= (1 << 21);
    GPFSEL4 = val4;
    
    // Blink both forever
    while (1) {
        // Both ON
        GPSET0 = (1 << 23);       // GPIO 23 HIGH
        GPSET1 = (1 << 15);       // ACT LED ON (GPIO 47)
        delay(2000000);
        
        // Both OFF
        GPCLR0 = (1 << 23);       // GPIO 23 LOW
        GPCLR1 = (1 << 15);       // ACT LED OFF
        delay(2000000);
    }
}