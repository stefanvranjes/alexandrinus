[bits 32]

section .text

global idt_load
idt_load:
    mov eax, [esp + 4]  
    lidt [eax]          
    ret

global keyboard_handler_isr
extern keyboard_handler_c

keyboard_handler_isr:
    pusha                    
    call keyboard_handler_c 
    popa                     
    iret                     

global timer_handler_isr
extern pit_tick

timer_handler_isr:
    pusha
    call pit_tick
    mov al, 0x20
    out 0x20, al
    popa
    iret

; ==========================================
; CPU EXCEPTION STUBS (Interrupts 0 - 21)
; ==========================================
; The CPU exception_handler_c(uint32_t num, uint32_t err_code) is our C handler.
; Some exceptions push an error code onto the stack automatically (marked ERR).
; For consistency, we manually push 0 for those that don't.
; ==========================================

extern exception_handler_c

; Macro-like pattern: no error code pushed by CPU
%macro ISR_NOERR 1
  global isr%1
  isr%1:
    cli
    push dword 0        ; Push dummy error code
    push dword %1       ; Push interrupt number
    jmp isr_common_stub
%endmacro

; Macro-like pattern: CPU pushes error code automatically
%macro ISR_ERR 1
  global isr%1
  isr%1:
    cli
    push dword %1       ; Push interrupt number (error code already on stack)
    jmp isr_common_stub
%endmacro

; --- Define all 32 CPU exceptions ---
ISR_NOERR 0   ; Division Error
ISR_NOERR 1   ; Debug
ISR_NOERR 2   ; Non-Maskable Interrupt
ISR_NOERR 3   ; Breakpoint
ISR_NOERR 4   ; Overflow
ISR_NOERR 5   ; Bound Range Exceeded
ISR_NOERR 6   ; Invalid Opcode
ISR_NOERR 7   ; Device Not Available
ISR_ERR   8   ; Double Fault          (error code pushed by CPU)
ISR_NOERR 9   ; Coprocessor Overrun
ISR_ERR   10  ; Invalid TSS           (error code pushed by CPU)
ISR_ERR   11  ; Segment Not Present   (error code pushed by CPU)
ISR_ERR   12  ; Stack-Segment Fault   (error code pushed by CPU)
ISR_ERR   13  ; General Protection    (error code pushed by CPU)
ISR_ERR   14  ; Page Fault            (error code pushed by CPU)
ISR_NOERR 15  ; Reserved
ISR_NOERR 16  ; x87 Floating-Point
ISR_ERR   17  ; Alignment Check       (error code pushed by CPU)
ISR_NOERR 18  ; Machine Check
ISR_NOERR 19  ; SIMD Floating-Point
ISR_NOERR 20  ; Virtualization
ISR_ERR   21  ; Control Protection    (error code pushed by CPU)

; --- Common stub: stack has [num, err_code] at entry ---
; Stack layout on entry: 
;   [esp+0] = interrupt number
;   [esp+4] = error code (real or dummy 0)
;   [esp+8] = EIP  (pushed by CPU)
;   [esp+12]= CS   (pushed by CPU)
;   [esp+16]= EFLAGS (pushed by CPU)
isr_common_stub:
    pusha               ; Save all general-purpose registers (32 bytes)

    ; Call C handler: exception_handler_c(num, err_code)
    ; pusha pushed 8 regs (32 bytes), so num is now at esp+32, err at esp+36
    mov eax, [esp + 32]   ; interrupt number
    mov ebx, [esp + 36]   ; error code
    push ebx
    push eax
    call exception_handler_c
    add esp, 8            ; clean up args

    popa
    add esp, 8            ; remove num + err_code we pushed
    iret
