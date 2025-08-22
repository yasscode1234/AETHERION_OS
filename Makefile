all:
    x86_64-elf-gcc -ffreestanding -O2 -c kernel.c -o kernel.o
    nasm -f bin bootloader.asm -o bootloader.o
    x86_64-elf-ld -T link.ld -o OrionOS.bin bootloader.o kernel.o
