#ifndef IO_H
#define IO_H

#include <stdint.h>

// Send a byte of data to an I/O port
// We use 'inline' and GCC's extended inline assembly syntax
static inline void outb(uint16_t port, uint8_t val) {
    // "a" forces 'val' into the AL register
    // "Nd" forces 'port' into the DX register
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

// Read a byte of data from an I/O port
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

#endif
