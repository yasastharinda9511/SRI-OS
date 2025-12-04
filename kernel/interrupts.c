#include "interrupts.h"
#include "../drivers/uart.h"

volatile uint32_t timer_ticks = 0;
static uint32_t next_tick_time = 0;

extern uint32_t _vectors[];

void interrupts_init(void) {
    uint32_t* vectors_src = _vectors;
    uint32_t* vectors_dst = (uint32_t*)0x0000;
    
    for (int i = 0; i < 16; i++) {
        vectors_dst[i] = vectors_src[i];
    }
    
    uart_puts("Vector table installed at 0x0000\n");
}

void timer_init(void) {
    // Initialize next tick time
    next_tick_time = *SYSTIMER_CLO + TIMER_INTERVAL;
    
    uart_puts("Timer initialized (polling mode for QEMU)\n");
    uart_puts("SYSTIMER_CLO = ");
    uart_puthex(*SYSTIMER_CLO);
    uart_puts("\n");
}

// Call this from main loop to check for timer events
void timer_poll(void) {
    uint32_t current = *SYSTIMER_CLO;
    
    // Check if we've passed the next tick time
    // Handle wraparound by checking if difference is "small" (less than half the range)
    if ((current - next_tick_time) < 0x80000000) {
        timer_ticks++;
        next_tick_time += TIMER_INTERVAL;
        
        uart_puts("Tick: ");
        uart_puthex(timer_ticks);
        uart_puts(" (CLO=");
        uart_puthex(current);
        uart_puts(")\n");
    }
}

void enable_irq(void) {
    uint32_t cpsr_before, cpsr_after;
    
    __asm__ __volatile__("mrs %0, cpsr" : "=r"(cpsr_before));
    
    __asm__ __volatile__(
        "mrs r0, cpsr\n"
        "bic r0, r0, #0x80\n"
        "msr cpsr_c, r0\n"
        ::: "r0"
    );
    
    __asm__ __volatile__("mrs %0, cpsr" : "=r"(cpsr_after));
    
    uart_puts("CPSR: ");
    uart_puthex(cpsr_before);
    uart_puts(" -> ");
    uart_puthex(cpsr_after);
    uart_puts("\n");
}

void disable_irq(void) {
    __asm__ __volatile__(
        "mrs r0, cpsr\n"
        "orr r0, r0, #0x80\n"
        "msr cpsr_c, r0\n"
        ::: "r0"
    );
}

// This will work on real hardware when timer compare IRQs work
void irq_handler_c(void) {
    if (*SYSTIMER_CS & SYSTIMER_M1) {
        *SYSTIMER_CS = SYSTIMER_M1;
        *SYSTIMER_C1 = *SYSTIMER_CLO + TIMER_INTERVAL;
        timer_ticks++;
        
        uart_puts("IRQ Tick: ");
        uart_puthex(timer_ticks);
        uart_puts("\n");
    }
}