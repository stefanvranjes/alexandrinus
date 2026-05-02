#include "idt.h"

struct idt_entry_struct idt[256];
struct idt_ptr_struct idtp;

// Explicitly tell C to use the exact symbol 'idt_load' without any underscores
extern void idt_load(uint32_t ptr) asm("idt_load");

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_init() {
    idtp.limit = (sizeof(struct idt_entry_struct) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    idt_load((uint32_t)&idtp);
}
