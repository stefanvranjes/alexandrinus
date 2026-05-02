[bits 16]           
[org 0x7c00]        

start:
    ; Clear the screen
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    ; Set up stack safely so we can call BIOS interrupts
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7c00
    
    ; 1. Save the boot drive number (BIOS gives this to us in DL)
    mov [BOOT_DRIVE], dl
    
    ; 2. Read the Kernel from the disk!
    mov ah, 0x02    ; BIOS Command: Read Sectors
    mov al, 15      ; Number of sectors to read
    mov ch, 0       ; Cylinder 0
    mov dh, 0       ; Head 0
    mov cl, 2       ; Start reading from Sector 2 (Sector 1 is us!)
    
    mov bx, 0x1000  ; Load the kernel into memory at address 0x1000!
    int 0x13        ; Call BIOS disk interrupt

    ; 3. Switch to 32-bit Protected Mode
    cli 
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1         
    mov cr0, eax
    jmp CODE_SEG:init_pm 


; ==========================================
; 32-BIT PROTECTED MODE
; ==========================================
[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov ebp, 0x90000
    mov esp, ebp

    ; We successfully loaded the kernel into 0x1000, now jump to it!
    jmp CODE_SEG:0x1000

; ==========================================
; DATA & GDT
; ==========================================
BOOT_DRIVE db 0

gdt_start:
    dq 0x0 
gdt_code:
    dw 0xffff       
    dw 0x0          
    db 0x0          
    db 10011010b    
    db 11001111b    
    db 0x0          
gdt_data:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b    
    db 11001111b
    db 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 
    dd gdt_start               

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

times 510-($-$$) db 0
dw 0xAA55
