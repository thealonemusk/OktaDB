#include "db_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

static Database db_instance;
static Record db_records[MAX_RECORDS];

// Open or create a database
Database* db_open(const char *filename) {
    if (!filename) {
        fprintf(stderr, "Error: Filename is NULL in db_open\n");
        return NULL;
    }

    Database *db = &db_instance; // Use statically allocated instance
    strncpy(db->filename, filename, MAX_KEY_LEN - 1); // Directly copy filename
    db->filename[MAX_KEY_LEN - 1] = '\0';
    db->capacity = MAX_RECORDS;
    db->count = 0;
    db->modified = false;
    db->tombstone_count = 0;
    db->records = db_records; // Use statically allocated records array

    // Clear the hashtable
    hash_table_clear();

    // Try to load existing database from disk
    FILE *f = fopen(filename, "rb");
    if (f) {
        size_t read_count;

        // Read the count
        if (fread(&read_count, sizeof(size_t), 1, f) == 1) {
            // Validate count to prevent buffer overflow
            if (read_count > MAX_RECORDS) {
                fprintf(stderr, "Error: Database file corrupted (record count exceeds maximum)\n");
                fclose(f);
                return NULL;
            }

            for (size_t i = 0; i < read_count; i++) {
                Record temp;
                if (fread(&temp, sizeof(Record), 1, f) == 1) {
                    if (!temp.deleted) { // Skip deleted records
                        if (db->count >= db->capacity) {
                            fprintf(stderr, "Error: Database capacity exceeded while loading\n");
                            fclose(f);
                            return NULL;
                        }
                        db->records[db->count] = temp;
                        hash_table_insert(temp.key, db->count); // Rebuild hashtable
                        db->count++;
                    } else {
                        db->tombstone_count++;
                    }
                } else {
                    fprintf(stderr, "Warning: Failed to read record %zu from database file\n", i);
                    break;
                }
            }
        } else {
            // Empty or corrupted file - treat as new database
            fprintf(stderr, "Warning: Could not read record count from database file, treating as new database\n");
        }
        fclose(f);
    }

    return db;
}

// Atomic file write helper - writes entire database atomically
static int atomic_write_file(const char *filename, const Database *db) {
    // Use platform-appropriate maximum path length
#ifdef _WIN32
    #define MAX_PATH_LEN MAX_PATH
#else
    #define MAX_PATH_LEN PATH_MAX
#endif
    
    char temp_filename[MAX_PATH_LEN];
    size_t filename_len = strlen(filename);
    
    // Ensure we have enough space for filename + ".tmp" + null terminator
    if (filename_len + 5 > MAX_PATH_LEN) {
        fprintf(stderr, "Error: Filename too long for temporary file\n");
        return -1;
    }
    
    if (snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename) >= (int)sizeof(temp_filename)) {
        fprintf(stderr, "Error: Failed to create temporary filename\n");
        return -1;
    }
    
    FILE *f = fopen(temp_filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: Failed to open temporary file: %s\n", strerror(errno));
        return -1;
    }

    // Write count
    if (fwrite(&db->count, sizeof(size_t), 1, f) != 1) {
        fclose(f);
        remove(temp_filename);
        return -1;
    }

    // Write records
    if (db->count > 0) {
        size_t written = fwrite(db->records, sizeof(Record), db->count, f);
        if (written != db->count) {
            fclose(f);
            remove(temp_filename);
            return -1;
        }
    }

    if (fflush(f) != 0 || fclose(f) != 0) {
        remove(temp_filename);
        return -1;
    }

    // Atomic rename (replace old file)
    // On Windows, use MoveFileEx for atomic operation
    // On Unix, rename() is atomic
#ifdef _WIN32
    // Use MoveFileEx with MOVEFILE_REPLACE_EXISTING for atomic operation
    // This replaces the destination file atomically if it exists
    if (MoveFileExA(temp_filename, filename, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0) {
        DWORD error = GetLastError();
        remove(temp_filename);
        fprintf(stderr, "Error: Failed to atomically replace database file: Windows error %lu\n", error);
        return -1;
    }
#else
    // On Unix/Linux, rename() is atomic
    if (rename(temp_filename, filename) != 0) {
        remove(temp_filename);
        fprintf(stderr, "Error: Failed to rename temporary file: %s\n", strerror(errno));
        return -1;
    }
#endif

    return 0;
}

// Close database and save to disk
void db_close(Database *db) {
    if (!db) return;

    // Compact BEFORE saving
    if (db->modified && db->tombstone_count > 0) {
        db_compact(db);
    }

    // Save if modified
    if (db->modified) {
        // Use atomic write to prevent corruption
        if (atomic_write_file(db->filename, db) != 0) {
            fprintf(stderr, "Error: Failed to save database to %s: %s\n", 
                    db->filename, strerror(errno));
            return;
        }
    }

    // Note: db uses static memory allocation, no need to free
}


