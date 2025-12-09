#ifndef GPIO_H
#define GPIO_H

void gpio_init(void);
void gpio_set_output(int pin);
void gpio_set_input(int pin);
void gpio_high(int pin);
void gpio_low(int pin);
void gpio_toggle(int pin);
int gpio_read(int pin);

#endif