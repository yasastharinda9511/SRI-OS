#ifndef FS_H
#define FS_H

#include <stdint.h>

#define MAX_FILES       16
#define MAX_FILENAME    32
#define MAX_FILESIZE    1024

// File structure
typedef struct {
    char name[MAX_FILENAME];
    char data[MAX_FILESIZE];
    uint32_t size;
    uint8_t used;
} File;

// Filesystem functions
void fs_init(void);
int fs_create(const char* name);
int fs_delete(const char* name);
int fs_write(const char* name, const char* data);
int fs_append(const char* name, const char* data);
int fs_read(const char* name, char* buf, uint32_t max);
void fs_list(void);
int fs_exists(const char* name);

#endif