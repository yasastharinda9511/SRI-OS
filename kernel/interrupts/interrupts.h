#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

#define PERIPHERAL_BASE     0x3F000000

// ARM Timer (more reliable on Pi Zero 2W)
#define ARM_TIMER_BASE      (PERIPHERAL_BASE + 0xB000)
#define ARM_TIMER_LOAD      ((volatile uint32_t*)(ARM_TIMER_BASE + 0x400))
#define ARM_TIMER_VALUE     ((volatile uint32_t*)(ARM_TIMER_BASE + 0x404))
#define ARM_TIMER_CTRL      ((volatile uint32_t*)(ARM_TIMER_BASE + 0x408))
#define ARM_TIMER_IRQ_CLR   ((volatile uint32_t*)(ARM_TIMER_BASE + 0x40C))
#define ARM_TIMER_RAW_IRQ   ((volatile uint32_t*)(ARM_TIMER_BASE + 0x410))
#define ARM_TIMER_MASK_IRQ  ((volatile uint32_t*)(ARM_TIMER_BASE + 0x414))
#define ARM_TIMER_RELOAD    ((volatile uint32_t*)(ARM_TIMER_BASE + 0x418))
#define ARM_TIMER_PREDIV    ((volatile uint32_t*)(ARM_TIMER_BASE + 0x41C))

// Interrupt Controller
#define IRQ_BASIC_PENDING   ((volatile uint32_t*)(ARM_TIMER_BASE + 0x200))
#define IRQ_PENDING_1       ((volatile uint32_t*)(ARM_TIMER_BASE + 0x204))
#define IRQ_PENDING_2       ((volatile uint32_t*)(ARM_TIMER_BASE + 0x208))
#define IRQ_ENABLE_BASIC    ((volatile uint32_t*)(ARM_TIMER_BASE + 0x218))
#define IRQ_ENABLE_1        ((volatile uint32_t*)(ARM_TIMER_BASE + 0x210))
#define IRQ_DISABLE_BASIC   ((volatile uint32_t*)(ARM_TIMER_BASE + 0x224))

// System Timer (keep for reference)
#define SYSTIMER_BASE       (PERIPHERAL_BASE + 0x3000)
#define SYSTIMER_CS         ((volatile uint32_t*)(SYSTIMER_BASE + 0x00))
#define SYSTIMER_CLO        ((volatile uint32_t*)(SYSTIMER_BASE + 0x04))
#define SYSTIMER_C1         ((volatile uint32_t*)(SYSTIMER_BASE + 0x10))
#define SYSTIMER_M1         (1 << 1)

#define TIMER_INTERVAL      10000

extern volatile uint32_t timer_ticks;

void interrupts_init(void);
void timer_init(void);
void enable_irq(void);
void disable_irq(void);
void irq_handler_c(void);

#endif