/*
 * sd_emmc.c - SD Card Driver using EMMC controller
 * 
 * Must request SD power from GPU via mailbox before use!
 */

#include "sd.h"
#include "../uart/uart.h"

// EMMC registers at 0x3F300000
#define EMMC_BASE       0x3F300000

#define EMMC_ARG2       ((volatile uint32_t*)(EMMC_BASE + 0x00))
#define EMMC_BLKSIZECNT ((volatile uint32_t*)(EMMC_BASE + 0x04))
#define EMMC_ARG1       ((volatile uint32_t*)(EMMC_BASE + 0x08))
#define EMMC_CMDTM      ((volatile uint32_t*)(EMMC_BASE + 0x0C))
#define EMMC_RESP0      ((volatile uint32_t*)(EMMC_BASE + 0x10))
#define EMMC_RESP1      ((volatile uint32_t*)(EMMC_BASE + 0x14))
#define EMMC_RESP2      ((volatile uint32_t*)(EMMC_BASE + 0x18))
#define EMMC_RESP3      ((volatile uint32_t*)(EMMC_BASE + 0x1C))
#define EMMC_DATA       ((volatile uint32_t*)(EMMC_BASE + 0x20))
#define EMMC_STATUS     ((volatile uint32_t*)(EMMC_BASE + 0x24))
#define EMMC_CONTROL0   ((volatile uint32_t*)(EMMC_BASE + 0x28))
#define EMMC_CONTROL1   ((volatile uint32_t*)(EMMC_BASE + 0x2C))
#define EMMC_INTERRUPT  ((volatile uint32_t*)(EMMC_BASE + 0x30))
#define EMMC_IRPT_MASK  ((volatile uint32_t*)(EMMC_BASE + 0x34))
#define EMMC_IRPT_EN    ((volatile uint32_t*)(EMMC_BASE + 0x38))
#define EMMC_CONTROL2   ((volatile uint32_t*)(EMMC_BASE + 0x3C))
#define EMMC_SLOTISR_VER ((volatile uint32_t*)(EMMC_BASE + 0xFC))

// STATUS register bits
#define SR_READ_AVAILABLE   (1 << 11)
#define SR_WRITE_AVAILABLE  (1 << 10)
#define SR_DAT_INHIBIT      (1 << 1)
#define SR_CMD_INHIBIT      (1 << 0)

// INTERRUPT register bits
#define INT_DATA_DONE       (1 << 1)
#define INT_CMD_DONE        (1 << 0)
#define INT_READ_RDY        (1 << 5)
#define INT_WRITE_RDY       (1 << 4)
#define INT_DATA_TIMEOUT    (1 << 20)
#define INT_CMD_TIMEOUT     (1 << 16)
#define INT_ERR             (1 << 15)
#define INT_ERROR_MASK      0xFFFF0000
#define INT_ALL_MASK        0xFFFF003F

// CONTROL1 register bits
#define C1_SRST_HC          (1 << 24)
#define C1_SRST_CMD         (1 << 25)
#define C1_SRST_DATA        (1 << 26)
#define C1_TOUNIT_MAX       (0x0E << 16)
#define C1_CLK_EN           (1 << 2)
#define C1_CLK_STABLE       (1 << 1)
#define C1_CLK_INTLEN       (1 << 0)

// Command flags
#define CMD_NEED_APP        0x80000000
#define CMD_RSPNS_48        (2 << 16)
#define CMD_RSPNS_136       (1 << 16)
#define CMD_RSPNS_48B       (3 << 16)
#define CMD_ISDATA          (1 << 21)
#define CMD_DAT_DIR_CH      (1 << 4)   // Card to Host (read)
#define CMD_DAT_DIR_HC      0          // Host to Card (write)

// GPIO
#define GPIO_BASE       0x3F200000
#define GPFSEL4         ((volatile uint32_t*)(GPIO_BASE + 0x10))
#define GPFSEL5         ((volatile uint32_t*)(GPIO_BASE + 0x14))
#define GPPUD           ((volatile uint32_t*)(GPIO_BASE + 0x94))
#define GPPUDCLK1       ((volatile uint32_t*)(GPIO_BASE + 0x9C))

// Mailbox for GPU communication
#define MBOX_BASE       0x3F00B880
#define MBOX_READ       ((volatile uint32_t*)(MBOX_BASE + 0x00))
#define MBOX_STATUS     ((volatile uint32_t*)(MBOX_BASE + 0x18))
#define MBOX_WRITE      ((volatile uint32_t*)(MBOX_BASE + 0x20))
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000
#define MBOX_CHANNEL    8  // Property channel

