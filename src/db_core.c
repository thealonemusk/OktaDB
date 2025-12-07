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

#include "db_core.h"
#include "pager.h"
#include "btree.h"
#include "wal.h"
#include "utility.h" // For MAX_FILENAME_LEN, MAX_KEY_LEN, MAX_VALUE_LEN, STATUS_* constants

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
        pager_set_wal(db->pager, NULL);
    }
    if (db->pager) {
        pager_close(db->pager);
    }
}

// Insert a new key-value pair
int db_insert(Database *db, const char *key, const char *value) {
    if (!db || !key || !value) return STATUS_ERROR;
    if (strlen(key) >= MAX_KEY_LEN) {
        fprintf(stderr, "Error: Key too long (max %d chars)\n", MAX_KEY_LEN - 1);
        return STATUS_ERROR;
    }
    if (strlen(value) >= MAX_VALUE_LEN) {
        fprintf(stderr, "Error: Value too long (max %d chars)\n", MAX_VALUE_LEN - 1);
        return STATUS_ERROR;
    }
    Cursor* cursor = table_find(db->pager, 0, key);
    if (!cursor) return STATUS_ERROR;
    void* page = pager_get_page(db->pager, cursor->page_num);
    if (!page) { free(cursor); return STATUS_ERROR; }
    if (cursor->cell_num < *leaf_node_num_cells(page)) {
        char* existing = leaf_node_key(page, cursor->cell_num);
        if (strcmp(key, existing) == 0) { free(cursor); return STATUS_EXISTS; }
    }
    leaf_node_insert(cursor, key, value);
    free(cursor);
    return STATUS_OK;
}

// Get value by key
const char* db_get(Database *db, const char *key) {
    if (!db || !key) return NULL;
    Cursor* cursor = table_find(db->pager, 0, key);
    if (!cursor) return NULL;
    void* page = pager_get_page(db->pager, cursor->page_num);
    if (!page) { free(cursor); return NULL; }
    uint32_t num_cells = *leaf_node_num_cells(page);
    if (cursor->cell_num < num_cells) {
        char* k = leaf_node_key(page, cursor->cell_num);
        if (strcmp(key, k) == 0) {
            char* v = leaf_node_value(page, cursor->cell_num);
            free(cursor);
            return v;
        }
    }
    free(cursor);
    return NULL;
}

// Delete a key-value pair
int db_delete(Database *db, const char *key) {
    if (!db || !key) return STATUS_ERROR;
    Cursor* cursor = table_find(db->pager, 0, key);
    if (!cursor) return STATUS_ERROR;
    void* page = pager_get_page(db->pager, cursor->page_num);
    if (!page) { free(cursor); return STATUS_ERROR; }
    if (get_node_type(page) != NODE_LEAF) { free(cursor); return STATUS_ERROR; }
    uint32_t* num_cells = leaf_node_num_cells(page);
    if (cursor->cell_num >= *num_cells) { free(cursor); return STATUS_NOT_FOUND; }
    char* existing = leaf_node_key(page, cursor->cell_num);
    if (strcmp(key, existing) != 0) { free(cursor); return STATUS_NOT_FOUND; }
    // Shift cells left
    for (uint32_t i = cursor->cell_num; i < *num_cells - 1; i++) {
        void* dest = leaf_node_cell(page, i);
        void* src = leaf_node_cell(page, i + 1);
        memcpy(dest, src, LEAF_NODE_CELL_SIZE);
    }
    (*num_cells)--;
    pager_flush(db->pager, cursor->page_num);
    free(cursor);
    return STATUS_OK;
}

// Update a key's value
int db_update(Database *db, const char *key, const char *value) {
    if (!db || !key || !value) return STATUS_ERROR;
    if (strlen(value) >= LEAF_NODE_VALUE_SIZE) return STATUS_ERROR;
    Cursor* cursor = table_find(db->pager, 0, key);
    if (!cursor) return STATUS_ERROR;
    void* page = pager_get_page(db->pager, cursor->page_num);
    if (!page) { free(cursor); return STATUS_ERROR; }
    uint32_t num_cells = *leaf_node_num_cells(page);
    if (cursor->cell_num < num_cells) {
        char* existing = leaf_node_key(page, cursor->cell_num);
        if (strcmp(key, existing) == 0) {
            char* val_ptr = leaf_node_value(page, cursor->cell_num);
            strncpy(val_ptr, value, LEAF_NODE_VALUE_SIZE - 1);
            val_ptr[LEAF_NODE_VALUE_SIZE - 1] = '\0';
            pager_flush(db->pager, cursor->page_num);
            free(cursor);
            return STATUS_OK;
        }
    }
    free(cursor);
    return STATUS_NOT_FOUND;
}

// Callback type for iteration
typedef void (*db_iter_cb)(const char *key, const char *value, void *ctx);

// Iterate over all records
int db_select_all(Database *db, db_iter_cb callback, void *ctx) {
    if (!db || !callback) return STATUS_ERROR;
    Cursor* cursor = table_start(db->pager, 0);
    if (!cursor) return STATUS_ERROR;
    while (!cursor->end_of_table) {
        void* page = pager_get_page(db->pager, cursor->page_num);
        if (!page) { free(cursor); return STATUS_ERROR; }
        char* key = leaf_node_key(page, cursor->cell_num);
        char* value = leaf_node_value(page, cursor->cell_num);
        callback(key, value, ctx);
        cursor_advance(cursor);
    }
    free(cursor);
    return STATUS_OK;
}

// Find a record by key and invoke callback
int db_select_where(Database *db, const char *key, db_iter_cb callback, void *ctx) {
    if (!db || !key || !callback) return STATUS_ERROR;
    Cursor* cursor = table_find(db->pager, 0, key);
    if (!cursor) return STATUS_ERROR;
    void* page = pager_get_page(db->pager, cursor->page_num);
    if (!page) { free(cursor); return STATUS_ERROR; }
    uint32_t num_cells = *leaf_node_num_cells(page);
    if (cursor->cell_num < num_cells) {
        char* k = leaf_node_key(page, cursor->cell_num);
        if (strcmp(key, k) == 0) {
            char* v = leaf_node_value(page, cursor->cell_num);
            callback(k, v, ctx);
            free(cursor);
            return STATUS_OK;
        }
    }
    free(cursor);
    return STATUS_NOT_FOUND;
}

// Helper printing callback for SELECT *
static void print_row_cb(const char *key, const char *value, void *ctx) {
    (void)ctx;
    printf("| %-20s | %-38s |\n", key, value);
}

// List all keys (SQL‑like table output)
void db_list(Database *db) {
    if (!db) return;
    printf("+----------------------+----------------------------------------+\n");
    printf("| %-20s | %-38s |\n", "Key", "Value");
    printf("+----------------------+----------------------------------------+\n");
    db_select_all(db, print_row_cb, NULL);
    printf("+----------------------+----------------------------------------+\n");
}
