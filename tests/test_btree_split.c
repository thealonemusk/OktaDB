#include "minunit.h"
#include "../src/db_core.h"
#include "../src/utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_SPLIT_DB "test_split.db"

static Database *db = NULL;

static void clean_split_db() {
    if (db) {
        db_close(db);
        db = NULL;
    }
    remove(TEST_SPLIT_DB);
    remove("test_split.db.wal");
}

static const char *test_btree_split_logic() {
    printf("Running test_btree_split_logic...\n");
    clean_split_db();
    
    db = db_open(TEST_SPLIT_DB);
    mu_assert("error, db_open failed", db != NULL);
    
    // Insert 50 records
    // This forces multiple splits (root split, then non-root splits)
    char key[32];
    char value[32];
    
    for (int i = 0; i < 50; i++) {
        snprintf(key, sizeof(key), "key-%02d", i);
        snprintf(value, sizeof(value), "value-%02d", i);
        
        int result = db_insert(db, key, value);
        if (result != STATUS_OK) {
            printf("Insert failed for %s with status %d\n", key, result);
        }
        mu_assert("error, insert failed", result == STATUS_OK);
    }
    
    // Verify
    for (int i = 0; i < 50; i++) {
        snprintf(key, sizeof(key), "key-%02d", i);
        snprintf(value, sizeof(value), "value-%02d", i);
        
        const char* result = db_get(db, key);
        if (!result) {
            printf("Get failed for %s\n", key);
        }
        mu_assert("error, get failed", result != NULL);
        if (strcmp(result, value) != 0) {
            printf("Mismatch for %s: expected %s, got %s\n", key, value, result);
        }
        mu_assert("error, value mismatch", strcmp(result, value) == 0);
    }
    
    clean_split_db();
    printf("[Pass]  test_btree_split_logic PASSED\n");
    return 0;
}

const char *all_btree_split_tests() {
    printf("\n=== Running B-Tree Split Tests ===\n");
    mu_run_test(test_btree_split_logic);
    printf("=== B-Tree Split Tests Complete ===\n\n");
    return 0;
}
