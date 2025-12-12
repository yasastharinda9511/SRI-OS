#ifndef STRING_UTILS_H
#define STRING_UTILS_H

int str_cmp(const char* s1, const char* s2);
int str_startswith(const char* str, const char* prefix);
const char* str_skip_spaces(const char* str);
int str_len(const char* s);
void str_copy(char* dst, const char* src, int max);
void str_concat(char* dst, const char* src, int max);   


#endif