#include "exceptions.h"
#include "terminal.h"
#include "idt.h"

static const char* exception_messages[] = {
    "Division Error",
    "Debug Exception",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available (No Math Coprocessor)",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
};

static void print_hex(uint32_t n) {
    terminal_writestring("0x");
    char buf[9]; buf[8] = '\0';
    for (int i = 7; i >= 0; i--) {
        int nibble = n & 0xF;
        buf[i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
        n >>= 4;
    }
    terminal_writestring(buf);
}

// Called from the isr_common_stub in interrupt.asm
void exception_handler_c(uint32_t num, uint32_t err_code) asm("exception_handler_c");
void exception_handler_c(uint32_t num, uint32_t err_code) {
    // Red banner
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    terminal_writestring("\n");
    terminal_writestring("  *** KERNEL PANIC - SYSTEM HALTED ***                                          ");
    terminal_writestring("                                                                                ");

    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
    terminal_writestring("\n  EXCEPTION #");
    if (num < 10) terminal_putchar('0' + num);
    else { terminal_putchar('0' + num / 10); terminal_putchar('0' + num % 10); }
    terminal_writestring(": ");
    if (num < 22) terminal_writestring(exception_messages[num]);
    else          terminal_writestring("Unknown Exception");

    if (num == 8 || (num >= 10 && num <= 14) || num == 17 || num == 21) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        terminal_writestring("\n  Error Code:   ");
        print_hex(err_code);
    }

    terminal_setcolor(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
    terminal_writestring("\n\n  The system has been halted. Please restart your computer.");

    asm volatile("cli");
    while(1) { asm volatile("hlt"); }
}

// Register all 22 CPU exception handlers individually — same pattern as
// keyboard/timer which we know resolves correctly through the PE linker.
void exceptions_init(void) {
    idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, 0x8E);
    idt_set_gate(2,  (uint32_t)isr2,  0x08, 0x8E);
    idt_set_gate(3,  (uint32_t)isr3,  0x08, 0x8E);
    idt_set_gate(4,  (uint32_t)isr4,  0x08, 0x8E);
    idt_set_gate(5,  (uint32_t)isr5,  0x08, 0x8E);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, 0x8E);
    idt_set_gate(7,  (uint32_t)isr7,  0x08, 0x8E);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, 0x8E);
    idt_set_gate(9,  (uint32_t)isr9,  0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
}
