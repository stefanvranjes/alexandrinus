#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE       4096            // 4 KB per page
#define TOTAL_PAGES     4096            // Manage 16 MB (4096 pages × 4 KB)

// Initialize the PMM: mark reserved regions and free available RAM
void pmm_init(void);

// Allocate one 4 KB page. Returns physical address, or 0 if out of memory.
uint32_t pmm_alloc_page(void);

// Free a previously allocated page by its physical address.
void pmm_free_page(uint32_t addr);

// Returns how many pages are currently free.
uint32_t pmm_free_pages(void);

// Returns total pages managed.
uint32_t pmm_total_pages(void);

// -------------------------------------------------------
// Simple linear heap allocator built on top of the PMM.
// Suitable for small kernel allocations (no fragmentation).
// -------------------------------------------------------
void* kmalloc(size_t size);
void  kfree(void* ptr);     // No-op for linear allocator (future: proper free)

#endif
