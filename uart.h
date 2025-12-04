#ifndef UART_H
#define UART_H

void uart_init(void);
void uart_putc(unsigned char c);
void uart_puts(const char* str);
void uart_puthex(unsigned int num);

char uart_getc();
int uart_getc_non_blocking(char* c);
void uart_readline(char* buffer, int max_length);
#endif