// Commands
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
#define CMD_SEND_CSD        9

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

// Mailbox property call - CRITICAL for getting SD card access!
static volatile uint32_t __attribute__((aligned(16))) mbox_buffer[32];

static int mbox_call(uint8_t channel) {
    uint32_t addr = ((uint32_t)&mbox_buffer) & ~0xF;
    
    // Wait until mailbox is not full
    while (*MBOX_STATUS & MBOX_FULL) {
        sd_delay(1);
    }
    
    // Write address + channel
    *MBOX_WRITE = addr | channel;
    
    // Wait for response
    while (1) {
        while (*MBOX_STATUS & MBOX_EMPTY) {
            sd_delay(1);
        }
        if (*MBOX_READ == (addr | channel)) {
            return mbox_buffer[1] == 0x80000000;
        }
    }
}

static int sd_power_on(void) {
    // Request power for SD card (device ID 0)
    mbox_buffer[0] = 8 * 4;          // Buffer size
    mbox_buffer[1] = 0;              // Request code
    mbox_buffer[2] = 0x28001;        // Tag: Set power state
    mbox_buffer[3] = 8;              // Value buffer size
    mbox_buffer[4] = 8;              // Request size
    mbox_buffer[5] = 0;              // Device ID: SD card
    mbox_buffer[6] = 3;              // State: ON + WAIT
    mbox_buffer[7] = 0;              // End tag
    
    if (!mbox_call(MBOX_CHANNEL)) {
        uart_puts("SD: Power on mailbox failed\n");
        return -1;
    }
    
    uart_puts("SD: Power on result = ");
    uart_puthex(mbox_buffer[6]);
    uart_puts("\n");
    
    return 0;
}

static int sd_get_clock_rate(void) {
    // Get clock rate for EMMC
    mbox_buffer[0] = 8 * 4;
    mbox_buffer[1] = 0;
    mbox_buffer[2] = 0x30002;        // Tag: Get clock rate
    mbox_buffer[3] = 8;
    mbox_buffer[4] = 4;
    mbox_buffer[5] = 1;              // Clock ID: EMMC
    mbox_buffer[6] = 0;
    mbox_buffer[7] = 0;
    
    if (!mbox_call(MBOX_CHANNEL)) {
        return 0;
    }
    
    return mbox_buffer[6];
}

static void sd_gpio_init(void) {
    // GPIO 48-53 for EMMC - ALT3
    uint32_t sel4 = *GPFSEL4;
    uint32_t sel5 = *GPFSEL5;
    
    // GPIO 48: bits 24-26, GPIO 49: bits 27-29
    sel4 &= ~((7 << 24) | (7 << 27));
    sel4 |= (7 << 24) | (7 << 27);  // ALT3 = 7
    
    // GPIO 50-53: bits 0-2, 3-5, 6-8, 9-11
    sel5 &= ~((7 << 0) | (7 << 3) | (7 << 6) | (7 << 9));
    sel5 |= (7 << 0) | (7 << 3) | (7 << 6) | (7 << 9);  // ALT3 = 7
    
    *GPFSEL4 = sel4;
    *GPFSEL5 = sel5;
    
    sd_delay(150);
    
    // Pull-ups on data lines
    *GPPUD = 2;  // Pull-up
    sd_delay(150);
    *GPPUDCLK1 = (1 << 16) | (1 << 17) | (1 << 18) | (1 << 19) | (1 << 20) | (1 << 21);
    sd_delay(150);
    *GPPUDCLK1 = 0;
    *GPPUD = 0;
}

static int sd_wait_cmd(void) {
    int timeout = 1000000;
    while ((*EMMC_STATUS & SR_CMD_INHIBIT) && timeout--) {
        sd_delay_us(1);
    }
    return (timeout > 0) ? SD_OK : SD_TIMEOUT;
}

