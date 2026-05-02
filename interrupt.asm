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
