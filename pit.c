#include "pit.h"
#include "io.h"

// The base oscillator frequency of the 8253/8254 PIT chip (in Hz)
#define PIT_BASE_FREQ   1193182

// The PIT I/O ports
#define PIT_CHANNEL0    0x40    // Channel 0 data port (connected to IRQ0)
#define PIT_COMMAND     0x43    // Mode/command register

// Internal tick counter - volatile so the compiler never caches it in a register
static volatile uint32_t ticks = 0;
static uint32_t ticks_per_second = 0;

void pit_tick() {
    ticks++;
}

void pit_init(uint32_t frequency_hz) {
    ticks_per_second = frequency_hz;

    // Calculate the divisor: how many base oscillations per tick
    uint32_t divisor = PIT_BASE_FREQ / frequency_hz;

    // Command: Channel 0, lobyte/hibyte mode, square wave mode 3, binary
    outb(PIT_COMMAND, 0x36);

    // Send divisor low byte first, then high byte
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}

uint32_t pit_get_ticks(void) {
    return ticks;
}

uint32_t pit_get_seconds(void) {
    if (ticks_per_second == 0) return 0;
    return ticks / ticks_per_second;
}

void pit_sleep_ms(uint32_t ms) {
    // How many ticks do we need to wait?
    uint32_t target = ticks + (ticks_per_second * ms / 1000);
    // Busy-wait until we've hit enough ticks
    while (ticks < target) {
        asm volatile("hlt"); // Halt until next interrupt (saves power vs spinning)
    }
}
