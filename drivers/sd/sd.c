/*
 * sd.c - SD Card Driver based on u-boot bcm2835_sdhost.c
 */

#include "sd.h"
#include "../uart/uart.h"

// SDHOST registers
#define SDHOST_BASE     0x3F202000

#define SDCMD           ((volatile uint32_t*)(SDHOST_BASE + 0x00))
#define SDARG           ((volatile uint32_t*)(SDHOST_BASE + 0x04))
#define SDTOUT          ((volatile uint32_t*)(SDHOST_BASE + 0x08))
#define SDCDIV          ((volatile uint32_t*)(SDHOST_BASE + 0x0C))
#define SDRSP0          ((volatile uint32_t*)(SDHOST_BASE + 0x10))
#define SDRSP1          ((volatile uint32_t*)(SDHOST_BASE + 0x14))
#define SDRSP2          ((volatile uint32_t*)(SDHOST_BASE + 0x18))
#define SDRSP3          ((volatile uint32_t*)(SDHOST_BASE + 0x1C))
#define SDHSTS          ((volatile uint32_t*)(SDHOST_BASE + 0x20))
#define SDVDD           ((volatile uint32_t*)(SDHOST_BASE + 0x30))
#define SDEDM           ((volatile uint32_t*)(SDHOST_BASE + 0x34))
#define SDHCFG          ((volatile uint32_t*)(SDHOST_BASE + 0x38))
#define SDHBCT          ((volatile uint32_t*)(SDHOST_BASE + 0x3C))
#define SDDATA          ((volatile uint32_t*)(SDHOST_BASE + 0x40))
#define SDHBLC          ((volatile uint32_t*)(SDHOST_BASE + 0x50))

// SDCMD register bits
#define SDCMD_NEW_FLAG          0x8000
#define SDCMD_FAIL_FLAG         0x4000
#define SDCMD_BUSY_CMD          0x0800
#define SDCMD_NO_RESPONSE       0x0400
#define SDCMD_LONG_RESPONSE     0x0200
#define SDCMD_WRITE_CMD         0x0080
#define SDCMD_READ_CMD          0x0040
#define SDCMD_CMD_MASK          0x003f

// SDHSTS register bits
#define SDHSTS_BUSY_IRPT        0x0400
#define SDHSTS_BLOCK_IRPT       0x0200
#define SDHSTS_SDIO_IRPT        0x0100
#define SDHSTS_REW_TIME_OUT     0x0080
#define SDHSTS_CMD_TIME_OUT     0x0040
#define SDHSTS_CRC16_ERROR      0x0020
#define SDHSTS_CRC7_ERROR       0x0010
#define SDHSTS_FIFO_ERROR       0x0008
#define SDHSTS_DATA_FLAG        0xFF

#define SDHSTS_CLEAR_MASK       0x07FF
#define SDHSTS_ERROR_MASK       (SDHSTS_CMD_TIME_OUT | SDHSTS_CRC16_ERROR | \
                                 SDHSTS_CRC7_ERROR | SDHSTS_REW_TIME_OUT | \
                                 SDHSTS_FIFO_ERROR)

// SDEDM register bits
#define SDEDM_FSM_MASK          0x000F
#define SDEDM_FSM_IDENTMODE     0x0
#define SDEDM_FSM_DATAMODE      0x1
#define SDEDM_FSM_READDATA      0x2
#define SDEDM_FSM_WRITEDATA     0x3
#define SDEDM_FSM_READWAIT      0x4
#define SDEDM_FSM_READCRC       0x5
#define SDEDM_FSM_WRITECRC      0x6
#define SDEDM_FSM_WRITEWAIT1    0x7
#define SDEDM_FSM_WRITEWAIT2    0x8
#define SDEDM_FSM_WRITESTART1   0xf
#define SDEDM_FSM_WRITESTART2   0xa
#define SDEDM_FIFO_FILL_SHIFT   4
#define SDEDM_FIFO_FILL_MASK    0x1f

#define SDEDM_WRITE_THRESHOLD_SHIFT 9
#define SDEDM_READ_THRESHOLD_SHIFT  14
#define SDEDM_THRESHOLD_MASK        0x1f

#define FIFO_READ_THRESHOLD     4
#define FIFO_WRITE_THRESHOLD    4
#define SDDATA_FIFO_WORDS       16

