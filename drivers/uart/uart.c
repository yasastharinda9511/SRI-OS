#include "uart.h"

// UART0 base address for BCM2835 (Pi Zero)
#define UART0_BASE 0x20201000

// UART registers
#define UART0_DR     ((volatile unsigned int*)(UART0_BASE + 0x00))
#define UART0_FR     ((volatile unsigned int*)(UART0_BASE + 0x18))
#define UART0_IBRD   ((volatile unsigned int*)(UART0_BASE + 0x24))
#define UART0_FBRD   ((volatile unsigned int*)(UART0_BASE + 0x28))
#define UART0_LCRH   ((volatile unsigned int*)(UART0_BASE + 0x2C))
#define UART0_CR     ((volatile unsigned int*)(UART0_BASE + 0x30))
#define UART0_ICR    ((volatile unsigned int*)(UART0_BASE + 0x44))

// GPIO base address
#define GPIO_BASE    0x20200000
#define GPFSEL1      ((volatile unsigned int*)(GPIO_BASE + 0x04))
#define GPPUD        ((volatile unsigned int*)(GPIO_BASE + 0x94))
#define GPPUDCLK0    ((volatile unsigned int*)(GPIO_BASE + 0x98))

// Delay function
static void delay(int count) {
    for(volatile int i = 0; i < count; i++);
}

void uart_init(void) {
    // Disable UART0
    *UART0_CR = 0;

    // Setup GPIO pins 14 & 15 for UART
    unsigned int selector = *GPFSEL1;
    selector &= ~(7 << 12);  // Clear GPIO 14
    selector |= (4 << 12);   // Set GPIO 14 to ALT0 (TXD0)
    selector &= ~(7 << 15);  // Clear GPIO 15
    selector |= (4 << 15);   // Set GPIO 15 to ALT0 (RXD0)
    *GPFSEL1 = selector;

    // Disable pull up/down for pins 14,15
    *GPPUD = 0;
    delay(150);
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    delay(150);
    *GPPUDCLK0 = 0;

    // Clear interrupts
    *UART0_ICR = 0x7FF;

    // Set baud rate to 115200
    *UART0_IBRD = 1;
    *UART0_FBRD = 40;

    // Enable FIFO & 8-bit data transmission (1 stop bit, no parity)
    *UART0_LCRH = (1 << 4) | (3 << 5);

    // Enable UART0, receive & transfer
    *UART0_CR = (1 << 0) | (1 << 8) | (1 << 9);
}

void uart_putc(unsigned char c) {
    // Wait while transmit FIFO is full (bit 5)
    // When bit 5 = 0, FIFO has space
    while(*UART0_FR & (1 << 5));
    
    *UART0_DR = c;
}

void uart_puts(const char* str) {
    while(*str) {
        if(*str == '\n') {
            uart_putc('\r');
        }
        uart_putc(*str++);
    }
}

void uart_puthex(unsigned int num) {
    const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    for(int i = 28; i >= 0; i -= 4) {
        uart_putc(hex[(num >> i) & 0xF]);
    }
}

char uart_getc() {
    // Wait until the receive FIFO is not empty (bit 4)
    while(*UART0_FR & (1 << 4));
    
    return (char)(*UART0_DR & 0xFF);
}

int uart_getc_non_blocking(char* c) {
    // Check if the receive FIFO is empty (bit 4)
    if(*UART0_FR & (1 << 4)) {
        return 0; // No data available
    } else {
        *c = (char)(*UART0_DR & 0xFF);
        return 1; // Data read successfully
    }
}

void uart_readline(char* buffer, int max_length) {
    int index = 0;
    while (index < max_length - 1) {
        char c = uart_getc();
        if (c == '\r' || c == '\n') {
            uart_putc('\r');
            uart_putc('\n');
            break;
        } else if (c == '\b' || c == 127) { // Handle backspace
            if (index > 0) {
                index--;
                uart_puts("\b \b"); // Move cursor back, print space, move back again
            }
        } else {
            buffer[index++] = c;
            uart_putc(c); // Echo the character
        }
    }
    buffer[index] = '\0'; // Null-terminate the string
}