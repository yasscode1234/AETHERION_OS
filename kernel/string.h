#ifndef STRING_H
#define STRING_H

#include <stddef.h>  // pour size_t

// Comparaison
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);

// Copie
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);

// Longueur
size_t strlen(const char* s);

// Concaténation
char* strcat(char* dest, const char* src);

// Recherche
char* strchr(const char* s, int c);

// Conversion
int atoi(const char* str);

// Format minimal (supporte %d et %s)
int sprintf(char* buffer, const char* format, ...);

#endif