// Insert a new key-value pair
int db_insert(Database *db, const char *key, const char *value) {
    if (!db || !key || !value) {
        fprintf(stderr, "Error: Invalid arguments to db_insert\n");
        return STATUS_ERROR;
    }

    // Check length limits
    if (strlen(key) >= MAX_KEY_LEN || strlen(value) >= MAX_VALUE_LEN) {
        fprintf(stderr, "Error: Key or value too long\n");
        return STATUS_ERROR;
    }

    // Check if key already exists
    int idx = hash_table_find(key);
    if (idx >= 0) {
        fprintf(stderr, "Error: Key '%s' already exists\n", key);
        return STATUS_EXISTS;
    }

    // Insert new record
    if (db->count >= db->capacity) {
        fprintf(stderr, "Error: Database is full\n");
        return STATUS_FULL;
    }

    strncpy(db->records[db->count].key, key, MAX_KEY_LEN - 1);
    db->records[db->count].key[MAX_KEY_LEN - 1] = '\0';

    strncpy(db->records[db->count].value, value, MAX_VALUE_LEN - 1);
    db->records[db->count].value[MAX_VALUE_LEN - 1] = '\0';

    db->records[db->count].deleted = false;
    hash_table_insert(key, db->count); // Add to hashtable
    db->count++;

    db->modified = true;
    return STATUS_OK;
}

// Get value by key
const char* db_get(Database *db, const char *key) {
    if (!db || !key) {
        fprintf(stderr, "Error: Invalid arguments to db_get\n");
        return NULL;
    }

    int idx = hash_table_find(key);
    if (idx < 0 || idx >= (int)db->count) {
        return NULL;
    }

    // Verify the record is not deleted
    if (db->records[idx].deleted) {
        return NULL;
    }

    // Return a direct pointer to the value in the record
    return db->records[idx].value;
}

// Delete a key-value pair
int db_delete(Database *db, const char *key) {
    if (!db || !key) {
        fprintf(stderr, "Error: Invalid arguments to db_delete\n");
        return STATUS_ERROR;
    }

    int idx = hash_table_find(key);
    if (idx < 0) {
        return STATUS_NOT_FOUND;
    }

    // Mark as deleted (tombstone)
    db->records[idx].deleted = true;
    hash_table_delete(key); // Remove from hashtable
    db->tombstone_count++;
    db->modified = true;

    // Trigger automatic compaction if tombstones exceed threshold
    size_t tombstone_threshold = db->capacity / 5;  // 20% of capacity
    if (db->tombstone_count > tombstone_threshold && db->tombstone_count > 0) {
        db_compact(db);
    }

    return STATUS_OK;
}

// Update db_list to skip empty or uninitialized records
void db_list(Database *db) {
    if (!db) {
        fprintf(stderr, "Error: Invalid database instance in db_list\n");
        return;
    }

    int active_count = 0;
    if (db->count == 0) {
        printf("Database is empty.\n");
        return;
    }

    printf("Keys in database:\n");
    printf("----------------------------------------\n");

    for (size_t i = 0; i < db->count; i++) {
        if (!db->records[i].deleted && strlen(db->records[i].key) > 0) {
            printf("  %s ->  %s\n", db->records[i].key, db->records[i].value);
            active_count++;
        }
    }

    printf("----------------------------------------\n");
    printf("Total: %d active record(s)\n", active_count);
}

// Update the value of an existing key
int db_update(Database *db, const char *key, const char *value) {
    if (!db || !key || !value) {
        fprintf(stderr, "Error: Invalid arguments to db_update\n");
        return STATUS_ERROR;
    }

    // Find the index of the key in the hash table
    int idx = hash_table_find(key);
    if (idx < 0) {
        fprintf(stderr, "Error: Key '%s' not found for update\n", key);
        return STATUS_NOT_FOUND;
    }

    // Ensure the record is not marked as deleted
    if (db->records[idx].deleted) {
        fprintf(stderr, "Error: Key '%s' is marked as deleted\n", key);
        return STATUS_NOT_FOUND;
    }

    // Check value length
    if (strlen(value) >= MAX_VALUE_LEN) {
        fprintf(stderr, "Error: Value too long (max %d characters)\n", MAX_VALUE_LEN - 1);
        return STATUS_ERROR;
    }

    // Update the value of the existing record
    strncpy(db->records[idx].value, value, MAX_VALUE_LEN - 1);
    db->records[idx].value[MAX_VALUE_LEN - 1] = '\0';
    db->modified = true;

    return STATUS_OK;
}

// Compact database by removing deleted records and rebuilding hashtable
void db_compact(Database *db) {
    if (!db) {
        fprintf(stderr, "Error: Invalid database instance in db_compact\n");
        return;
    }

    if (db->tombstone_count == 0) {
        // Nothing to compact
        return;
    }

    Record temp_records[MAX_RECORDS]; // Use local temporary array
    size_t new_count = 0;

    // Save original state for rollback if needed
    size_t original_count = db->count;
    Record original_records[MAX_RECORDS];
    memcpy(original_records, db->records, sizeof(Record) * original_count);

    // Copy active records to the temporary array (don't rebuild hashtable yet)
    for (size_t i = 0; i < db->count; i++) {
        if (!db->records[i].deleted) {
            if (new_count >= MAX_RECORDS) {
                // Rollback on error
                fprintf(stderr, "Error: Compaction would exceed maximum records, rolling back\n");
                memcpy(db->records, original_records, sizeof(Record) * original_count);
                db->count = original_count;
                return;
            }
            temp_records[new_count] = db->records[i];
            new_count++;
        }
    }

    // Only now, after successful copy, clear and rebuild hashtable
    hash_table_clear();
    
    // Copy back to the original records array and rebuild hashtable
    memcpy(db->records, temp_records, sizeof(Record) * new_count);
    db->count = new_count;

    // Rebuild hashtable with new indices
    for (size_t i = 0; i < new_count; i++) {
        hash_table_insert(db->records[i].key, i);
    }

    // Reset tombstone count
    db->tombstone_count = 0;

    db->modified = true;
}