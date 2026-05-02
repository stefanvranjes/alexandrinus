#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdint.h>

// Register all 22 CPU exception handlers in the IDT
void exceptions_init(void);

// The common C handler called by the ASM stubs
void exception_handler_c(uint32_t num, uint32_t err_code);

// Forward-declare all 22 ISR entry points from interrupt.asm
// The asm() attribute tells the C compiler to link to the bare symbol name
// without prepending the usual underscore.
extern void isr0()  asm("isr0");  extern void isr1()  asm("isr1");
extern void isr2()  asm("isr2");  extern void isr3()  asm("isr3");
extern void isr4()  asm("isr4");  extern void isr5()  asm("isr5");
extern void isr6()  asm("isr6");  extern void isr7()  asm("isr7");
extern void isr8()  asm("isr8");  extern void isr9()  asm("isr9");
extern void isr10() asm("isr10"); extern void isr11() asm("isr11");
extern void isr12() asm("isr12"); extern void isr13() asm("isr13");
extern void isr14() asm("isr14"); extern void isr15() asm("isr15");
extern void isr16() asm("isr16"); extern void isr17() asm("isr17");
extern void isr18() asm("isr18"); extern void isr19() asm("isr19");
extern void isr20() asm("isr20"); extern void isr21() asm("isr21");

#endif
