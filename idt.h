#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// A struct describing an Interrupt Gate
struct idt_entry_struct {
    uint16_t base_low;             // The lower 16 bits of the ISR's address
    uint16_t sel;                  // The kernel segment selector
    uint8_t  always0;              // This must always be zero
    uint8_t  flags;                // Set flags (e.g., Present, Ring 0, 32-bit gate)
    uint16_t base_high;            // The upper 16 bits of the ISR's address
} __attribute__((packed));

// A struct describing a pointer to an array of interrupt handlers
struct idt_ptr_struct {
    uint16_t limit;
    uint32_t base;                 // The address of the first element in our idt_entry_t array
} __attribute__((packed));

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void idt_init();

#endif
