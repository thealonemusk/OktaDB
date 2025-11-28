#include "db_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

static Database db_instance;

// Open or create a database
Database* db_open(const char *filename) {
    if (!filename) {
        fprintf(stderr, "Error: Filename is NULL in db_open\n");
        return NULL;
    }

    Database *db = &db_instance;
    strncpy(db->filename, filename, MAX_KEY_LEN - 1);
    db->filename[MAX_KEY_LEN - 1] = '\0';

    // Open Pager
    db->pager = pager_open(filename);
    if (!db->pager) {
        return NULL;
    }

    // Open WAL
    db->wal = wal_open(filename);
    if (db->wal) {
        pager_set_wal(db->pager, db->wal);
        
        // Checkpoint WAL on startup to recover any unsaved changes
        wal_checkpoint(db->wal, db->pager);
    }

    // Initialize root page if new database
    if (db->pager->num_pages == 0) {
        void* root_node = pager_get_page(db->pager, 0);
        leaf_node_init(root_node);
        set_node_root(root_node, true);
    }

    return db;
}

// Close database and save to disk
void db_close(Database *db) {
    if (!db) return;

    if (db->wal) {
        wal_checkpoint(db->wal, db->pager);
        wal_close(db->wal);
    }

    if (db->pager) {
        pager_close(db->pager);
    }
}

// Insert a new key-value pair
int db_insert(Database *db, const char *key, const char *value) {
    if (!db || !key || !value) {
        return STATUS_ERROR;
    }

    // Validate key and value lengths to prevent buffer overflows
    if (strlen(key) >= MAX_KEY_LEN) {
        fprintf(stderr, "Error: Key too long (max %d chars)\n", MAX_KEY_LEN - 1);
        return STATUS_ERROR;
    }
    if (strlen(value) >= MAX_VALUE_LEN) {
        fprintf(stderr, "Error: Value too long (max %d chars)\n", MAX_VALUE_LEN - 1);
        return STATUS_ERROR;
    }

    Cursor* cursor = table_find(db->pager, 0, key);
    if (!cursor) {
        return STATUS_ERROR;
    }
    
    // Check if key already exists
    if (cursor->cell_num < *leaf_node_num_cells(pager_get_page(db->pager, cursor->page_num))) {
        char* key_at_index = leaf_node_key(pager_get_page(db->pager, cursor->page_num), cursor->cell_num);
        if (strcmp(key, key_at_index) == 0) {
            free(cursor);
            return STATUS_EXISTS;
        }
    }

    leaf_node_insert(cursor, key, value);
    free(cursor);
    return STATUS_OK;
}

// Get value by key
const char* db_get(Database *db, const char *key) {
    if (!db || !key) {
        return NULL;
    }

    Cursor* cursor = table_find(db->pager, 0, key);
    if (!cursor) {
        return NULL;
    }
    
    void* page = pager_get_page(db->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(page);
    
    if (cursor->cell_num < num_cells) {
        char* key_at_index = leaf_node_key(page, cursor->cell_num);
        if (strcmp(key, key_at_index) == 0) {
            char* value = leaf_node_value(page, cursor->cell_num);
            free(cursor);
            return value;
        }
    }
    
    free(cursor);
    return NULL;
}
// Delete a key-value pair
// TODO: Not yet implemented for B-tree storage
int db_delete(Database *db, const char *key) {
    (void)db; (void)key;
    return STATUS_NOT_IMPLEMENTED; 
}

// List all keys
void db_list(Database *db) {
    if (!db) return;
    
    printf("Keys in database:\n");
    printf("----------------------------------------\n");
    
    Cursor* cursor = table_start(db->pager, 0);
    if (!cursor) {
        printf("Error: Failed to create cursor\n");
        return;
    }
    
    int count = 0;
    
    while (!cursor->end_of_table) {
        void* page = pager_get_page(db->pager, cursor->page_num);
        char* key = leaf_node_key(page, cursor->cell_num);
        char* value = leaf_node_value(page, cursor->cell_num);
        printf("  %s -> %s\n", key, value);
        count++;
        cursor_advance(cursor);
    }
    
    printf("----------------------------------------\n");
    printf("Total: %d active record(s)\n", count);
    free(cursor);
}

// Update the value of an existing key
int db_update(Database *db, const char *key, const char *value) {
    if (!db || !key || !value) {
        return STATUS_ERROR;
    }

    // Validate key and value lengths to prevent buffer overflows
    if (strlen(key) >= MAX_KEY_LEN) {
        fprintf(stderr, "Error: Key too long (max %d chars)\n", MAX_KEY_LEN - 1);
        return STATUS_ERROR;
    }
    if (strlen(value) >= MAX_VALUE_LEN) {
        fprintf(stderr, "Error: Value too long (max %d chars)\n", MAX_VALUE_LEN - 1);
        return STATUS_ERROR;
    }

    Cursor* cursor = table_find(db->pager, 0, key);
    if (!cursor) {
        return STATUS_ERROR;
    }
    
    void* page = pager_get_page(db->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(page);
    
    if (cursor->cell_num < num_cells) {
        char* key_at_index = leaf_node_key(page, cursor->cell_num);
        if (strcmp(key, key_at_index) == 0) {
            // Found, update value
            // Note: This is a simplified update that assumes value length fits.
            // In our fixed-size cell design, it always fits (256 bytes).
            char* value_at = leaf_node_value(page, cursor->cell_num);
            strncpy(value_at, value, LEAF_NODE_VALUE_SIZE - 1);
            value_at[LEAF_NODE_VALUE_SIZE - 1] = '\0';
            
            pager_flush(db->pager, cursor->page_num);
            free(cursor);
            return STATUS_OK;
        }
    }
    
    free(cursor);
    return STATUS_NOT_FOUND;
}