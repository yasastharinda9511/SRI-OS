#include "interrupts.h"
#include "../drivers/uart/uart.h"
#include "scheduler/task.h"

volatile uint32_t timer_ticks = 0;

void interrupts_init(void) {
    uart_puts("Interrupts init\n");
}

void timer_init(void) {
    uart_puts("ARM Timer setup...\n");
    
    *ARM_TIMER_CTRL = 0;
    *ARM_TIMER_LOAD = 1000000;
    *ARM_TIMER_RELOAD = 1000000;
    *ARM_TIMER_PREDIV = 249;
    *ARM_TIMER_IRQ_CLR = 0;
    *IRQ_ENABLE_BASIC = (1 << 0);
    *ARM_TIMER_CTRL = (1 << 7) | (1 << 5) | (1 << 1);
    
    uart_puts("Timer started\n");
}

void enable_irq(void) {
    __asm__ __volatile__("cpsie i" ::: "memory");
}

void disable_irq(void) {
    __asm__ __volatile__("cpsid i" ::: "memory");
}

void irq_handler_c(void) {
    // First thing - print to show we're here
    uart_putc('!');
    
    // Clear interrupt
    *ARM_TIMER_IRQ_CLR = 0;
    
    timer_ticks++;
    
    // Toggle GPIO 23
    static int toggle = 0;
    if (toggle) {
        *((volatile uint32_t*)0x3F200028) = (1 << 23);
    } else {
        *((volatile uint32_t*)0x3F20001C) = (1 << 23);
    }
    toggle = !toggle;

    // scheduler_tick();
    
    uart_putc('T');
}