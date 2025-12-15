#include "string_utils.h"

int str_cmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int str_startswith(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) {
            return 0;
        }
    }
    return 1;
}

const char* str_skip_spaces(const char* str) {
    while (*str == ' ') {
        str++;
    }
    return str;
}

int str_len(const char* s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

void str_copy(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}
void str_concat(char* dst, const char* src, int max) {
    int dst_len = str_len(dst);
    int i = 0;
    while (src[i] && (dst_len + i) < max - 1) {
        dst[dst_len + i] = src[i];
        i++;
    }
    dst[dst_len + i] = '\0';
}

char *strchr(const char *s, int c) {
    char ch = (char)c;

    while (*s) {
        if (*s == ch)
            return (char *)s;
        s++;
    }

    /* Check for terminating null */
    if (ch == '\0')
        return (char *)s;

    return 0;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *a = s1;
    const unsigned char *b = s2;

    while (n--) {
        if (*a != *b)
            return *a - *b;
        a++;
        b++;
    }
    return 0;
}

// memcpy: copy n bytes from src to dest
void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

// memset: fill n bytes of s with c
void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}