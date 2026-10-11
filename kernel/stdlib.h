#ifndef AETHERION_STDLIB_H
#define AETHERION_STDLIB_H

#include <stddef.h>
#include <stdint.h>

/* ---- stdlib.h - Fonctions utilitaires générales ---- */

/* Allocation mémoire dynamique (simple bump allocator) */
void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);

/* Conversion chaîne -> nombre */
int atoi(const char* str);
long atol(const char* str);
long long atoll(const char* str);
double atof(const char* str);

long strtol(const char* str, char** endptr, int base);
unsigned long strtoul(const char* str, char** endptr, int base);
long long strtoll(const char* str, char** endptr, int base);
unsigned long long strtoull(const char* str, char** endptr, int base);

/* Génération de nombres aléatoires (simple LCG) */
int rand(void);
void srand(unsigned int seed);

/* Recherche et tri */
void* bsearch(const void* key, const void* base, size_t nmemb, size_t size,
              int (*compar)(const void*, const void*));
void qsort(void* base, size_t nmemb, size_t size,
           int (*compar)(const void*, const void*));

/* Valeurs absolues */
int abs(int x);
long labs(long x);
long long llabs(long long x);

/* Division avec quotient et reste */
typedef struct { int quot; int rem; } div_t;
typedef struct { long quot; long rem; } ldiv_t;
typedef struct { long long quot; long long rem; } lldiv_t;

div_t div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);

/* Gestion de l'environnement (stub pour compatibilité) */
char* getenv(const char* name);
int system(const char* command);

/* Sortie du programme */
void exit(int status);
void abort(void);

/* Alignement mémoire */
void* aligned_alloc(size_t alignment, size_t size);

/* ---- Constantes ---- */
#define NULL ((void*)0)
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767

#endif /* AETHERION_STDLIB_H */