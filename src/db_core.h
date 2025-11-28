#ifndef DB_CORE_H
#define DB_CORE_H

#include "pager.h"
#include "btree.h"
#include "wal.h"
#include "utility.h" // For MAX_KEY_LEN

// Database structure
typedef struct Database {
    char filename[MAX_KEY_LEN];
    Pager* pager;
    WAL* wal;
} Database;

// Function declarations

/**
 * Open or create a database file
 * @param filename Path to the database file
 * @return Pointer to Database structure, or NULL on error
 */
Database* db_open(const char *filename);

/**
 * Close the database and save to disk
 * @param db Database to close
 */
void db_close(Database *db);

/**
 * Insert or update a key-value pair
 * @param db Database instance
 * @param key Key string (max 127 chars)
 * @param value Value string (max 255 chars)
 * @return STATUS_OK on success, STATUS_ERROR on failure
 */
int db_insert(Database *db, const char *key, const char *value);

/**
 * Get value by key
 * @param db Database instance
 * @param key Key to search for
 * @return Direct pointer to value string in database record, or NULL if not found
 * @note The returned pointer is valid until the database is closed or the record is deleted/updated
 */
const char* db_get(Database *db, const char *key);

/**
 * Delete a key-value pair
 * @param db Database instance
 * @param key Key to delete
 * @return STATUS_OK on success, STATUS_NOT_FOUND if key doesn't exist,
 *         STATUS_NOT_IMPLEMENTED (stub: B-tree delete not yet implemented)
 * @note This function is currently a stub - B-tree deletion is not yet implemented
 */
int db_delete(Database *db, const char *key);

/**
 * List all keys in the database
 * @param db Database instance
 */
void db_list(Database *db);

/**
 * Update the value for an existing key
 * @param db Database instance
 */
int db_update(Database *db, const char *key, const char *value);

#endif // DB_CORE_H