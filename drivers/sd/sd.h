/*
 * sd.h - SD Card Driver for Pi Zero 2W
 * 
 * Uses EMMC controller to read/write SD card sectors
 */

#ifndef SD_H
#define SD_H

#include <stdint.h>

// Sector size (always 512 bytes for SD cards)
#define SD_SECTOR_SIZE 512

// Error codes
#define SD_OK           0
#define SD_ERROR       -1
#define SD_TIMEOUT     -2
#define SD_NOT_FOUND   -3

// Initialize SD card
// Returns: SD_OK on success, error code on failure
int sd_init(void);

// Read sectors from SD card
// sector: Starting sector number (LBA)
// count: Number of sectors to read
// buffer: Destination buffer (must be count * 512 bytes)
// Returns: SD_OK on success, error code on failure
int sd_read(uint32_t sector, uint32_t count, uint8_t* buffer);

// Write sectors to SD card
// sector: Starting sector number (LBA)
// count: Number of sectors to write
// buffer: Source buffer (must be count * 512 bytes)
// Returns: SD_OK on success, error code on failure
int sd_write(uint32_t sector, uint32_t count, const uint8_t* buffer);

// Get SD card size in sectors
uint32_t sd_get_sector_count(void);

void test_sd_write(void);
void test_sd_read(void);

#endif