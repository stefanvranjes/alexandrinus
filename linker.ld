ENTRY(_kmain)
SECTIONS
{
    /* Tell the linker that our kernel starts at memory address 0x1000 */
    . = 0x1000;
    
    .text : { 
        /* Ensure that the kernel entry point is exactly the first thing in the binary */
        kernel_entry.o(.text)
        *(.text) 
    }
    
    .data : { *(.data) }
    .rodata : { *(.rodata) }
    .bss : { *(.bss) }
}
