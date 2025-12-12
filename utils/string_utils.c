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