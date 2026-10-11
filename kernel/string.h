#ifndef AETHERION_STRING_H
#define AETHERION_STRING_H

#include <stddef.h>
#include <stdint.h>

/* ---- string.h - Fonctions de manipulation de chaînes ---- */

/* Copie n octets de src vers dest */
void* memcpy(void* dest, const void* src, size_t n);

/* Copie n octets de src vers dest (gère le chevauchement) */
void* memmove(void* dest, const void* src, size_t n);

/* Remplit n octets de dest avec la valeur c */
void* memset(void* dest, int c, size_t n);

/* Compare n octets de s1 et s2 */
int memcmp(const void* s1, const void* s2, size_t n);

/* Recherche le caractère c dans les n premiers octets de s */
void* memchr(const void* s, int c, size_t n);

/* Copie la chaîne src vers dest (inclut le terminateur nul) */
char* strcpy(char* dest, const char* src);

/* Copie au plus n caractères de src vers dest */
char* strncpy(char* dest, const char* src, size_t n);

/* Concatène src à la fin de dest */
char* strcat(char* dest, const char* src);

/* Concatène au plus n caractères de src à la fin de dest */
char* strncat(char* dest, const char* src, size_t n);

/* Compare deux chaînes */
int strcmp(const char* s1, const char* s2);

/* Compare au plus n caractères de deux chaînes */
int strncmp(const char* s1, const char* s2, size_t n);

/* Retourne la longueur de la chaîne s (sans le terminateur nul) */
size_t strlen(const char* s);

/* Recherche la première occurrence du caractère c dans s */
char* strchr(const char* s, int c);

/* Recherche la dernière occurrence du caractère c dans s */
char* strrchr(const char* s, int c);

/* Recherche la première occurrence de l'un des caractères de accept dans s */
char* strpbrk(const char* s, const char* accept);

/* Recherche la première occurrence de la sous-chaîne needle dans haystack */
char* strstr(const char* haystack, const char* needle);

/* Calcule la longueur du préfixe de s ne contenant que des caractères de accept */
size_t strspn(const char* s, const char* accept);

/* Calcule la longueur du préfixe de s ne contenant aucun caractère de reject */
size_t strcspn(const char* s, const char* reject);

/* Recherche le prochain token dans s délimité par delim */
char* strtok(char* s, const char* delim);

/* Convertit une chaîne en entier */
int atoi(const char* str);

/* Convertit une chaîne en long */
long atol(const char* str);

/* Convertit une chaîne en unsigned long */
unsigned long strtoul(const char* str, char** endptr, int base);

/* Convertit un entier en chaîne (base 10) */
char* itoa(int value, char* str, int base);

/* Convertit un unsigned en chaîne (base 10) */
char* utoa(unsigned int value, char* str, int base);

/* Convertit un long en chaîne */
char* ltoa(long value, char* str, int base);

/* Convertit un unsigned long en chaîne */
char* ultoa(unsigned long value, char* str, int base);

#endif /* AETHERION_STRING_H */