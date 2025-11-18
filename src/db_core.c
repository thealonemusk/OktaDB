#include "db_core.h" // Include the header file for declarations
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <io.h>
#include <stdint.h>

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
            for (size_t i = 0; i < read_count; i++) {
                Record temp;
                if (fread(&temp, sizeof(Record), 1, f) == 1) {
                    if (!temp.deleted) { // Skip deleted records
                        db->records[db->count] = temp;
                        hash_table_insert(temp.key, db->count); // Rebuild hashtable
                        db->count++;
                    } else {
                        db->tombstone_count++;
                    }
                }
            }
            printf("Loaded %zu records from disk\n", db->count);
        }
        fclose(f);
    } else {
        printf("Created new database\n");
    }

    return db;
}

// Close database and save to disk
void db_close(Database *db) {
    if (!db) return;

    // Compact BEFORE saving (never after free!)
    if (db->modified) {
        db_compact(db);
    }

    // Save if modified
    if (db->modified) {
        FILE *f = fopen(db->filename, "wb");
        if (!f) {
            fprintf(stderr, "Warning: Could not save database to %s\n", db->filename);
        } else {
            // Write count
            if (fwrite(&db->count, sizeof(size_t), 1, f) != 1) {
                fprintf(stderr, "Error: Failed to write record count.\n");
            }

            // Write records
            if (db->count > 0) {
                size_t written = fwrite(db->records, sizeof(Record), db->count, f);
                if (written != db->count) {
                    fprintf(stderr, "Error: Failed to write all records.\n");
                }
            }

            fclose(f);
            printf("Saved %zu records to disk\n", db->count);
        }
    }

    // Free all memory
    free(db->records);
    free(db->filename);
    free(db);
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
    printf("db_insert: Inserted key '%s' at index %zu with value '%s'.\n", key, db->count, value);
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
    if (idx < 0) {
        printf("Key '%s' not found in db_get.\n", key);
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
    if (db->tombstone_count > tombstone_threshold) {
        printf("Tombstone threshold exceeded. Triggering compaction...\n");
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

    // Update the value of the existing record
    strncpy(db->records[idx].value, value, MAX_VALUE_LEN - 1);
    db->records[idx].value[MAX_VALUE_LEN - 1] = '\0';
    db->modified = true;

    printf("OK: Updated key '%s' with new value '%s'.\n", key, value);
    return STATUS_OK;
}

// Enhance db_compact to ensure proper handling of tombstone data
void db_compact(Database *db) {
    if (!db) {
        fprintf(stderr, "Error: Invalid database instance in db_compact\n");
        return;
    }

    Record temp_records[MAX_RECORDS]; // Use local temporary array
    size_t new_count = 0;

    // Copy active records to the temporary array
    for (size_t i = 0; i < db->count; i++) {
        if (!db->records[i].deleted) {
            temp_records[new_count++] = db->records[i];
        } else {
            printf("Skipping tombstone record: %s\n", db->records[i].key);
        }
    }

    // Copy back to the original records array
    memcpy(db->records, temp_records, sizeof(Record) * new_count);
    db->count = new_count;

    // Reset tombstone count
    db->tombstone_count = 0;

    db->modified = true;
    printf("Compaction complete. %zu active records retained.\n", new_count);

    // Save the database to persist changes
    FILE *f = fopen(db->filename, "wb");
    if (!f) {
        fprintf(stderr, "Warning: Could not save database to %s after compaction\n", db->filename);
        return;
    }

    // Write count and records
    fwrite(&db->count, sizeof(size_t), 1, f);
    fwrite(db->records, sizeof(Record), db->count, f);
    fclose(f);
    printf("Database saved after compaction.\n");

    // Debug: Verify in-memory database state
    printf("In-memory database state after compaction:\n");
    for (size_t i = 0; i < db->count; i++) {
        printf("  %s -> %s\n", db->records[i].key, db->records[i].value);
    }
}