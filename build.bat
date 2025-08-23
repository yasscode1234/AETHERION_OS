@echo off
setlocal

REM ---- CONFIGURATION ----
set NASM=C:\mingw64\bin\nasm.exe
set GCC=C:\mingw64\bin\gcc.exe
set LD=C:\mingw64\bin\ld.exe
set OBJCOPY=C:\mingw64\bin\objcopy.exe

set BUILD_DIR=build
set SRC_DIR=%CD%
set BIN=%BUILD_DIR%\aetherion.bin
set ELF=%BUILD_DIR%\aetherion.elf

REM ---- CREER LE DOSSIER BUILD ----
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM ---- ASSEMBLER LE BOOTLOADER ----
echo Assembling bootloader...
%NASM% -f elf64 boot/bootloader.s -o %BUILD_DIR%\bootloader.o
if errorlevel 1 (
    echo ERREUR: assemblage bootloader échoué
    exit /b 1
)

REM ---- COMPILER LES SOURCES C ----
echo Compiling C sources...
for %%f in (kernel\kernel.c kernel\ia.c kernel\vga.c kernel\keyboard.c kernel\system.c kernel\string.c) do (
    %GCC% -ffreestanding -c %%f -o %BUILD_DIR%\%%~nf.o
    if errorlevel 1 (
        echo ERREUR: compilation de %%f échouée
        exit /b 1
    )
)

REM ---- LINKER L'OS ----
echo Linking...
%LD% -T boot/link.ld -o %ELF% %BUILD_DIR%\bootloader.o %BUILD_DIR%\kernel.o %BUILD_DIR%\ia.o %BUILD_DIR%\vga.o %BUILD_DIR%\keyboard.o %BUILD_DIR%\system.o %BUILD_DIR%\string.o
if errorlevel 1 (
    echo ERREUR: linkage échoué
    exit /b 1
)

REM ---- CREER LE BINAIRE BOOTABLE ----
echo Creating bootable binary...
%OBJCOPY% -O binary %ELF% %BIN%
if errorlevel 1 (
    echo ERREUR: création du binaire échoué
    exit /b 1
)

echo ------------------------------------------
echo Compilation terminée avec succès !
echo Binaire bootable: %BIN%
echo ------------------------------------------
endlocal
pause
