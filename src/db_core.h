#ifndef DB_CORE_H
#define DB_CORE_H

#include "pager.h"
#include "btree.h"
#include "wal.h"
#include "utility.h" // For MAX_FILENAME_LEN, MAX_KEY_LEN, MAX_VALUE_LEN, STATUS_* constants

// Database structure
// WARNING: This implementation uses a global static instance (db_instance in db_core.c)
// which limits the application to a single database and is NOT thread-safe.
// For multi-database or multi-threaded use, this design would need to be refactored.
typedef struct Database {
    char filename[MAX_FILENAME_LEN];
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
 * Insert a new key-value pair
 * @param db Database instance
 * @param key Key string (max 127 chars)
 * @param value Value string (max 255 chars)
 * @return STATUS_OK on success, STATUS_ERROR on failure, STATUS_EXISTS if key already exists
 */
int db_insert(Database *db, const char *key, const char *value);

/**
 * Get value by key
 * @param db Database instance
 * @param key Key to search for
 * @return Direct pointer to value string in database record, or NULL if not found
 */
const char* db_get(Database *db, const char *key);

/**
 * Delete a key-value pair
 * @param db Database instance
 * @param key Key to delete
 * @return STATUS_OK on success, STATUS_NOT_FOUND if key doesn't exist, STATUS_ERROR on failure
 */
int db_delete(Database *db, const char *key);

/**
 * Update a key's value
 * @param db Database instance
 * @param key Key to update
 * @param value New value
 * @return STATUS_OK on success, STATUS_NOT_FOUND if key not found, STATUS_ERROR on error
 */
int db_update(Database *db, const char *key, const char *value);

/**
 * Iterate over all records and invoke a callback for each.
 * @param db Database instance
 * @param callback Function to call for each key/value pair
 * @param ctx User-provided context passed to callback
 * @return STATUS_OK on success, STATUS_ERROR on failure
 */
int db_select_all(Database *db, void (*callback)(const char *key, const char *value, void *ctx), void *ctx);

/**
 * Find a record by key and invoke a callback.
 * @param db Database instance
 * @param key Key to search for
 * @param callback Function to call with the found key/value
 * @param ctx User-provided context passed to callback
 * @return STATUS_OK if found, STATUS_NOT_FOUND if not, STATUS_ERROR on error
 */
int db_select_where(Database *db, const char *key, void (*callback)(const char *key, const char *value, void *ctx), void *ctx);

/**
 * List all keys (SQL‑like table output)
 * @param db Database instance
 */
void db_list(Database *db);

#endif // DB_CORE_H