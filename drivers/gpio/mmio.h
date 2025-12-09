#ifndef MMIO_H
#define MMIO_H

// Pi Zero 2W only
#define PERIPHERAL_BASE     0x3F000000
#define BOARD_NAME          "Pi Zero 2W"

// Peripheral addresses
#define GPIO_BASE       (PERIPHERAL_BASE + 0x200000)
#define UART0_BASE      (PERIPHERAL_BASE + 0x201000)
#define SYSTIMER_BASE   (PERIPHERAL_BASE + 0x003000)
#define IRQ_BASE        (PERIPHERAL_BASE + 0x00B000)
#define PM_BASE         (PERIPHERAL_BASE + 0x100000)

#endif
