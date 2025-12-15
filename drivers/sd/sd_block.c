
#include "sd.h"
#include "../uart/uart.h"
#include "../../block/block.h"

#include "block.h"
#include "../sd/sd.h"

static int sd_block_read(
    uint32_t lba,
    uint32_t count,
    uint8_t *buffer
) {
    return sd_read(lba, count, buffer);
}

static int sd_block_write(
    uint32_t lba,
    uint32_t count,
    const uint8_t *buffer
) {
    return sd_write(lba, count, buffer);
}

static uint32_t sd_block_sector_count(void) {
    return sd_get_sector_count();
}

block_device_t sd_block_dev = {
    .name = "sd0",
    .read = sd_block_read,
    .write = sd_block_write,
    .sector_count = sd_block_sector_count,
    .sector_size = 512,
};

void sd_block_init(void) {
    block_register(&sd_block_dev);
}