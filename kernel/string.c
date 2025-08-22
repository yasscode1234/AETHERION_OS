#include "string.h"
#include <stdarg.h>  // pour sprintf

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++; s2++; n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

char* strcat(char* dest, const char* src) {
    char* d = dest + strlen(dest);
    while ((*d++ = *src++));
    return dest;
}

char* strchr(const char* s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    return NULL;
}

int atoi(const char* str) {
    int res = 0;
    int sign = 1;
    if (*str == '-') { sign = -1; str++; }
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res * sign;
}

// Mini sprintf (supporte %d et %s)
int sprintf(char* buffer, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char* buf_ptr = buffer;

    while (*format) {
        if (*format == '%' && *(format + 1)) {
            format++;
            if (*format == 'd') {
                int val = va_arg(args, int);
                char temp[32];
                int i = 0, neg = 0;
                if (val < 0) { neg = 1; val = -val; }
                do {
                    temp[i++] = (val % 10) + '0';
                    val /= 10;
                } while (val > 0);
                if (neg) temp[i++] = '-';
                while (i--) *buf_ptr++ = temp[i];
            }
            else if (*format == 's') {
                char* str = va_arg(args, char*);
                while (*str) *buf_ptr++ = *str++;
            }
            else {
                *buf_ptr++ = *format;
            }
        } else {
            *buf_ptr++ = *format;
        }
        format++;
    }
    *buf_ptr = '\0';
    va_end(args);
    return buf_ptr - buffer;
}
