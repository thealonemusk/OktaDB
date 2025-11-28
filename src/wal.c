#include "wal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

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
#ifndef O_APPEND
#define O_APPEND _O_APPEND
#endif
#ifndef O_TRUNC
#define O_TRUNC _O_TRUNC
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

// WAL Frame Header
typedef struct {
    uint32_t page_num;
    uint32_t checksum; // Simple checksum for now
} WalFrameHeader;

struct WAL {
    int fd;
    char filename[256];
};

WAL* wal_open(const char* db_filename) {
    WAL* wal = malloc(sizeof(WAL));
    if (!wal) {
        fprintf(stderr, "Failed to allocate memory for WAL\n");
        return NULL;
    }
    int written = snprintf(wal->filename, sizeof(wal->filename), "%s.wal", db_filename);  
    if (written < 0 || written >= (int)sizeof(wal->filename)) {  
        fprintf(stderr, "WAL filename too long\n");  
        free(wal);  
        return NULL;  
    }  
    
    wal->fd = open(wal->filename, O_RDWR | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR);
    if (wal->fd == -1) {
        fprintf(stderr, "Failed to open WAL file: %s\n", wal->filename);
        free(wal);
        return NULL;
    }
    
    return wal;
}

void wal_close(WAL* wal) {
    if (wal) {
        close(wal->fd);
        free(wal);
    }
}

// Simple checksum (sum of bytes)
uint32_t calculate_checksum(void* data, size_t len) {
    uint32_t sum = 0;
    uint8_t* ptr = (uint8_t*)data;
    for (size_t i = 0; i < len; i++) {
        sum += ptr[i];
    }
    return sum;
}

int wal_log_page(WAL* wal, uint32_t page_num, void* data) {
    WalFrameHeader header;
    header.page_num = page_num;
    header.checksum = calculate_checksum(data, PAGE_SIZE);
    
    if (write(wal->fd, &header, sizeof(header)) != sizeof(header)) {
        return -1;
    }
    
    if (write(wal->fd, data, PAGE_SIZE) != PAGE_SIZE) {
        return -1;
    }
    
    // fsync(wal->fd); // Ensure durability (omitted for performance in this simple impl)
    return 0;
}

int wal_checkpoint(WAL* wal, Pager* pager) {
    // Read all frames from WAL and write to DB
    // We should read from start
    lseek(wal->fd, 0, SEEK_SET);
    
    WalFrameHeader header;
    void* buffer = malloc(PAGE_SIZE);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for WAL buffer\n");
        return -1;
    }
    
    while (read(wal->fd, &header, sizeof(header)) == sizeof(header)) {
        ssize_t bytes_read = read(wal->fd, buffer, PAGE_SIZE);  
        if (bytes_read != PAGE_SIZE) {  
            if (bytes_read > 0) {  
                fprintf(stderr, "Incomplete page read in WAL\n");  
            } else if (bytes_read < 0) {  
                perror("Error reading page from WAL");  
            }  
            break;
        }
        
        uint32_t checksum = calculate_checksum(buffer, PAGE_SIZE);
        if (checksum != header.checksum) {
            fprintf(stderr, "Checksum mismatch in WAL frame for page %d\n", header.page_num);
            continue; // Skip corrupted frame? Or abort?
        }
        
        // Write to DB
        // We need to bypass pager cache or update it?
        // Pager writes to disk using pager_flush, but that uses cache.
        // Here we want to write directly to file descriptor of pager.
        // But pager struct exposes file_descriptor.
        
        lseek(pager->file_descriptor, header.page_num * PAGE_SIZE, SEEK_SET);
        ssize_t bytes_written = write(pager->file_descriptor, buffer, PAGE_SIZE);
        if (bytes_written != PAGE_SIZE) {
            fprintf(stderr, "Failed to write page in checkpoint\n");
            free(buffer);
            return -1;
        }  
        
        // Also update pager cache if present
        if (header.page_num < TABLE_MAX_PAGES && pager->pages[header.page_num] != NULL) {
            memcpy(pager->pages[header.page_num], buffer, PAGE_SIZE);
        }
    }
    
    free(buffer);
    
    // Truncate WAL
    close(wal->fd);
    wal->fd = open(wal->filename, O_RDWR | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);  
    if (wal->fd == -1) {  
        fprintf(stderr, "Failed to truncate WAL file\n");  
        return -1;  
    }  
    
    return 0;
}
