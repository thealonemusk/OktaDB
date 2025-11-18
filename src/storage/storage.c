#include "storage.h"
#include <stdio.h>
#include <stdlib.h>  // For memory allocation functions
#include <string.h>  // For strdup
#include <stdbool.h> // For bool type
#include <unistd.h> // For ftruncate and fileno
#include <io.h> // For _chsize and _fileno

// Internal record structure
typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
    int deleted;  // 1 if deleted, 0 if active
} Record;

// Database structure (hidden from users)
struct Database {
    char *filename;
    Record *records;
    size_t count;       // Number of records (including deleted)
    size_t capacity;    // Maximum capacity
    bool modified;      // 1 if database has unsaved changes
    size_t tombstone_count;  // Number of deleted records
};  

// Replace dynamic allocation with static allocation
static Database db_instance;
static Record db_records[MAX_RECORDS];

// Open or create a database
Database* db_open(const char *filename) {
    if (!filename) {
        fprintf(stderr, "Error: Filename is NULL in db_open\n");
        return NULL;
    }

    Database *db = &db_instance; // Use statically allocated instance
    db->filename = my_strdup(filename); 
    db->capacity = MAX_RECORDS;
    db->count = 0;
    db->modified = false;
    db->tombstone_count = 0;
    db->records = db_records; // Use statically allocated records array

    // Try to load existing database from disk
    FILE *f = fopen(filename, "rb");
    if (f) {
        size_t read_count;

        // Read the count
        if (fread(&read_count, sizeof(size_t), 1, f) == 1) {
            if (read_count <= db->capacity) {
                // Read the records
                size_t read = fread(db->records, sizeof(Record), read_count, f);
                db->count = read;
                printf("Loaded %zu records from disk\n", db->count);
            }
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


// Internal helper: find a record by key
// Returns index if found, -1 if not found
static int find_record(Database *db, const char *key) {
    for (size_t i = 0; i < db->count; i++) {
        if (!db->records[i].deleted && 
            strcmp(db->records[i].key, key) == 0) {
            return (int)i;
        }
    }
    return -1;
}

// Insert or update a key-value pair
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
    int idx = find_record(db, key);
    
    if (idx >= 0) {
        // Update existing record
        strncpy(db->records[idx].value, value, MAX_VALUE_LEN - 1);
        db->records[idx].value[MAX_VALUE_LEN - 1] = '\0';
    } else {
        // Insert new record
        if (db->count >= db->capacity) {
            fprintf(stderr, "Error: Database is full\n");
            return STATUS_FULL;
        }
        
        strncpy(db->records[db->count].key, key, MAX_KEY_LEN - 1);
        db->records[db->count].key[MAX_KEY_LEN - 1] = '\0';
        
        strncpy(db->records[db->count].value, value, MAX_VALUE_LEN - 1);
        db->records[db->count].value[MAX_VALUE_LEN - 1] = '\0';
        
        db->records[db->count].deleted = 0;
        db->count++;
    }

    db->modified = true;
    return STATUS_OK;
}

// Get value by key
char* db_get(Database *db, const char *key) {
    if (!db || !key) {
        fprintf(stderr, "Error: Invalid arguments to db_get\n");
        return NULL;
    }

    int idx = find_record(db, key);
    if (idx < 0) {
        return NULL;
    }

    // Return a copy of the value
    return my_strdup(db->records[idx].value);
}

// Delete a key-value pair
int db_delete(Database *db, const char *key) {
    if (!db || !key) {
        fprintf(stderr, "Error: Invalid arguments to db_delete\n");
        return STATUS_ERROR;
    }

    int idx = find_record(db, key);
    if (idx < 0) {
        return STATUS_NOT_FOUND;
    }

    // Mark as deleted (tombstone)
    db->records[idx].deleted = 1;
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

// List all keys
void db_list(Database *db) {
    if (!db) {
        fprintf(stderr, "Error: Invalid database instance in db_list\n");
        return;
    }

    int active_count = 0;
    if ( db->count == 0) {
        printf("Database is empty.\n");
        return;
    }
    printf("Keys in database:\n");
    printf("----------------------------------------\n");
    
    for (size_t i = 0; i < db->count; i++) {
        if (!db->records[i].deleted) {
            printf("  %s ->  ", db->records[i].key);
            printf(" %s\n", db->records[i].value);
            active_count++;
        }
    }
    
    printf("----------------------------------------\n");
    printf("Total: %d active record(s)\n", active_count);
}

// Update the value for an existing key
int db_update(Database *db, const char *key, const char *value) {
    if (!db || !key || !value) {
        fprintf(stderr, "Error: Invalid arguments to db_update\n");
        return STATUS_ERROR;
    }
    int idx = find_record(db, key);
    if (idx < 0 || db->records[idx].deleted) {
        fprintf(stderr, "Error: Key '%s' not found for update\n", key);
        return STATUS_NOT_FOUND;
    }
    strncpy(db->records[idx].value, value, MAX_VALUE_LEN - 1);
    db->records[idx].value[MAX_VALUE_LEN - 1] = '\0';
    db->modified = true;
    printf("OK: Updated key '%s'\n", key);
    return STATUS_OK;
}

// Modify db_compact to use a static temporary array
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
        }
    }

    // Copy back to the original records array
    memcpy(db->records, temp_records, sizeof(Record) * new_count);
    db->count = new_count;

    // Reset tombstone count
    db->tombstone_count = 0;

    db->modified = true;
    printf("Compaction complete. %zu active records retained.\n", new_count);
}