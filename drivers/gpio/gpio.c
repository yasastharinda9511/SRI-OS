#include "gpio.h"
#include "mmio.h"

// GPIO registers
#define GPFSEL0     ((volatile unsigned int*)(GPIO_BASE + 0x00))
#define GPFSEL1     ((volatile unsigned int*)(GPIO_BASE + 0x04))
#define GPFSEL2     ((volatile unsigned int*)(GPIO_BASE + 0x08))
#define GPFSEL3     ((volatile unsigned int*)(GPIO_BASE + 0x0C))
#define GPFSEL4     ((volatile unsigned int*)(GPIO_BASE + 0x10))
#define GPFSEL5     ((volatile unsigned int*)(GPIO_BASE + 0x14))
#define GPSET0      ((volatile unsigned int*)(GPIO_BASE + 0x1C))
#define GPSET1      ((volatile unsigned int*)(GPIO_BASE + 0x20))
#define GPCLR0      ((volatile unsigned int*)(GPIO_BASE + 0x28))
#define GPCLR1      ((volatile unsigned int*)(GPIO_BASE + 0x2C))
#define GPLEV0      ((volatile unsigned int*)(GPIO_BASE + 0x34))
#define GPLEV1      ((volatile unsigned int*)(GPIO_BASE + 0x38))

// Get GPFSEL register for a pin
static volatile unsigned int* get_gpfsel(int pin) {
    if (pin < 10) return GPFSEL0;
    if (pin < 20) return GPFSEL1;
    if (pin < 30) return GPFSEL2;
    if (pin < 40) return GPFSEL3;
    if (pin < 50) return GPFSEL4;
    return GPFSEL5;
}

void gpio_init(void) {
    // Nothing needed for now
}

void gpio_set_output(int pin) {
    volatile unsigned int* gpfsel = get_gpfsel(pin);
    int shift = (pin % 10) * 3;
    
    unsigned int val = *gpfsel;
    val &= ~(7 << shift);    // Clear bits
    val |= (1 << shift);     // Set as output (001)
    *gpfsel = val;
}

void gpio_set_input(int pin) {
    volatile unsigned int* gpfsel = get_gpfsel(pin);
    int shift = (pin % 10) * 3;
    
    unsigned int val = *gpfsel;
    val &= ~(7 << shift);    // Clear bits (000 = input)
    *gpfsel = val;
}

void gpio_high(int pin) {
    if (pin < 32) {
        *GPSET0 = (1 << pin);
    } else {
        *GPSET1 = (1 << (pin - 32));
    }
}

void gpio_low(int pin) {
    if (pin < 32) {
        *GPCLR0 = (1 << pin);
    } else {
        *GPCLR1 = (1 << (pin - 32));
    }
}

void gpio_toggle(int pin) {
    if (gpio_read(pin)) {
        gpio_low(pin);
    } else {
        gpio_high(pin);
    }
}

int gpio_read(int pin) {
    if (pin < 32) {
        return (*GPLEV0 >> pin) & 1;
    } else {
        return (*GPLEV1 >> (pin - 32)) & 1;
    }
}