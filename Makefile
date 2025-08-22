# Toolchain
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
AS = nasm

# Flags
CFLAGS = -ffreestanding -O2 -Wall -Wextra -Ikernel
LDFLAGS = -T boot/link.ld -nostdlib

# Sources et objets
C_SOURCES = kernel/kernel.c kernel/keyboard.c kernel/vga.c kernel/system.c kernel/ia.c
ASM_SOURCES = boot/bootloader.asm
C_OBJECTS = $(C_SOURCES:.c=.o)
ASM_OBJECTS = $(ASM_SOURCES:.asm=.o)

# Nom du kernel
KERNEL = kernel.bin

# Tout compiler
all: $(KERNEL)

# Compiler les fichiers C
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assembler le bootloader
%.o: %.asm
	$(AS) -f elf64 $< -o $@

# Lier le kernel
$(KERNEL): $(C_OBJECTS) $(ASM_OBJECTS)
	$(LD) $(LDFLAGS) -o $(KERNEL) $(ASM_OBJECTS) $(C_OBJECTS)

# Créer une ISO bootable avec GRUB
os.iso: $(KERNEL)
	mkdir -p iso/boot/grub
	cp $(KERNEL) iso/boot/
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'menuentry "AETHERION OS" { multiboot /boot/kernel.bin }' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o os.iso iso

clean:
	rm -f kernel/*.o boot/*.o $(KERNEL)
	rm -rf iso
