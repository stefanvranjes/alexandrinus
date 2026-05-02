#include "terminal.h"
#include "io.h"
#include "idt.h"

// US QWERTY Keyboard Layout mapping
const char kbd_US[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', 
  '9', '0', '-', '=', '\b', 
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, 
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, 
  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

extern void keyboard_handler_isr() asm("keyboard_handler_isr"); 

void pic_remap() {
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28); 
    outb(0x21, 0x04); outb(0xA1, 0x02); 
    outb(0x21, 0x01); outb(0xA1, 0x01); 
    outb(0x21, 0xFD); outb(0xA1, 0xFF); 
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
int cursor_pos = 0; // Tracks the internal cursor position within the buffer

void execute_command(char* cmd) {
    if (buffer_length == 0) return; // Ignore empty presses of Enter
    cmd[buffer_length] = '\0'; // Null-terminate the string

    if (strcmp(cmd, "help") == 0) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        terminal_writestring("Available commands:\n");
        terminal_writestring("  help  - Shows this menu\n");
        terminal_writestring("  clear - Clears the terminal screen\n");
        terminal_writestring("  ping  - Pong!\n");
        terminal_writestring("  echo  - Prints whatever you type after it\n");
    } 
    else if (strcmp(cmd, "clear") == 0) {
        terminal_initialize();
    } 
    else if (strcmp(cmd, "ping") == 0) {
        terminal_writestring("pong!\n");
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

// --- Keyboard Handler with Arrow Navigation ---

void keyboard_handler_c() asm("keyboard_handler_c");
void keyboard_handler_c() {
    uint8_t scancode = inb(0x60);
    outb(0x20, 0x20); // Send EOI

    if (scancode > 128) return; // Ignore key releases
    
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

    char ascii = kbd_US[scancode];
    
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
    terminal_writestring("The CLI is now active with Arrow Key Navigation.\n\n");
    
    idt_init();
    pic_remap();
    idt_set_gate(33, (uint32_t)keyboard_handler_isr, 0x08, 0x8E);
    asm volatile("sti");

    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("> ");
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));

    while(1) {}
}
