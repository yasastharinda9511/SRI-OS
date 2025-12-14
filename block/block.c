#include "block.h"
#include "../drivers/uart/uart.h"
#include "../utils/string_utils.h"

#define MAX_BLOCK_DEVICES 4

static block_device_t *devices[MAX_BLOCK_DEVICES];
static int device_count = 0;

void block_register(block_device_t *dev) {
    if (device_count >= MAX_BLOCK_DEVICES) {
        uart_puts("BLOCK: too many devices\n");
        return;
    }

    devices[device_count++] = dev;

    uart_puts("BLOCK: registered ");
    uart_puts(dev->name);
    uart_puts("\n");
}

block_device_t *block_get(const char *name) {
    for (int i = 0; i < device_count; i++) {
        if (!str_cmp(devices[i]->name, name)) {
            return devices[i];
        }
    }
    return 0;
}
