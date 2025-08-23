# ----------------- Variables -----------------
NASM       := C:/mingw64/bin/nasm.exe
CC         := C:/mingw64/bin/gcc.exe
LD         := C:/mingw64/bin/ld.exe
CFLAGS     := -ffreestanding -O2 -Wall -Wextra -Ikernel
LDFLAGS    := -Tboot/link.ld -nostdlib

BUILD_DIR  := build
KERNEL_SRCS := $(wildcard kernel/*.c)
KERNEL_OBJS := $(patsubst kernel/%.c,$(BUILD_DIR)/%.o,$(KERNEL_SRCS))
ASM_SRCS   := boot/bootloader.asm
ASM_OBJS   := $(patsubst boot/%.asm,$(BUILD_DIR)/%.o,$(ASM_SRCS))

TARGET     := $(BUILD_DIR)/AETHERION_OS.elf

# ----------------- Rules -----------------
all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile ASM -> OBJ
$(BUILD_DIR)/%.o: boot/%.asm
	$(NASM) -f elf64 -o $@ $< -Ikernel

# Compile C -> OBJ
$(BUILD_DIR)/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link ELF64
$(TARGET): $(ASM_OBJS) $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR)/*

.PHONY: all clean