// GPIO registers
#define GPIO_BASE       0x3F200000
#define GPFSEL4         ((volatile uint32_t*)(GPIO_BASE + 0x10))
#define GPFSEL5         ((volatile uint32_t*)(GPIO_BASE + 0x14))
#define GPPUD           ((volatile uint32_t*)(GPIO_BASE + 0x94))
#define GPPUDCLK1       ((volatile uint32_t*)(GPIO_BASE + 0x9C))

// SD commands
#define CMD_GO_IDLE         0
#define CMD_SEND_IF_COND    8
#define CMD_STOP_TRANS      12
#define CMD_SET_BLOCKLEN    16
#define CMD_READ_SINGLE     17
#define CMD_READ_MULTI      18
#define CMD_WRITE_SINGLE    24
#define CMD_WRITE_MULTI     25
#define CMD_APP_CMD         55
#define ACMD_SEND_OP_COND   41
#define CMD_ALL_SEND_CID    2
#define CMD_SEND_REL_ADDR   3
#define CMD_SELECT_CARD     7
#define ACMD_SET_BUS_WIDTH  6
#define CMD_SEND_CSD 9

#define TEST_SECTOR      4096

static uint32_t sd_rca = 0;
static int sd_high_capacity = 0;
static uint32_t sd_total_sectors = 0;

static void sd_delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++) {
        __asm__ volatile("nop");
    }
}

static void sd_delay_us(uint32_t us) {
    sd_delay(us * 50);
}

static void sd_delay_ms(uint32_t ms) {
    sd_delay_us(ms * 1000);
}

static void sd_gpio_init(void) {
    uint32_t sel4 = *GPFSEL4;
    uint32_t sel5 = *GPFSEL5;
    
    // GPIO 48-53 for SDHOST - ALT0
    sel4 &= ~(7 << 24); sel4 |= (4 << 24);  // GPIO48
    sel4 &= ~(7 << 27); sel4 |= (4 << 27);  // GPIO49
    sel5 &= ~(7 << 0);  sel5 |= (4 << 0);   // GPIO50
    sel5 &= ~(7 << 3);  sel5 |= (4 << 3);   // GPIO51
    sel5 &= ~(7 << 6);  sel5 |= (4 << 6);   // GPIO52
    sel5 &= ~(7 << 9);  sel5 |= (4 << 9);   // GPIO53
    
    *GPFSEL4 = sel4;
    *GPFSEL5 = sel5;
    
    *GPPUD = 2;
    sd_delay(150);
    *GPPUDCLK1 = 0x3F << 16;
    sd_delay(150);
    *GPPUDCLK1 = 0;
    *GPPUD = 0;
}

static void sd_reset(void) {
    uint32_t temp;
    
    *SDVDD = 0;  // Power off
    *SDCMD = 0;
    *SDARG = 0;
    *SDTOUT = 0xF00000;
    *SDCDIV = 0;
    *SDHSTS = SDHSTS_CLEAR_MASK;
    *SDHCFG = 0;
    *SDHBCT = 0;
    *SDHBLC = 0;
    
    // Set FIFO thresholds (silicon bug workaround)
    temp = *SDEDM;
    temp &= ~((SDEDM_THRESHOLD_MASK << SDEDM_READ_THRESHOLD_SHIFT) |
              (SDEDM_THRESHOLD_MASK << SDEDM_WRITE_THRESHOLD_SHIFT));
    temp |= (FIFO_READ_THRESHOLD << SDEDM_READ_THRESHOLD_SHIFT) |
            (FIFO_WRITE_THRESHOLD << SDEDM_WRITE_THRESHOLD_SHIFT);
    *SDEDM = temp;
    
    sd_delay_ms(20);
    
    *SDVDD = 1;  // Power on
    sd_delay_ms(20);
    
    *SDHCFG = 0;
    *SDCDIV = 0x1FB;  // ~400kHz (250MHz / 0x1FB)
}

