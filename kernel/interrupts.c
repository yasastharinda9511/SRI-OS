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

    *SYSTIMER_C1 = *SYSTIMER_CLO + TIMER_INTERVAL;
    
    *SYSTIMER_CS = SYSTIMER_M1;

    *IRQ_ENABLE_1 = (1 << 1);  // Bit 1 = System Timer 1 IRQ

    uart_puts("Timer initialized and IRQ enabled.\n");
    uart_puts("SYSTIMER_CLO = ");
    uart_puthex(*SYSTIMER_CLO);
    uart_puts("\n");
}

void enable_irq(void) {
    __asm__ __volatile__(
        "mrs r0, cpsr\n"
        "bic r0, r0, #0x80\n"
        "msr cpsr_c, r0\n"
        ::: "r0"
    );
}

// Disable IRQs globally
void disable_irq(void) {
    __asm__ __volatile__(
        "mrs r0, cpsr\n"
        "orr r0, r0, #0x80\n"
        "msr cpsr_c, r0\n"
        ::: "r0"
    );
}

// IRQ handler called from assembly
void irq_handler_c(void) {
    // Check if this is System Timer 1 IRQ
    if (*SYSTIMER_CS & SYSTIMER_M1) {
        // Clear interrupt
        *SYSTIMER_CS = SYSTIMER_M1;

        // Schedule next tick
        *SYSTIMER_C1 = *SYSTIMER_CLO + TIMER_INTERVAL;

        timer_ticks++;

        uart_puts("IRQ Tick: ");
        uart_puthex(timer_ticks);
        uart_puts("\n");
    } else {
        uart_puts("Unknown IRQ\n");
    }
}

void timer_poll(void) {
    uint32_t current = *SYSTIMER_CLO;

    if ((current - next_tick_time) < 0x80000000) {
        timer_ticks++;
        next_tick_time += TIMER_INTERVAL;

        uart_puts("Tick (poll): ");
        uart_puthex(timer_ticks);
        uart_puts(" CLO=");
        uart_puthex(current);
        uart_puts("\n");
    }
}
