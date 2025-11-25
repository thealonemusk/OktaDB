#ifndef PAGER_H
#define PAGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100

typedef struct WAL WAL;

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void* pages[TABLE_MAX_PAGES]; // Simple cache for now
    WAL* wal; // Pointer to WAL instance
} Pager;

/**
 * Open the pager for a given file.
 * Creates the file if it doesn't exist.
 */
Pager* pager_open(const char* filename);

/**
 * Get a page from the pager.
 * If the page is not in cache, it is read from disk.
 */
void* pager_get_page(Pager* pager, uint32_t page_num);

/**
 * Flush a specific page to disk.
 */
void pager_flush(Pager* pager, uint32_t page_num);

/**
 * Set the WAL instance for the pager.
 */
void pager_set_wal(Pager* pager, WAL* wal);

/**
 * Close the pager and flush all pages to disk.
 */
void pager_close(Pager* pager);

#endif // PAGER_H