static int sd_send_cmd(uint32_t cmd, uint32_t arg, uint32_t flags) {
    uint32_t sdcmd;
    int timeout;
    
    // Wait for not busy
    timeout = 100000;
    while ((*SDCMD & SDCMD_NEW_FLAG) && timeout--) {
        sd_delay_us(1);
    }
    if (timeout <= 0) return SD_TIMEOUT;
    
    // Clear old status
    *SDHSTS = SDHSTS_CLEAR_MASK;
    
    // Set argument
    *SDARG = arg;
    
    // Build and send command
    sdcmd = (cmd & SDCMD_CMD_MASK) | SDCMD_NEW_FLAG | flags;
    *SDCMD = sdcmd;
    
    // Wait for command complete
    timeout = 100000;
    do {
        sdcmd = *SDCMD;
        sd_delay_us(1);
    } while ((sdcmd & SDCMD_NEW_FLAG) && timeout--);
    
    if (timeout <= 0) return SD_TIMEOUT;
    
    // Check for errors
    if (sdcmd & SDCMD_FAIL_FLAG) {
        uint32_t hsts = *SDHSTS;
        if (hsts & SDHSTS_ERROR_MASK) {
            *SDHSTS = SDHSTS_ERROR_MASK;
            return SD_ERROR;
        }
    }
    
    return SD_OK;
}

static int sd_send_acmd(uint32_t cmd, uint32_t arg, uint32_t flags) {
    if (sd_send_cmd(CMD_APP_CMD, sd_rca, 0) != SD_OK) {
        return SD_ERROR;
    }
    return sd_send_cmd(cmd, arg, flags);
}
int sd_init(void) {
    uint32_t temp;
    int retries;
    int32_t csd[4]; // Array to hold the CSD response
    
    uart_puts("\n=== SD Card Init ===\n");
    
    sd_gpio_init();
    sd_reset();
    
    uart_puts("SD: Reset done\n");
    
    // CMD0 - Go idle (no response)
    sd_send_cmd(CMD_GO_IDLE, 0, SDCMD_NO_RESPONSE);
    sd_delay_ms(50);
    
    // CMD8 - Interface condition
    uart_puts("SD: CMD8...\n");
    int sd_v2 = 0;
    if (sd_send_cmd(CMD_SEND_IF_COND, 0x1AA, 0) == SD_OK) {
        temp = *SDRSP0;
        if ((temp & 0xFFF) == 0x1AA) {
            uart_puts("SD: v2.0 card\n");
            sd_v2 = 1;
        }
    }
    
    // ACMD41 - Operating condition
    uart_puts("SD: ACMD41...\n");
    retries = 100;
    uint32_t ocr = 0;
    uint32_t acmd41_arg = sd_v2 ? 0x40FF8000 : 0x00FF8000;
    
    do {
        if (sd_send_acmd(ACMD_SEND_OP_COND, acmd41_arg, 0) == SD_OK) {
            ocr = *SDRSP0;
            if (ocr & 0x80000000) break;
        }
        sd_delay_ms(50);
    } while (retries--);
    
    if (retries <= 0) {
        uart_puts("SD: ACMD41 timeout\n");
        return SD_TIMEOUT;
    }
    
    sd_high_capacity = (ocr & 0x40000000) ? 1 : 0;
    uart_puts(sd_high_capacity ? "SD: SDHC card\n" : "SD: SDSC card\n");
    
    // CMD2 - Get CID (State: Identification)
    uart_puts("SD: CMD2...\n");
    if (sd_send_cmd(CMD_ALL_SEND_CID, 0, SDCMD_LONG_RESPONSE) != SD_OK) {
        return SD_ERROR;
    }
    
    // CMD3 - Get RCA (State: Stand-by)
    uart_puts("SD: CMD3...\n");
    if (sd_send_cmd(CMD_SEND_REL_ADDR, 0, 0) != SD_OK) {
        return SD_ERROR;
    }
    sd_rca = *SDRSP0 & 0xFFFF0000;
    
    /* ------------------------------------------------ */
    /* === NEW: CMD9 and Sector Calculation === */
    /* ------------------------------------------------ */
    
    if (sd_send_cmd(CMD_SEND_CSD, sd_rca, SDCMD_LONG_RESPONSE) != SD_OK) {
        uart_puts("SD: CMD9 failed\n");
        return SD_ERROR;
    }

    // Read 128-bit CSD
    csd[0] = *SDRSP0;
    csd[1] = *SDRSP1;
    csd[2] = *SDRSP2;
    csd[3] = *SDRSP3;

    // Calculate total sectors and store globally
    sd_total_sectors = 0;
    uint32_t csd_structure = (csd[3] >> 30) & 0x3;

    if (csd_structure == 1) {
        /* SDHC / SDXC (CSD v2.0) */
        uint32_t c_size = ((csd[2] & 0x3F) << 16) | ((csd[1] >> 16) & 0xFFFF);
        sd_total_sectors = (c_size + 1) * 1024;
        uart_puts("SD: Sector count (SDHC) calculated.\n");
    } else {
        /* SDSC (CSD v1.0) */
        uint32_t c_size = ((csd[2] >> 30) & 0x3) | ((csd[1] & 0x3FF) << 2) | ((csd[2] >> 16) & 0x3);
        uint32_t c_size_mult = ((csd[1] >> 15) & 0x7);
        uint32_t read_bl_len = (csd[2] >> 8) & 0xF;
        
        uint32_t block_len = 1 << read_bl_len;
        uint32_t mult = 1 << (c_size_mult + 2);
        uint32_t block_count = (c_size + 1) * mult;

        sd_total_sectors = (block_count * block_len) / 512;
        uart_puts("SD: Sector count (SDSC) calculated.\n");
    }
    
    /* ------------------------------------------------ */
    /* === END: CMD9 and Sector Calculation === */
    /* ------------------------------------------------ */

    // CMD7 - Select card (State: Transfer)
    uart_puts("SD: CMD7...\n");
    if (sd_send_cmd(CMD_SELECT_CARD, sd_rca, SDCMD_BUSY_CMD) != SD_OK) {
        return SD_ERROR;
    }

    uart_puts("SD: ACMD6 (Set 4-bit bus)...\n");
    if (sd_send_acmd(ACMD_SET_BUS_WIDTH, 2, 0) != SD_OK) {
        uart_puts("SD: ACMD6 failed\n");
        return SD_ERROR;
    }

    // 2. Tell the Controller to use 4-bit bus (SDHCFG register)
    uint32_t hcfg = *SDHCFG;
    hcfg |= 1; 
    *SDHCFG = hcfg;
    
    // Set block length for SDSC
    if (!sd_high_capacity) {
        sd_send_cmd(CMD_SET_BLOCKLEN, 512, 0);
    }
    
    // Increase clock speed
    *SDCDIV = 4;  // ~25MHz
    
    // Set block size
    *SDHBCT = 512;
    
    uart_puts("=== SD Ready ===\n\n");
    return SD_OK;
}

