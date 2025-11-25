#ifndef WAL_H
#define WAL_H

#include <stdint.h>
#include <stdbool.h>
#include "pager.h"

typedef struct WAL WAL;

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