static int sd_send_cmd(uint32_t cmd, uint32_t arg) {
    if (sd_wait_cmd() != SD_OK) {
        uart_puts("SD: CMD inhibit timeout\n");
        return SD_TIMEOUT;
    }
    
    *EMMC_INTERRUPT = INT_ALL_MASK;
    *EMMC_ARG1 = arg;
    *EMMC_CMDTM = cmd;
    
    int timeout = 1000000;
    while (!(*EMMC_INTERRUPT & (INT_CMD_DONE | INT_ERROR_MASK)) && timeout--) {
        sd_delay_us(1);
    }
    
    uint32_t irq = *EMMC_INTERRUPT;
    
    if (timeout <= 0) {
        uart_puts("SD: CMD timeout\n");
        return SD_TIMEOUT;
    }
    
    if (irq & INT_ERROR_MASK) {
        // Don't report errors for expected failures (CMD8, ACMD41)
        *EMMC_INTERRUPT = irq;
        return SD_ERROR;
    }
    
    *EMMC_INTERRUPT = INT_CMD_DONE;
    return SD_OK;
}

static int sd_send_acmd(uint32_t cmd, uint32_t arg) {
    uint32_t cmd55 = (CMD_APP_CMD << 24) | CMD_RSPNS_48;
    if (sd_send_cmd(cmd55, sd_rca) != SD_OK) {
        return SD_ERROR;
    }
    return sd_send_cmd(cmd, arg);
}

static int sd_set_clock(uint32_t freq) {
    // Get base clock
    uint32_t base_clock = sd_get_clock_rate();
    if (base_clock == 0) {
        base_clock = 41666666;  // Default
    }
    
    uart_puts("SD: Base clock = ");
    uart_puthex(base_clock);
    uart_puts("\n");
    
    // Disable clock
    uint32_t ctrl1 = *EMMC_CONTROL1;
    ctrl1 &= ~C1_CLK_EN;
    *EMMC_CONTROL1 = ctrl1;
    sd_delay_ms(10);
    
    // Calculate divider
    uint32_t div = base_clock / freq;
    if (div < 2) div = 2;
    if (div > 0x3FF) div = 0x3FF;
    
    uart_puts("SD: Clock divider = ");
    uart_puthex(div);
    uart_puts("\n");
    
    // Set divider
    ctrl1 = *EMMC_CONTROL1;
    ctrl1 &= ~0xFFE0;
    ctrl1 |= (div & 0xFF) << 8;
    ctrl1 |= ((div >> 8) & 0x3) << 6;
    *EMMC_CONTROL1 = ctrl1;
    sd_delay_ms(10);
    
    // Enable clock
    ctrl1 |= C1_CLK_EN;
    *EMMC_CONTROL1 = ctrl1;
    
    // Wait for stable
    int timeout = 10000;
    while (!(*EMMC_CONTROL1 & C1_CLK_STABLE) && timeout--) {
        sd_delay_us(10);
    }
    
    if (timeout <= 0) {
        uart_puts("SD: Clock not stable\n");
        return SD_TIMEOUT;
    }
    
    uart_puts("SD: Clock stable\n");
    return SD_OK;
}

