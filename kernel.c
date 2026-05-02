#include "terminal.h"
#include "io.h"
#include "idt.h"
#include "pit.h"
#include "exceptions.h"
#include "pmm.h"

// US QWERTY Keyboard Layout mapping (lowercase/unshifted)
const char kbd_US[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', 
  '9', '0', '-', '=', '\b', 
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, 
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, 
  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Shifted layout: uppercase letters and shifted symbols
const char kbd_US_shifted[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*',
  '(', ')', '_', '+', '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0,
  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Shift state flag — 1 when either Shift key is held
static int shift_pressed = 0;

extern void keyboard_handler_isr() asm("keyboard_handler_isr"); 

void pic_remap() {
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28); 
    outb(0x21, 0x04); outb(0xA1, 0x02); 
    outb(0x21, 0x01); outb(0xA1, 0x01); 
    // Unmask IRQ0 (Timer) and IRQ1 (Keyboard) — 0xFC = 1111 1100
    outb(0x21, 0xFC); outb(0xA1, 0xFF); 
}

// --- CLI Logic ---

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int starts_with(const char* str, const char* prefix) {
    while (*prefix) { if (*prefix++ != *str++) return 0; }
    return 1;
}

char keyboard_buffer[256];
int buffer_length = 0;
int cursor_pos = 0;

// Print a uint32 integer to the terminal
void print_uint32(uint32_t n) {
    if (n == 0) { terminal_putchar('0'); return; }
    char buf[12]; int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    for (int j = i - 1; j >= 0; j--) terminal_putchar(buf[j]);
}

// Print a uint32 as hex (0xABCDEF)
void print_hex32(uint32_t n) {
    terminal_writestring("0x");
    for (int i = 28; i >= 0; i -= 4) {
        int nibble = (n >> i) & 0xF;
        terminal_putchar(nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
    }
}

// Print value in KB
void print_kb(uint32_t bytes) {
    print_uint32(bytes / 1024);
    terminal_writestring(" KB");
}

void execute_command(char* cmd) {
    if (buffer_length == 0) return; // Ignore empty presses of Enter
    cmd[buffer_length] = '\0'; // Null-terminate the string

    if (strcmp(cmd, "help") == 0) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        terminal_writestring("Available commands:\n");
        terminal_writestring("  help    - Shows this menu\n");
        terminal_writestring("  clear   - Clears the terminal screen\n");
        terminal_writestring("  ping    - Pong!\n");
        terminal_writestring("  echo    - Prints text after it\n");
        terminal_writestring("  uptime  - Shows seconds since boot\n");
        terminal_writestring("  mem     - Shows memory usage\n");
        terminal_writestring("  kmalloc - Demo dynamic allocation\n");
        terminal_writestring("  div0    - Trigger a CPU Exception\n");
    } 
    else if (strcmp(cmd, "clear") == 0) {
        terminal_initialize();
    } 
    else if (strcmp(cmd, "ping") == 0) {
        terminal_writestring("pong!\n");
    }
    else if (strcmp(cmd, "uptime") == 0) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
        terminal_writestring("Uptime: ");
        print_uint32(pit_get_seconds());
        terminal_writestring(" seconds (ticks: ");
        print_uint32(pit_get_ticks());
        terminal_writestring(")\n");
    }
    else if (strcmp(cmd, "mem") == 0) {
        uint32_t free_p  = pmm_free_pages();
        uint32_t total_p = pmm_total_pages();
        uint32_t used_p  = total_p - free_p;
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        terminal_writestring("Physical Memory Map:\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        terminal_writestring("  Free  : ");
        print_uint32(free_p);  terminal_writestring(" pages = "); print_kb(free_p * PAGE_SIZE);  terminal_writestring("\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        terminal_writestring("  Used  : ");
        print_uint32(used_p);  terminal_writestring(" pages = "); print_kb(used_p * PAGE_SIZE);  terminal_writestring("\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        terminal_writestring("  Total : ");
        print_uint32(total_p); terminal_writestring(" pages = "); print_kb(total_p * PAGE_SIZE); terminal_writestring("\n");
    }
    else if (strcmp(cmd, "kmalloc") == 0) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
        terminal_writestring("Allocating 3 test pages via pmm_alloc_page():\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        uint32_t p1 = pmm_alloc_page();
        uint32_t p2 = pmm_alloc_page();
        uint32_t p3 = pmm_alloc_page();
        terminal_writestring("  Page 1 @ "); print_hex32(p1); terminal_writestring("\n");
        terminal_writestring("  Page 2 @ "); print_hex32(p2); terminal_writestring("\n");
        terminal_writestring("  Page 3 @ "); print_hex32(p3); terminal_writestring("\n");
        pmm_free_page(p1); pmm_free_page(p2); pmm_free_page(p3);
        terminal_writestring("Freed all 3 pages.\n");
        terminal_writestring("kmalloc(64): @ "); print_hex32((uint32_t)kmalloc(64)); terminal_writestring("\n");
        terminal_writestring("kmalloc(128): @ "); print_hex32((uint32_t)kmalloc(128)); terminal_writestring("\n");
    }
    else if (strcmp(cmd, "div0") == 0) {
        // Deliberately trigger a Division-by-Zero exception (CPU Exception #0)
        // to test our Kernel Panic screen!
        terminal_writestring("Triggering Division by Zero...\n");
        volatile int x = 0;
        volatile int y = 1 / x; // This will fire Exception #0!
        (void)y;
    }
    else if (starts_with(cmd, "echo ")) {
        terminal_writestring(cmd + 5);
        terminal_putchar('\n');
    }
    else {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        terminal_writestring("Unknown command: ");
        terminal_writestring(cmd);
        terminal_putchar('\n');
    }
}

// Scancodes for Left and Right Shift
#define LSHIFT_PRESS   0x2A
#define RSHIFT_PRESS   0x36
#define LSHIFT_RELEASE 0xAA
#define RSHIFT_RELEASE 0xB6

// --- Keyboard Handler with Arrow Navigation + Shift Support ---

void keyboard_handler_c() asm("keyboard_handler_c");
void keyboard_handler_c() {
    uint8_t scancode = inb(0x60);
    outb(0x20, 0x20); // Send EOI

    // Track Shift key presses and releases BEFORE the early-return
    if (scancode == LSHIFT_PRESS || scancode == RSHIFT_PRESS) {
        shift_pressed = 1;
        return;
    }
    if (scancode == LSHIFT_RELEASE || scancode == RSHIFT_RELEASE) {
        shift_pressed = 0;
        return;
    }

    if (scancode > 128) return; // Ignore other key releases
    
    // Left Arrow
    if (scancode == 0x4B) {
        if (cursor_pos > 0) {
            cursor_pos--;
            size_t cur_col = terminal_get_column();
            size_t cur_row = terminal_get_row();
            if (cur_col == 0) {
                cur_col = 79;
                cur_row--;
            } else {
                cur_col--;
            }
            terminal_set_cursor(cur_col, cur_row);
        }
        return;
    }
    
    // Right Arrow
    if (scancode == 0x4D) {
        if (cursor_pos < buffer_length) {
            cursor_pos++;
            size_t cur_col = terminal_get_column();
            size_t cur_row = terminal_get_row();
            if (cur_col == 79) {
                cur_col = 0;
                cur_row++;
            } else {
                cur_col++;
            }
            terminal_set_cursor(cur_col, cur_row);
        }
        return;
    }

    // Pick the correct table based on whether Shift is held
    char ascii = shift_pressed ? kbd_US_shifted[scancode] : kbd_US[scancode];
    
    if (ascii == '\n') {
        // Move hardware cursor to end of string before printing newline
        if (cursor_pos < buffer_length) {
            int offset = buffer_length - cursor_pos;
            size_t end_col = terminal_get_column();
            size_t end_row = terminal_get_row();
            for(int i=0; i<offset; i++){
                end_col++;
                if(end_col >= 80) { end_col = 0; end_row++; }
            }
            terminal_set_cursor(end_col, end_row);
        }
        
        terminal_putchar('\n');
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        execute_command(keyboard_buffer);
        
        buffer_length = 0;
        cursor_pos = 0;
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        terminal_writestring("> ");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    } 
    else if (ascii == '\b') {
        if (cursor_pos > 0) {
            // Shift buffer left
            for (int i = cursor_pos - 1; i < buffer_length - 1; i++) {
                keyboard_buffer[i] = keyboard_buffer[i + 1];
            }
            buffer_length--;
            cursor_pos--;

            // Move hardware cursor back 1
            size_t cur_col = terminal_get_column();
            size_t cur_row = terminal_get_row();
            if (cur_col == 0) {
                cur_col = 79;
                cur_row--;
            } else {
                cur_col--;
            }
            terminal_set_cursor(cur_col, cur_row);

            // Redraw rest of line
            size_t old_col = cur_col;
            size_t old_row = cur_row;
            for (int i = cursor_pos; i < buffer_length; i++) {
                terminal_putchar(keyboard_buffer[i]);
            }
            terminal_putchar(' '); // Erase the last trailing char

            // Restore cursor
            terminal_set_cursor(old_col, old_row);
        }
    } 
    else if (ascii != 0) {
        if (buffer_length < 255) {
            // Shift buffer right to make room
            for (int i = buffer_length; i > cursor_pos; i--) {
                keyboard_buffer[i] = keyboard_buffer[i - 1];
            }
            keyboard_buffer[cursor_pos] = ascii;
            buffer_length++;
            cursor_pos++;
            
            // Redraw the rest of the line
            size_t old_col = terminal_get_column();
            size_t old_row = terminal_get_row();
            
            for (int i = cursor_pos - 1; i < buffer_length; i++) {
                terminal_putchar(keyboard_buffer[i]);
            }
            
            // Restore cursor position to exactly after the inserted char
            old_col++;
            if (old_col >= 80) {
                old_col = 0;
                old_row++;
            }
            terminal_set_cursor(old_col, old_row);
        }
    }
}

void kmain() asm("_kmain");
void kmain() {
    terminal_initialize();

    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK));
    terminal_writestring("Welcome to Alexandrinus OS!\n");
    terminal_writestring("Type 'help' for available commands.\n\n");
    
    idt_init();
    exceptions_init();
    pic_remap();

    // IRQ0 = Timer,  IRQ1 = Keyboard
    extern void timer_handler_isr() asm("timer_handler_isr");
    idt_set_gate(32, (uint32_t)timer_handler_isr, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)keyboard_handler_isr, 0x08, 0x8E);
    pit_init(100);

    // Init Physical Memory Manager
    pmm_init();

    asm volatile("sti");

    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("> ");
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));

    while(1) { asm volatile("hlt"); }
}
