#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

// BCM2835 Interrupt Controller registers
#define IRQ_BASE            0x2000B000

#define IRQ_BASIC_PENDING   ((volatile uint32_t*)(IRQ_BASE + 0x200))
#define IRQ_PENDING_1       ((volatile uint32_t*)(IRQ_BASE + 0x204))
#define IRQ_PENDING_2       ((volatile uint32_t*)(IRQ_BASE + 0x208))
#define IRQ_FIQ_CONTROL     ((volatile uint32_t*)(IRQ_BASE + 0x20C))
#define IRQ_ENABLE_1        ((volatile uint32_t*)(IRQ_BASE + 0x210))
#define IRQ_ENABLE_2        ((volatile uint32_t*)(IRQ_BASE + 0x214))
#define IRQ_ENABLE_BASIC    ((volatile uint32_t*)(IRQ_BASE + 0x218))
#define IRQ_DISABLE_1       ((volatile uint32_t*)(IRQ_BASE + 0x21C))
#define IRQ_DISABLE_2       ((volatile uint32_t*)(IRQ_BASE + 0x220))
#define IRQ_DISABLE_BASIC   ((volatile uint32_t*)(IRQ_BASE + 0x224))

// BCM2835 System Timer
#define SYSTIMER_BASE       0x20003000

#define SYSTIMER_CS         ((volatile uint32_t*)(SYSTIMER_BASE + 0x00))
#define SYSTIMER_CLO        ((volatile uint32_t*)(SYSTIMER_BASE + 0x04))
#define SYSTIMER_CHI        ((volatile uint32_t*)(SYSTIMER_BASE + 0x08))
#define SYSTIMER_C0         ((volatile uint32_t*)(SYSTIMER_BASE + 0x0C))
#define SYSTIMER_C1         ((volatile uint32_t*)(SYSTIMER_BASE + 0x10))
#define SYSTIMER_C2         ((volatile uint32_t*)(SYSTIMER_BASE + 0x14))
#define SYSTIMER_C3         ((volatile uint32_t*)(SYSTIMER_BASE + 0x18))

// System Timer match flags
#define SYSTIMER_M0         (1 << 0)
#define SYSTIMER_M1         (1 << 1)
#define SYSTIMER_M2         (1 << 2)
#define SYSTIMER_M3         (1 << 3)

// System Timer IRQs
#define SYSTIMER_IRQ_1      (1 << 1)
#define SYSTIMER_IRQ_3      (1 << 3)

// Timer interval (1 second = 1,000,000 us at 1MHz)
#define TIMER_INTERVAL      1000000

// Global tick counter
extern volatile uint32_t timer_ticks;

// Function prototypes
void interrupts_init(void);
void enable_irq(void);
void disable_irq(void);
void irq_handler_c(void);
void timer_init(void);
void timer_poll(void);  // New: polling function for QEMU

#endif