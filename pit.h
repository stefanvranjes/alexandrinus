#ifndef PIT_H
#define PIT_H

#include <stdint.h>

// Call this once during kernel init to configure the PIT
void pit_init(uint32_t frequency_hz);

// Returns the total number of ticks since boot
uint32_t pit_get_ticks(void);

// Returns elapsed seconds since boot (integer)
uint32_t pit_get_seconds(void);

// Stall the CPU for the given number of milliseconds
void pit_sleep_ms(uint32_t ms);

// Called from interrupt.asm on IRQ0
void pit_tick() asm("pit_tick");

#endif
