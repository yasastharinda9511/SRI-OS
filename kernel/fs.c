#include "fs.h"
#include "../drivers/uart/uart.h"

// File storage in RAM
static File files[MAX_FILES];

// String functions
static int str_len(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static void str_copy(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int str_cmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Initialize filesystem
void fs_init(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].used = 0;
        files[i].size = 0;
        files[i].name[0] = '\0';
        files[i].data[0] = '\0';
    }
    uart_puts("Filesystem initialized (RAM-based)\n");
}

// Find file by name, returns index or -1
static int fs_find(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && str_cmp(files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Find empty slot, returns index or -1
static int fs_find_empty(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            return i;
        }
    }
    return -1;
}

// Check if file exists
int fs_exists(const char* name) {
    return fs_find(name) >= 0;
}

// Create empty file
int fs_create(const char* name) {
    if (fs_find(name) >= 0) {
        return -1;  // Already exists
    }
    
    int idx = fs_find_empty();
    if (idx < 0) {
        return -2;  // No space
    }
    
    str_copy(files[idx].name, name, MAX_FILENAME);
    files[idx].data[0] = '\0';
    files[idx].size = 0;
    files[idx].used = 1;
    
    return 0;
}

// Delete file
int fs_delete(const char* name) {
    int idx = fs_find(name);
    if (idx < 0) {
        return -1;  // Not found
    }
    
    files[idx].used = 0;
    files[idx].name[0] = '\0';
    files[idx].size = 0;
    
    return 0;
}

// Write to file (overwrite)
int fs_write(const char* name, const char* data) {
    int idx = fs_find(name);
    if (idx < 0) {
        // Auto-create if doesn't exist
        if (fs_create(name) < 0) {
            return -1;
        }
        idx = fs_find(name);
    }
    
    int len = str_len(data);
    if (len >= MAX_FILESIZE) {
        len = MAX_FILESIZE - 1;
    }
    
    str_copy(files[idx].data, data, MAX_FILESIZE);
    files[idx].size = len;
    
    return len;
}

// Append to file
int fs_append(const char* name, const char* data) {
    int idx = fs_find(name);
    if (idx < 0) {
        return -1;
    }
    
    int current_len = files[idx].size;
    int add_len = str_len(data);
    
    if (current_len + add_len >= MAX_FILESIZE) {
        add_len = MAX_FILESIZE - current_len - 1;
    }
    
    for (int i = 0; i < add_len; i++) {
        files[idx].data[current_len + i] = data[i];
    }
    files[idx].data[current_len + add_len] = '\0';
    files[idx].size = current_len + add_len;
    
    return add_len;
}

// Read file contents
int fs_read(const char* name, char* buf, uint32_t max) {
    int idx = fs_find(name);
    if (idx < 0) {
        return -1;
    }
    
    uint32_t len = files[idx].size;
    if (len >= max) {
        len = max - 1;
    }
    
    str_copy(buf, files[idx].data, len + 1);
    
    return len;
}

// List all files
void fs_list(void) {
    int count = 0;
    
    uart_puts("\n");
    uart_puts("  Name                Size\n");
    uart_puts("  ----                ----\n");
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            uart_puts("  ");
            uart_puts(files[i].name);
            
            // Padding
            int pad = 20 - str_len(files[i].name);
            while (pad-- > 0) uart_putc(' ');
            
            // Print size
            uint32_t size = files[i].size;
            if (size == 0) {
                uart_puts("0");
            } else {
                char buf[16];
                int j = 0;
                while (size > 0) {
                    buf[j++] = '0' + (size % 10);
                    size /= 10;
                }
                while (j > 0) {
                    uart_putc(buf[--j]);
                }
            }
            uart_puts(" bytes\n");
            count++;
        }
    }
    
    if (count == 0) {
        uart_puts("  (no files)\n");
    }
    
    uart_puts("\n");
}