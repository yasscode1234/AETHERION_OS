# Makefile pour AETHERION OS
# Copyright (C) 2025 yasscode
# Licence GPLv2

# Chemin vers le cross-compiler x86_64-elf
CC=x86_64-elf-gcc
LD=x86_64-elf-ld
NASM=nasm
CFLAGS=-ffreestanding -O2 -Wall -Wextra -mno-red-zone -m64
LDFLAGS=-nostdlib -T boot/link.ld
OBJDIR=obj
BINDIR=bin

# Fichiers sources
KERNEL_SRC=$(wildcard kernel/*.c)
KERNEL_OBJ=$(KERNEL_SRC:kernel/%.c=$(OBJDIR)/%.o)

BOOT_SRC=boot/bootloader.asm
BOOT_OBJ=$(OBJDIR)/bootloader.o

# Nom de l'image finale
IMAGE=$(BINDIR)/aetherion_os.bin

# Targets
all: dirs $(IMAGE)

dirs:
	@mkdir -p $(OBJDIR) $(BINDIR)

# Compilation du kernel
$(OBJDIR)/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compilation du bootloader
$(BOOT_OBJ): $(BOOT_SRC)
	$(NASM) -f bin $< -o $@

# Link
$(IMAGE): $(BOOT_OBJ) $(KERNEL_OBJ)
	$(LD) $(LDFLAGS) $(KERNEL_OBJ) -o $(BINDIR)/kernel.bin
	cat $(BOOT_OBJ) $(BINDIR)/kernel.bin > $(IMAGE)

# Nettoyage
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Exécuter sur QEMU
run: $(IMAGE)
	qemu-system-x86_64 -drive format=raw,file=$(IMAGE)

.PHONY: all clean dirs run
