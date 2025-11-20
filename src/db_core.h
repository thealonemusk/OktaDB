#ifndef DB_CORE_H
#define DB_CORE_H
#include <stddef.h>
#include <stdbool.h>
#include "hashtable.h" // Include hashtable operations
#include "utility.h" // Include constants like MAX_KEY_LEN

// Internal record structure
typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
    bool deleted;  // true if deleted, false if active
} Record;

// Database structure
typedef struct Database {
    char filename[MAX_KEY_LEN];
    Record *records;
    size_t count;       // Number of records (including deleted)
    size_t capacity;    // Maximum capacity
    bool modified;      // true if database has unsaved changes
    size_t tombstone_count;  // Number of deleted records
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

#endif // DB_CORE_H