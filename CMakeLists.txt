cmake_minimum_required(VERSION 3.20)
project(AETHERION_OS C ASM)

set(CMAKE_C_STANDARD 11)

# Dossier de sortie
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Flags pour freestanding OS
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -O2 -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS "-T${CMAKE_SOURCE_DIR}/boot/link.ld -nostdlib")

# Inclure headers
include_directories(${CMAKE_SOURCE_DIR}/kernel)

# Activer ASM NASM
enable_language(ASM_NASM)

# Sources C
set(SOURCES
    kernel/kernel.c
    kernel/string.c
    kernel/system.c
    kernel/keyboard.c
    kernel/vga.c
    kernel/ia.c
)

# Ajouter exécutable (bootloader + kernel)
add_executable(AETHERION_OS
    boot/bootloader64.asm
    ${SOURCES}
)
