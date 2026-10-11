@echo off
setlocal

set "TOOLCHAIN_DIR=E:\Code"
set "PATH=%TOOLCHAIN_DIR%\bin;%TOOLCHAIN_DIR%\libexec\gcc\x86_64-elf\15.2.0;%PATH%"

if not exist "%TOOLCHAIN_DIR%\bin\x86_64-elf-gcc.exe" (
    echo [ERROR] Cross-compiler not found: %TOOLCHAIN_DIR%\bin\x86_64-elf-gcc.exe
    exit /b 1
)
if not exist "%TOOLCHAIN_DIR%\bin\x86_64-elf-ld.exe" (
    echo [ERROR] Cross-linker not found: %TOOLCHAIN_DIR%\bin\x86_64-elf-ld.exe
    exit /b 1
)
if not exist "%TOOLCHAIN_DIR%\bin\x86_64-elf-objcopy.exe" (
    echo [ERROR] objcopy not found: %TOOLCHAIN_DIR%\bin\x86_64-elf-objcopy.exe
    exit /b 1
)
where nasm >nul 2>&1
if errorlevel 1 (
    echo [ERROR] NASM is not on PATH.
    exit /b 1
)
where mingw32-make >nul 2>&1
if errorlevel 1 (
    echo [ERROR] mingw32-make is not on PATH.
    exit /b 1
)

if not exist build mkdir build
echo [INFO] Building Aetherion kernel and BIOS disk image...
mingw32-make build
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

if not exist "build\aetherion.elf" (
    echo [ERROR] Build reported success but build\aetherion.elf is missing.
    exit /b 1
)
if not exist "build\kernel.bin" (
    echo [ERROR] Build reported success but build\kernel.bin is missing.
    exit /b 1
)
if not exist "build\aetherion.img" (
    echo [ERROR] Build reported success but build\aetherion.img is missing.
    exit /b 1
)

echo [SUCCESS] Build artifacts:
powershell -NoProfile -Command "Get-Item 'build\aetherion.elf','build\kernel.bin','build\aetherion.img' | Select-Object Name,Length | Format-Table -AutoSize"
exit /b 0
