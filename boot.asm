[bits 16]           ; Tell assembler to use 16-bit mode
[org 0x7c00]        ; BIOS loads our bootloader at memory address 0x7c00

start:
    ; Set up the segment registers
    xor ax, ax      ; Clear AX (AX=0)
    mov ds, ax      ; Set Data Segment to 0
    mov es, ax      ; Set Extra Segment to 0
    
    ; Print our welcome message
    mov si, message ; Point SI (Source Index) to our message string
    call print_string

    ; Infinite loop to prevent the CPU from executing random memory
hang:
    jmp hang

; Function: print_string
; Prints a null-terminated string pointed to by SI
print_string:
    mov ah, 0x0E    ; BIOS interrupt parameter: Teletype output (print character)
.loop:
    lodsb           ; Load byte at DS:SI into AL and increment SI
    cmp al, 0       ; Check if end of string (null byte)
    je .done        ; If yes, we're done
    int 0x10        ; Call BIOS video interrupt to print the character in AL
    jmp .loop       ; Repeat for next character
.done:
    ret

; Data Section
message db 'Welcome to Alexandrinus OS!', 13, 10, 0  ; 13 (CR) and 10 (LF) for new line, 0 for null terminator

; Bootsector padding and magic number
times 510-($-$$) db 0   ; Pad the remaining bytes up to 510 with zeroes
dw 0xAA55               ; The standard PC boot signature (Magic number) that tells BIOS this is bootable