int sd_init(void) {
    int retries;
    
    uart_puts("\n=== SD Card Init (EMMC) ===\n");
    
    // CRITICAL: Power on SD card via mailbox
    if (sd_power_on() != 0) {
        uart_puts("SD: Failed to power on\n");
        return SD_ERROR;
    }
    
    sd_delay_ms(100);
    
    // Setup GPIO
    sd_gpio_init();
    
    // Check version
    uint32_t ver = (*EMMC_SLOTISR_VER >> 16) & 0xFF;
    uart_puts("SD: EMMC version = ");
    uart_puthex(ver);
    uart_puts("\n");
    
    // Reset controller
    *EMMC_CONTROL0 = 0;
    *EMMC_CONTROL1 = C1_SRST_HC;
    
    int timeout = 10000;
    while ((*EMMC_CONTROL1 & C1_SRST_HC) && timeout--) {
        sd_delay_us(10);
    }
    if (timeout <= 0) {
        uart_puts("SD: Reset timeout\n");
        return SD_TIMEOUT;
    }
    uart_puts("SD: Controller reset OK\n");
    
    // Setup clock and timeout
    *EMMC_CONTROL1 = C1_CLK_INTLEN | C1_TOUNIT_MAX;
    sd_delay_ms(10);
    
    // Set slow clock for init (400 kHz)
    if (sd_set_clock(400000) != SD_OK) {
        uart_puts("SD: Clock setup failed\n");
        return SD_ERROR;
    }
    
    // Enable interrupts
    *EMMC_IRPT_EN = 0;
    *EMMC_IRPT_MASK = INT_ALL_MASK;
    *EMMC_INTERRUPT = INT_ALL_MASK;
    
    sd_delay_ms(100);
    
    // CMD0 - Go idle (no response expected)
    uart_puts("SD: CMD0\n");
    *EMMC_ARG1 = 0;
    *EMMC_CMDTM = (CMD_GO_IDLE << 24);
    sd_delay_ms(50);
    *EMMC_INTERRUPT = INT_ALL_MASK;
    
    // CMD8 - Interface condition
    uart_puts("SD: CMD8\n");
    int sd_v2 = 0;
    uint32_t cmd8 = (CMD_SEND_IF_COND << 24) | CMD_RSPNS_48;
    if (sd_send_cmd(cmd8, 0x1AA) == SD_OK) {
        uint32_t resp = *EMMC_RESP0;
        uart_puts("SD: CMD8 resp = ");
        uart_puthex(resp);
        uart_puts("\n");
        if ((resp & 0xFFF) == 0x1AA) {
            uart_puts("SD: v2.0 card detected\n");
            sd_v2 = 1;
        }
    } else {
        uart_puts("SD: CMD8 failed (v1 card?)\n");
    }
    
    // ACMD41 - Send operating condition
    uart_puts("SD: ACMD41 loop\n");
    retries = 100;
    uint32_t ocr = 0;
    uint32_t acmd41_arg = sd_v2 ? 0x40FF8000 : 0x00FF8000;
    uint32_t acmd41 = (ACMD_SEND_OP_COND << 24) | CMD_RSPNS_48;
    
    do {
        if (sd_send_acmd(acmd41, acmd41_arg) == SD_OK) {
            ocr = *EMMC_RESP0;
            if (ocr & 0x80000000) {
                uart_puts("SD: Card ready, OCR = ");
                uart_puthex(ocr);
                uart_puts("\n");
                break;
            }
        }
        sd_delay_ms(50);
    } while (retries--);
    
    if (retries <= 0) {
        uart_puts("SD: ACMD41 timeout - card not ready\n");
        return SD_TIMEOUT;
    }
    
    sd_high_capacity = (ocr & 0x40000000) ? 1 : 0;
    uart_puts(sd_high_capacity ? "SD: SDHC card\n" : "SD: SDSC card\n");
    
    // CMD2 - Get CID
    uart_puts("SD: CMD2\n");
    uint32_t cmd2 = (CMD_ALL_SEND_CID << 24) | CMD_RSPNS_136;
    if (sd_send_cmd(cmd2, 0) != SD_OK) {
        uart_puts("SD: CMD2 failed\n");
        return SD_ERROR;
    }
    
    // CMD3 - Get RCA
    uart_puts("SD: CMD3\n");
    uint32_t cmd3 = (CMD_SEND_REL_ADDR << 24) | CMD_RSPNS_48;
    if (sd_send_cmd(cmd3, 0) != SD_OK) {
        uart_puts("SD: CMD3 failed\n");
        return SD_ERROR;
    }
    sd_rca = *EMMC_RESP0 & 0xFFFF0000;
    uart_puts("SD: RCA = ");
    uart_puthex(sd_rca);
    uart_puts("\n");
    
    // CMD9 - Get CSD
    uart_puts("SD: CMD9\n");
    uint32_t cmd9 = (CMD_SEND_CSD << 24) | CMD_RSPNS_136;
    if (sd_send_cmd(cmd9, sd_rca) == SD_OK) {
        uint32_t csd[4];
        csd[0] = *EMMC_RESP0;
        csd[1] = *EMMC_RESP1;
        csd[2] = *EMMC_RESP2;
        csd[3] = *EMMC_RESP3;
        
        uint32_t csd_struct = (csd[3] >> 30) & 0x3;
        if (csd_struct == 1) {
            uint32_t c_size = ((csd[2] & 0x3F) << 16) | ((csd[1] >> 16) & 0xFFFF);
            sd_total_sectors = (c_size + 1) * 1024;
            uart_puts("SD: Sectors = ");
            uart_puthex(sd_total_sectors);
            uart_puts("\n");
        }
    }
    
    // CMD7 - Select card
    uart_puts("SD: CMD7\n");
    uint32_t cmd7 = (CMD_SELECT_CARD << 24) | CMD_RSPNS_48B;
    if (sd_send_cmd(cmd7, sd_rca) != SD_OK) {
        uart_puts("SD: CMD7 failed\n");
        return SD_ERROR;
    }
    uart_puts("SD: Card selected\n");
    
    // Increase clock speed
    sd_set_clock(25000000);
    
    // Set block size
    *EMMC_BLKSIZECNT = 512;
    
    uart_puts("=== SD Ready (EMMC) ===\n\n");
    return SD_OK;
}

