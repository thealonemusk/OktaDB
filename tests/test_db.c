#include "minunit.h"
#include "../src/db_core.h"
#include "../src/utility.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB_FILE "test_db.dat"

static Database *db = NULL;

// Helper to clean up before/after tests
static void clean_test_db() {
    if (db) {
        db_close(db);
        db = NULL;
    }
    remove(TEST_DB_FILE);
    remove("test_db.dat.wal");
}

static const char *test_db_open_close() {
    printf("Running test_db_open_close...\n");
    clean_test_db();
    
    db = db_open(TEST_DB_FILE);
    mu_assert("error, db_open failed", db != NULL);
    mu_assert("error, db filename mismatch", strcmp(db->filename, TEST_DB_FILE) == 0);
    
    db_close(db);
    db = NULL;
    
    // Re-open to check persistence (though empty)
    db = db_open(TEST_DB_FILE);
    mu_assert("error, db_open failed on reopen", db != NULL);
    
    clean_test_db();
    printf("[Pass]  test_db_open_close PASSED\n");
    return 0;
}

static const char *test_db_insert_get() {
    printf("Running test_db_insert_get...\n");
    clean_test_db();
    db = db_open(TEST_DB_FILE);
    
    mu_assert("error, insert failed", db_insert(db, "key1", "value1") == STATUS_OK);
    
    const char *val = db_get(db, "key1");
    mu_assert("error, get returned NULL", val != NULL);
    mu_assert("error, value mismatch", strcmp(val, "value1") == 0);
    
    mu_assert("error, get non-existent key should be NULL", db_get(db, "nonexistent") == NULL);
    
    clean_test_db();
    printf("[Pass]  test_db_insert_get PASSED\n");
    return 0;
}

static const char *test_db_update() {
    printf("Running test_db_update...\n");
    clean_test_db();
    db = db_open(TEST_DB_FILE);
    
    db_insert(db, "key1", "value1");
    mu_assert("error, update failed", db_update(db, "key1", "value2") == STATUS_OK);
    
    const char *val = db_get(db, "key1");
    mu_assert("error, value not updated", strcmp(val, "value2") == 0);
    
    mu_assert("error, update non-existent key should fail", db_update(db, "key2", "val") == STATUS_NOT_FOUND);
    
    clean_test_db();
    printf("[Pass]  test_db_update PASSED\n");
    return 0;
}

const char *all_db_tests() {
    printf("\n=== Running Database Core Tests ===\n");
    mu_run_test(test_db_open_close);
    mu_run_test(test_db_insert_get);
    mu_run_test(test_db_update);
    printf("=== Database Core Tests Complete ===\n\n");
    return 0;
}
