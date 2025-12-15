#include "diskio.h"
#include "../block/block.h"
#include "../drivers/uart/uart.h"
#include "ff.h"

extern block_device_t sd_block_dev;

/* Drive 0 = SD card */

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    return 0;
}

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    return 0;
}

DRESULT disk_read(
    BYTE pdrv,
    BYTE *buff,
    LBA_t sector,
    UINT count
) {
    if (pdrv != 0) return RES_PARERR;
    if (sd_block_dev.read(sector, count, buff) == 0){
        return RES_OK;
    }
        
    return RES_ERROR;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(
    BYTE pdrv,
    const BYTE *buff,
    LBA_t sector,
    UINT count
) {
    if (pdrv != 0) return RES_PARERR;
    if (sd_block_dev.write(sector, count, buff) == 0)
        return RES_OK;
    return RES_ERROR;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    uart_puts("disk_ioctl called\n");
    uart_puts("cmd: ");
    uart_puthex(cmd);
    uart_puts("\n");
    if (pdrv != 0) return RES_PARERR;

    switch (cmd) {

    case CTRL_SYNC:
        return RES_OK;

    case GET_SECTOR_SIZE:
        *(WORD*)buff = 512;
        return RES_OK;

    case GET_SECTOR_COUNT:
        *(DWORD*)buff = (DWORD)sd_block_dev.sector_count();
        return RES_OK;

    case GET_BLOCK_SIZE:
        *(DWORD*)buff = 1;   // erase block size in sectors
        return RES_OK;

    default:
        return RES_PARERR;
    }
}


DWORD get_fattime(void) {
    return ((DWORD)(2025-1980) << 25)  // year
         | ((DWORD)12 << 21)           // month
         | ((DWORD)14 << 16)           // day
         | ((DWORD)12 << 11)           // hour
         | ((DWORD)0 << 5)             // minute
         | ((DWORD)0 >> 1);            // seconds / 2
}