// And then, your sd_get_sector_count function becomes trivial:
uint32_t sd_get_sector_count(void)
{
    return sd_total_sectors;
}

int sd_read(uint32_t sector, uint32_t count, uint8_t *buffer)
{
    uint32_t addr = sd_high_capacity ? sector : sector * 512;

    for (uint32_t blk = 0; blk < count; blk++) {

        *SDHSTS = SDHSTS_CLEAR_MASK;
        *SDHBCT = 512;
        *SDHBLC = 1;

        *SDARG = addr + (sd_high_capacity ? blk : blk * 512);
        *SDCMD = CMD_READ_SINGLE | SDCMD_NEW_FLAG | SDCMD_READ_CMD;

        int timeout = 100000;
        while ((*SDCMD & SDCMD_NEW_FLAG) && timeout--)
            sd_delay_us(1);

        if (timeout <= 0 || (*SDCMD & SDCMD_FAIL_FLAG)) {
            *SDHSTS = SDHSTS_ERROR_MASK;
            return SD_ERROR;
        }

        int remaining = 512;

        while (remaining > 0) {

            int fifo =
                (*SDEDM >> SDEDM_FIFO_FILL_SHIFT) & SDEDM_FIFO_FILL_MASK;

            if (!fifo)
                continue;

            if (fifo > remaining)
                fifo = remaining;

            for (int i = 0; i < fifo; i++) {
                *buffer++ = *(volatile uint8_t *)SDDATA;
            }

            remaining -= fifo;
        }

        timeout = 100000;
        while (!(*SDHSTS & SDHSTS_BLOCK_IRPT) && timeout--) {
            if (*SDHSTS & SDHSTS_ERROR_MASK) {
                *SDHSTS = SDHSTS_ERROR_MASK;
                return SD_ERROR;
            }
        }
    }

    return SD_OK;
}



