#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>

#define BLOCK_OK        0
#define BLOCK_ERROR    -1
#define BLOCK_TIMEOUT  -2

#define BLOCK_SECTOR_SIZE 512

typedef struct block_device {
    const char *name;

    int (*read)(
        uint32_t lba,
        uint32_t count,
        uint8_t *buffer
    );

    int (*write)(
        uint32_t lba,
        uint32_t count,
        const uint8_t *buffer
    );

    uint32_t (*sector_count)(void);

    uint32_t sector_size;
} block_device_t;

/* Register / access block devices */
void block_register(block_device_t *dev);
block_device_t *block_get(const char *name);

#endif
