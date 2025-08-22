cmake_minimum_required(VERSION 3.15)
project(AETHERION_OS C ASM)

# Standard C
set(CMAKE_C_STANDARD 11)

# Dossier de sortie des fichiers compilés
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Compiler flags pour freestanding OS
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -O2 -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS "-T${CMAKE_SOURCE_DIR}/boot/link.ld -nostdlib")

# Inclure le dossier kernel pour les headers
include_directories(${CMAKE_SOURCE_DIR}/kernel)

# Activer ASM NASM
enable_language(ASM_NASM)

# Sources C
set(SOURCES
    kernel/kernel.c
    kernel/keyboard.c
    kernel/vga.c
    kernel/system.c
    kernel/ia.c
)

# Ajouter le bootloader et les sources C
add_executable(AETHERION_OS
    boot/bootloader.asm
    ${SOURCES}
)
