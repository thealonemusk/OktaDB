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
    strncpy(db->filename, filename, MAX_FILENAME_LEN - 1);
    db->filename[MAX_FILENAME_LEN - 1] = '\0';

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
        if (!root_node) {
            fprintf(stderr, "Error: Failed to initialize root page\n");
            pager_close(db->pager);
            if (db->wal) wal_close(db->wal);
            return NULL;
        }
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
    
    void* page = pager_get_page(db->pager, cursor->page_num);
    if (!page) {
        free(cursor);
        return STATUS_ERROR;
    }
    
    // Check if key already exists
    if (cursor->cell_num < *leaf_node_num_cells(page)) {
        char* key_at_index = leaf_node_key(page, cursor->cell_num);
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
    if (!page) {
        free(cursor);
        return NULL;
    }
    
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
int db_delete(Database *db, const char *key) {
    if (!db || !key) {
        return STATUS_ERROR;
    }

    // Find the key in the B-tree
    Cursor* cursor = table_find(db->pager, 0, key);
    if (!cursor) {
        return STATUS_ERROR;
    }
    
    void* page = pager_get_page(db->pager, cursor->page_num);
    if (!page) {
        free(cursor);
        return STATUS_ERROR;
    }
    
    uint32_t* num_cells = leaf_node_num_cells(page);
    
    // Check if key exists at cursor position
    if (cursor->cell_num >= *num_cells) {
        free(cursor);
        return STATUS_NOT_FOUND;
    }
    
    char* key_at_index = leaf_node_key(page, cursor->cell_num);
    if (strcmp(key, key_at_index) != 0) {
        free(cursor);
        return STATUS_NOT_FOUND;
    }
    
    // Key found, now delete it by shifting cells left
    // Shift all cells after the deleted cell one position to the left
    for (uint32_t i = cursor->cell_num; i < *num_cells - 1; i++) {
        void* dest_cell = leaf_node_cell(page, i);
        void* src_cell = leaf_node_cell(page, i + 1);
        memcpy(dest_cell, src_cell, LEAF_NODE_CELL_SIZE);
    }
    
    // Decrement the number of cells
    (*num_cells)--;
    
    // Flush the modified page to disk
    pager_flush(db->pager, cursor->page_num);
    
    free(cursor);
    return STATUS_OK;
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
        if (!page) {
            printf("Error: Failed to read page %d\n", cursor->page_num);
            break;
        }
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

    if (strlen(value) >= LEAF_NODE_VALUE_SIZE) {
        return STATUS_ERROR;
    }

    Cursor* cursor = table_find(db->pager, 0, key);
    if (!cursor) {
        return STATUS_ERROR;
    }
    
    void* page = pager_get_page(db->pager, cursor->page_num);
    if (!page) {
        free(cursor);
        return STATUS_ERROR;
    }
    
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