int sd_write(uint32_t sector, uint32_t count, const uint8_t *buffer)
{
    uint32_t addr = sd_high_capacity ? sector : (sector * 512);
    int timeout;

    for (uint32_t block = 0; block < count; block++) {

        *SDHSTS = SDHSTS_CLEAR_MASK;
        *SDHBCT = 512;
        *SDHBLC = 1;

        *SDARG = addr + (sd_high_capacity ? block : block * 512);
        *SDCMD = CMD_WRITE_SINGLE | SDCMD_NEW_FLAG | SDCMD_WRITE_CMD;

        timeout = 100000;
        while ((*SDCMD & SDCMD_NEW_FLAG) && timeout--) {
            sd_delay_us(1);
        }

        if (timeout <= 0 || (*SDCMD & SDCMD_FAIL_FLAG)) {
            uart_puts("SD: WRITE CMD failed\n");
            return SD_ERROR;
        }

        const uint8_t *buf = buffer + block * 512;

        /* Write 512 data bytes */
        for (int i = 0; i < 512; i++) {
            timeout = 100000;
            while (timeout--) {
                uint32_t edm = *SDEDM;
                uint32_t fifo_fill =
                    (edm >> SDEDM_FIFO_FILL_SHIFT) & SDEDM_FIFO_FILL_MASK;

                if (fifo_fill < SDDATA_FIFO_WORDS)
                    break;

                sd_delay_us(1);
            }

            if (timeout <= 0) {
                uart_puts("SD: FIFO timeout\n");
                return SD_TIMEOUT;
            }

            *(volatile uint8_t*)SDDATA = buf[i];
        }

        /* Wait for block complete */
        timeout = 1000000;
        while (!(*SDHSTS & SDHSTS_BLOCK_IRPT) && timeout--) {
            if (*SDHSTS & SDHSTS_ERROR_MASK) {
                uart_puts("SD: WRITE block error\n");
                *SDHSTS = SDHSTS_ERROR_MASK;
                return SD_ERROR;
            }
            sd_delay_us(1);
        }
    }

    return SD_OK;
}

void test_sd_write(void) {
    uart_puts("\n=== SD WRITE TEST ===\n");

    uint8_t write_buf[512];
    uint8_t read_buf[512];
    uint8_t backup_buf[512];

    // Step 1: read original sector
    if (sd_read(TEST_SECTOR, 1, backup_buf) != SD_OK) {
        uart_puts("Backup read failed!\n");
        return;
    }

    // Step 2: fill test pattern
    for (int i = 0; i < 512; i++) {
        write_buf[i] = (uint8_t)(i & 0xFF);  // 00 01 02 ... FF
    }

    // Step 3: write test pattern
    if (sd_write(TEST_SECTOR, 1, write_buf) != SD_OK) {
        uart_puts("Write failed!\n");
        return;
    }

    // Step 4: read back
    if (sd_read(TEST_SECTOR, 1, read_buf) != SD_OK) {
        uart_puts("Read-back failed!\n");
        return;
    }

    // Step 5: verify
    for (int i = 0; i < 512; i++) {
        if (read_buf[i] != write_buf[i]) {
            uart_puts("VERIFY FAILED at byte ");
            uart_puthex(i);
            uart_puts("\n");
            goto restore;
        }
    }

    uart_puts("WRITE TEST PASSED !!\n");

restore:
    // Step 6: restore original data
    sd_write(TEST_SECTOR, 1, backup_buf);
}

void test_sd_read(){
    uart_puts("\n=== Reading MBR (sector 0) ===\n");
    
    uint8_t buffer[512];
    
    // Clear buffer first
    for (int i = 0; i < 512; i++) {
        buffer[i] = 0;
    }
    
    if (sd_read(0, 1, buffer) == SD_OK) {
        uart_puts("Read OK!\n\n");
        
        // Print first 32 bytes as HEX (byte by byte)
        uart_puts("First 32 bytes:\n");
        for (int i = 0; i < 32; i++) {
            // Print byte as 2 hex digits
            const char hex[] = "0123456789ABCDEF";
            uart_putc(hex[(buffer[i] >> 4) & 0x0F]);
            uart_putc(hex[buffer[i] & 0x0F]);
            uart_putc(' ');
            if ((i + 1) % 16 == 0) uart_puts("\n");
        }
        
        // Check MBR signature
        uart_puts("\nMBR signature: ");
        const char hex[] = "0123456789ABCDEF";
        uart_putc(hex[(buffer[510] >> 4) & 0x0F]);
        uart_putc(hex[buffer[510] & 0x0F]);
        uart_putc(' ');
        uart_putc(hex[(buffer[511] >> 4) & 0x0F]);
        uart_putc(hex[buffer[511] & 0x0F]);
        
        if (buffer[510] == 0x55 && buffer[511] == 0xAA) {
            uart_puts(" - VALID!\n");
        } else {
            uart_puts(" - INVALID\n");
        }
        
    } else {
        uart_puts("Read FAILED!\n");
    }
}
