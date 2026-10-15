cmake_minimum_required(VERSION 3.16)
project(AETHERION_OS2 LANGUAGES C ASM)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Architecture and target
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Cross-compilation toolchain for bare metal
set(CMAKE_C_COMPILER_TARGET x86_64-elf)
set(CMAKE_ASM_COMPILER_TARGET x86_64-elf)
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

# Compiler flags for kernel development
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -fno-pic -mno-red-zone -mcmodel=large -mno-mmx -mno-sse -mno-sse2 -Wa,--64")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -f elf64")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--oformat=elf64-x86-64")

# Linker flags - explicitly set to avoid CMake default flags that x86_64-elf-ld doesn't understand
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -nostartfiles -nodefaultlibs -T ${CMAKE_SOURCE_DIR}/boot/link.ld")
set(CMAKE_SHARED_LINKER_FLAGS "")
set(CMAKE_MODULE_LINKER_FLAGS "")
set(CMAKE_STATIC_LINKER_FLAGS "")

# Disable all default linker flags that might cause issues
set(CMAKE_EXE_LINKER_FLAGS_INIT "")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "")
set(CMAKE_STATIC_LINKER_FLAGS_INIT "")

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Source files
set(KERNEL_SOURCES
    kernel/kernel.c
    kernel/vga.c
    kernel/keyboard.c
    kernel/disk.c
    kernel/ata.c
    kernel/ahci.c
    kernel/pci.c
)

set(STAGE1_SOURCES
    boot/stage1.asm
)

set(STAGE2_SOURCES
    boot/stage2.asm
)

# Kernel object files
add_library(kernel OBJECT ${KERNEL_SOURCES})
target_include_directories(kernel PUBLIC kernel)

# Final kernel executable (ELF)
add_executable(aetherion.elf $<TARGET_OBJECTS:kernel>)
set_target_properties(aetherion.elf PROPERTIES
    OUTPUT_NAME "aetherion"
    SUFFIX ".elf"
)

# Convert kernel ELF to raw binary
add_custom_target(kernel_bin
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_BINARY_DIR}/bin/aetherion.elf ${CMAKE_BINARY_DIR}/bin/kernel.bin
    DEPENDS aetherion.elf
    COMMENT "Converting kernel ELF to raw binary"
)

# Generate kernel_sectors.inc with computed sector count (using CMake script)
add_custom_target(kernel_sectors
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/generate_kernel_sectors.cmake
    DEPENDS kernel_bin
    COMMENT "Generating kernel_sectors.inc"
)

# Stage 1 bootloader (512 bytes)
add_custom_target(stage1_bin
    COMMAND nasm -f bin ${CMAKE_SOURCE_DIR}/boot/stage1.asm -o ${CMAKE_BINARY_DIR}/bin/stage1.bin
    DEPENDS ${STAGE1_SOURCES}
    COMMENT "Assembling Stage 1 bootloader"
)

# Stage 2 bootloader (includes kernel_sectors.inc)
add_custom_target(stage2_bin
    COMMAND nasm -f bin ${CMAKE_SOURCE_DIR}/boot/stage2.asm -o ${CMAKE_BINARY_DIR}/bin/stage2.bin
    DEPENDS ${STAGE2_SOURCES} kernel_sectors
    COMMENT "Assembling Stage 2 bootloader"
)

# Create raw disk image (Stage 1 + Stage 2 + kernel)
# Layout: Sector 0 = Stage 1 (512 bytes)
#         Sectors 1-16 = Stage 2 (16 sectors = 8KB)
#         Sectors 18+ = Kernel
add_custom_target(disk_image
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/create_disk_image.cmake
    DEPENDS stage1_bin stage2_bin kernel_bin
    COMMENT "Creating raw disk image (aetherion.img)"
)

# Default target
add_custom_target(aetherion_all DEPENDS aetherion.elf disk_image)

# Clean target
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMENT "Cleaning build directory"
)

# Install target (optional)
install(TARGETS aetherion.elf DESTINATION bin)
