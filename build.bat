@echo off
echo Compiling Bootloader...
"C:\Users\Stefan\AppData\Local\bin\NASM\nasm.exe" -f bin boot.asm -o boot.bin
if %errorlevel% neq 0 ( pause & exit /b %errorlevel% )

echo Compiling Assembly Objects...
"C:\Users\Stefan\AppData\Local\bin\NASM\nasm.exe" -f win32 kernel_entry.asm -o kernel_entry.o
if %errorlevel% neq 0 ( pause & exit /b %errorlevel% )
"C:\Users\Stefan\AppData\Local\bin\NASM\nasm.exe" -f win32 interrupt.asm -o interrupt.o
if %errorlevel% neq 0 ( pause & exit /b %errorlevel% )

echo Compiling C Source Files...
C:\msys64\ucrt64\bin\gcc.exe -m32 -ffreestanding -nostdlib -c kernel.c terminal.c idt.c
if %errorlevel% neq 0 ( pause & exit /b %errorlevel% )

echo Linking...
C:\msys64\ucrt64\bin\ld.exe -m i386pe -T linker.ld kernel_entry.o interrupt.o kernel.o terminal.o idt.o -o kernel.pe
if %errorlevel% neq 0 ( pause & exit /b %errorlevel% )

C:\msys64\ucrt64\bin\objcopy.exe -O binary kernel.pe kernel.bin

echo Building OS Image...
copy /b boot.bin + kernel.bin os-image.bin > nul

echo Booting OS in QEMU...
"C:\Program Files\qemu\qemu-system-x86_64.exe" -drive format=raw,file=os-image.bin
