#ifndef WAL_H
#define WAL_H

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
#define fsync _commit
#else
#include <unistd.h>
#endif
#include <stdint.h>
#include <stdbool.h>
#include "pager.h"

typedef struct WAL WAL;

// WAL Frame Header
typedef struct {
    uint32_t page_num;
    uint32_t checksum; // Simple checksum for now
} WalFrameHeader;

struct WAL {
    int fd;
    char filename[256];
};

/**
 * Open the WAL for a given database file.
 * The WAL file will be named "<db_filename>.wal".
 */
WAL* wal_open(const char* db_filename);

/**
 * Close the WAL.
 */
void wal_close(WAL* wal);

/**
 * Log a page modification to the WAL.
 * @param wal WAL instance
 * @param page_num Page number being modified
 * @param data Pointer to the new page data (PAGE_SIZE bytes)
 * @return 0 on success, -1 on error
 */
int wal_log_page(WAL* wal, uint32_t page_num, void* data);

/**
 * Checkpoint the WAL.
 * Moves all frames from WAL to the main database file.
 * Truncates the WAL after successful checkpoint.
 * @param wal WAL instance
 * @param pager Pager instance (used to write to DB file)
 * @return 0 on success, -1 on error
 */
int wal_checkpoint(WAL* wal, Pager* pager);

#endif // WAL_H
