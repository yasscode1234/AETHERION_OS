#ifndef AETHERION_KERNEL_H
#define AETHERION_KERNEL_H

/* ---- Standard Library (minimal) ---- */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ---- Types de base ---- */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* ---- Point d'entree du kernel ---- */
void kernel_main(void);

#endif /* AETHERION_KERNEL_H */
