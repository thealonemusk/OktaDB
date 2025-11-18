#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>
#include "../common/utility.h"

// Opaque database structure
// The actual implementation is hidden in storage.c
typedef struct Database Database;

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
 * @return Dynamically allocated value string (caller must free), or NULL if not found
 */
const char* db_get(Database *db, const char *key);

/**
 * Delete a key-value pair
 * @param db Database instance
 * @param key Key to delete
 * @return STATUS_OK on success, STATUS_NOT_FOUND if key doesn't exist
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

/**
 * Compact the database by removing deleted records
 * @param db Database instance
 */
void db_compact(Database *db);

#endif // STORAGE_H