uint32_t sd_get_sector_count(void) {
    return sd_total_sectors;
}

int sd_read(uint32_t sector, uint32_t count, uint8_t *buffer) {
    uint32_t addr = sd_high_capacity ? sector : (sector * 512);
    uint32_t *buf = (uint32_t *)buffer;

    for (uint32_t blk = 0; blk < count; blk++) {
        int timeout = 100000;
        while ((*EMMC_STATUS & SR_DAT_INHIBIT) && timeout--) {
            sd_delay_us(1);
        }
        if (timeout <= 0) return SD_TIMEOUT;

        *EMMC_BLKSIZECNT = (1 << 16) | 512;
        *EMMC_INTERRUPT = INT_ALL_MASK;
        *EMMC_ARG1 = addr + (sd_high_capacity ? blk : blk * 512);

        uint32_t cmd = (CMD_READ_SINGLE << 24) | CMD_RSPNS_48 | CMD_ISDATA | CMD_DAT_DIR_CH;
        *EMMC_CMDTM = cmd;

        timeout = 1000000;
        while (!(*EMMC_INTERRUPT & (INT_CMD_DONE | INT_ERROR_MASK)) && timeout--) {
            sd_delay_us(1);
        }

        uint32_t irq = *EMMC_INTERRUPT;
        if (timeout <= 0 || (irq & INT_ERROR_MASK)) {
            uart_puts("SD: Read CMD error, IRQ=");
            uart_puthex(irq);
            uart_puts("\n");
            *EMMC_INTERRUPT = irq;
            return SD_ERROR;
        }
        *EMMC_INTERRUPT = INT_CMD_DONE;

        uint32_t *dest = &buf[blk * 128];
        for (int i = 0; i < 128; i++) {
            timeout = 100000;
            while (!(*EMMC_STATUS & SR_READ_AVAILABLE) && timeout--) {
                sd_delay_us(1);
            }
            if (timeout <= 0) return SD_TIMEOUT;
            dest[i] = *EMMC_DATA;
        }

        timeout = 1000000;
        while (!(*EMMC_INTERRUPT & INT_DATA_DONE) && timeout--) {
            sd_delay_us(1);
        }
        *EMMC_INTERRUPT = INT_DATA_DONE;
    }

    return SD_OK;
}

int sd_write(uint32_t sector, uint32_t count, const uint8_t *buffer) {
    uint32_t addr = sd_high_capacity ? sector : (sector * 512);
    const uint32_t *buf = (const uint32_t *)buffer;

    for (uint32_t blk = 0; blk < count; blk++) {
        int timeout = 100000;
        while ((*EMMC_STATUS & SR_DAT_INHIBIT) && timeout--) {
            sd_delay_us(1);
        }
        if (timeout <= 0) return SD_TIMEOUT;

        *EMMC_BLKSIZECNT = (1 << 16) | 512;
        *EMMC_INTERRUPT = INT_ALL_MASK;
        *EMMC_ARG1 = addr + (sd_high_capacity ? blk : blk * 512);

        uint32_t cmd = (CMD_WRITE_SINGLE << 24) | CMD_RSPNS_48 | CMD_ISDATA | CMD_DAT_DIR_HC;
        *EMMC_CMDTM = cmd;

        timeout = 1000000;
        while (!(*EMMC_INTERRUPT & (INT_CMD_DONE | INT_ERROR_MASK)) && timeout--) {
            sd_delay_us(1);
        }

        uint32_t irq = *EMMC_INTERRUPT;
        if (timeout <= 0 || (irq & INT_ERROR_MASK)) {
            uart_puts("SD: Write CMD error, IRQ=");
            uart_puthex(irq);
            uart_puts("\n");
            *EMMC_INTERRUPT = irq;
            return SD_ERROR;
        }
        *EMMC_INTERRUPT = INT_CMD_DONE;

        const uint32_t *src = &buf[blk * 128];
        for (int i = 0; i < 128; i++) {
            timeout = 100000;
            while (!(*EMMC_STATUS & SR_WRITE_AVAILABLE) && timeout--) {
                sd_delay_us(1);
            }
            if (timeout <= 0) return SD_TIMEOUT;
            *EMMC_DATA = src[i];
        }

        timeout = 1000000;
        while (!(*EMMC_INTERRUPT & INT_DATA_DONE) && timeout--) {
            sd_delay_us(1);
        }
        *EMMC_INTERRUPT = INT_DATA_DONE;
    }

    return SD_OK;
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