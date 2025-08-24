#ifndef MALLOC_H
#define MALLOC_H
#include "stddef.h"
void malloc_init(void* base, size_t size);
void* malloc(size_t s);
void free(void* p);
#endif
