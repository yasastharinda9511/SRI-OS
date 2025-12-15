#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stddef.h>  

int str_cmp(const char* s1, const char* s2);
int str_startswith(const char* str, const char* prefix);
const char* str_skip_spaces(const char* str);
int str_len(const char* s);
void str_copy(char* dst, const char* src, int max);
void str_concat(char* dst, const char* src, int max);   
char *strchr(const char *s, int c);
int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

#endif