#include "pmm.h"

// ============================================================
// BITMAP PHYSICAL MEMORY MANAGER
// ============================================================
// Each bit in the bitmap represents one 4 KB page of RAM.
// Bit = 0 → FREE       Bit = 1 → USED / RESERVED
//
// Physical memory layout we manage (16 MB total):
//
//  0x000000 – 0x000FFF  : Page 0    → RESERVED (Real Mode IVT / BDA)
//  0x001000 – 0x07FFFF  : Pages 1-127 → RESERVED (Kernel lives here)
//  0x080000 – 0x09FFFF  : Pages 128-159 → RESERVED (BIOS data area)
//  0x0A0000 – 0x0FFFFF  : Pages 160-255 → RESERVED (VGA + BIOS ROM)
//  0x100000 – 0xFFFFFF  : Pages 256-4095 → FREE (Extended RAM, 15 MB)
// ============================================================

// 4096 pages → 4096 bits → 128 uint32_t entries
static uint32_t bitmap[TOTAL_PAGES / 32];

// Linear heap: allocate from 2MB upward (well above our kernel)
static uint32_t heap_ptr = 0x200000;

// Mark a page as used (set bit)
static void bitmap_set(uint32_t page) {
    bitmap[page / 32] |= (1u << (page % 32));
}

// Mark a page as free (clear bit)
static void bitmap_clear(uint32_t page) {
    bitmap[page / 32] &= ~(1u << (page % 32));
}

// Test if a page is used
static int bitmap_test(uint32_t page) {
    return (bitmap[page / 32] >> (page % 32)) & 1;
}

void pmm_init(void) {
    // 1. Mark ALL pages as used by default (safe starting point)
    for (int i = 0; i < TOTAL_PAGES / 32; i++) {
        bitmap[i] = 0xFFFFFFFF;
    }

    // 2. Free only the extended RAM (1 MB and above) — pages 256 to 4095
    //    This is the conventional "safe" free region in a PC.
    for (int i = 256; i < TOTAL_PAGES; i++) {
        bitmap_clear(i);
    }

    // 3. Re-mark the heap region (pages 512-767 = 2MB–3MB) as used
    //    so kmalloc's linear heap doesn't conflict with pmm_alloc_page.
    for (int i = 512; i < 768; i++) {
        bitmap_set(i);
    }
}

uint32_t pmm_alloc_page(void) {
    // Walk the bitmap looking for the first free (0) page
    for (uint32_t i = 0; i < TOTAL_PAGES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);              // Mark as used
            return i * PAGE_SIZE;       // Return physical address
        }
    }
    return 0; // Out of memory
}

void pmm_free_page(uint32_t addr) {
    uint32_t page = addr / PAGE_SIZE;
    if (page < TOTAL_PAGES) {
        bitmap_clear(page);
    }
}

uint32_t pmm_free_pages(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < TOTAL_PAGES; i++) {
        if (!bitmap_test(i)) count++;
    }
    return count;
}

uint32_t pmm_total_pages(void) {
    return TOTAL_PAGES;
}

// ============================================================
// LINEAR HEAP ALLOCATOR (kmalloc / kfree)
// ============================================================
// Allocates sequentially from heap_ptr upward. Fast and simple.
// Suitable for early kernel use where we never need to free.
// A proper slab or buddy allocator can replace this later.
// ============================================================
void* kmalloc(size_t size) {
    if (size == 0) return 0;

    // Align to 4 bytes (keep allocations aligned)
    size = (size + 3) & ~3;

    void* ptr = (void*)heap_ptr;
    heap_ptr += size;
    return ptr;
}

void kfree(void* ptr) {
    // Linear allocator: freeing individual blocks is a no-op.
    // A future proper allocator would handle this.
    (void)ptr;
}
