#include "pager.h"
#include "wal.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#ifndef open
#define open _open
#endif
#ifndef close
#define close _close
#endif
#ifndef lseek
#define lseek _lseek
#endif
#ifndef read
#define read _read
#endif
#ifndef write
#define write _write
#endif
#ifndef O_RDWR
#define O_RDWR _O_RDWR
#endif
#ifndef O_CREAT
#define O_CREAT _O_CREAT
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#else
#include <unistd.h>
#endif

Pager* pager_open(const char* filename) {
    int fd = open(filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd == -1) {
        fprintf(stderr, "Unable to open file\n");
        return NULL;
    }

    off_t file_length = lseek(fd, 0, SEEK_END);
    if (file_length == -1) {
        fprintf(stderr, "Error seeking file: %d\n", errno);
        close(fd);
        return NULL;
    }
    
    Pager* pager = malloc(sizeof(Pager));
    if (!pager) {
        fprintf(stderr, "Failed to allocate memory for pager\n");
        close(fd);
        return NULL;
    }
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    pager->num_pages = (file_length / PAGE_SIZE);

    if (file_length % PAGE_SIZE != 0) {
        fprintf(stderr, "Db file is not a whole number of pages. Corrupt file.\n");
        free(pager);
        close(fd);
        return NULL;
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }
    pager->wal = NULL;
    pager->wal = NULL;

    return pager;
}

    if (page_num >= TABLE_MAX_PAGES) {
        fprintf(stderr, "Tried to fetch page number out of bounds. %d >= %d\n", page_num, TABLE_MAX_PAGES);
        fprintf(stderr, "Tried to fetch page number out of bounds. %d > %d\n", page_num, TABLE_MAX_PAGES);
        return NULL; // Or handle error appropriately
    }

    if (pager->pages[page_num] == NULL) {
        // Cache miss. Allocate memory and load from file.
        void* page = calloc(1, PAGE_SIZE);
        if (!page) {
            fprintf(stderr, "Failed to allocate memory for page\n");
            return NULL;
        }
        uint32_t num_pages = pager->file_length / PAGE_SIZE;

        // We might save a partial page at the end of the file
        if (pager->file_length % PAGE_SIZE) {
            num_pages += 1;
        }

        if (page_num <= num_pages) {
            lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
            if (bytes_read == -1) {
                fprintf(stderr, "Error reading file: %d\n", errno);
                free(page);
                return NULL;
            }
        }

        pager->pages[page_num] = page;
        
        if (page_num >= pager->num_pages) {
             pager->num_pages = page_num + 1;
        }
    }

    return pager->pages[page_num];
}
void pager_set_wal(Pager* pager, WAL* wal) {
    pager->wal = wal;
}

void pager_flush(Pager* pager, uint32_t page_num) {
    if (pager->pages[page_num] == NULL) {
        fprintf(stderr, "Tried to flush null page\n");
        return;
    }

    if (pager->wal) {
        // Write to WAL
        wal_log_page(pager->wal, page_num, pager->pages[page_num]);
    } else {
        // Write directly to DB file
        off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
        if (offset == -1) {
            fprintf(stderr, "Error seeking: %d\n", errno);
            return;
        }

        ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);
        if (bytes_written == -1) {
            fprintf(stderr, "Error writing: %d\n", errno);
            return;
        }
    }
}

void pager_close(Pager* pager) {
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        if (pager->pages[i]) {
            pager_flush(pager, i);
            free(pager->pages[i]);
            pager->pages[i] = NULL;
        }
    }
    
    int result = close(pager->file_descriptor);
    if (result == -1) {
        fprintf(stderr, "Error closing db file.\n");
    }
    
    free(pager);
}
