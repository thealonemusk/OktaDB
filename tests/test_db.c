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
}

static const char *test_db_open_close() {
    clean_test_db();
    
    db = db_open(TEST_DB_FILE);
    mu_assert("error, db_open failed", db != NULL);
    mu_assert("error, db filename mismatch", strcmp(db->filename, TEST_DB_FILE) == 0);
    mu_assert("error, new db should be empty", db->count == 0);
    
    db_close(db);
    db = NULL;
    
    // Re-open to check persistence (though empty)
    db = db_open(TEST_DB_FILE);
    mu_assert("error, db_open failed on reopen", db != NULL);
    
    clean_test_db();
    return 0;
}

static const char *test_db_insert_get() {
    clean_test_db();
    db = db_open(TEST_DB_FILE);
    
    mu_assert("error, insert failed", db_insert(db, "key1", "value1") == STATUS_OK);
    mu_assert("error, count should be 1", db->count == 1);
    
    const char *val = db_get(db, "key1");
    mu_assert("error, get returned NULL", val != NULL);
    mu_assert("error, value mismatch", strcmp(val, "value1") == 0);
    
    mu_assert("error, get non-existent key should be NULL", db_get(db, "nonexistent") == NULL);
    
    clean_test_db();
    return 0;
}

static const char *test_db_update() {
    clean_test_db();
    db = db_open(TEST_DB_FILE);
    
    db_insert(db, "key1", "value1");
    mu_assert("error, update failed", db_update(db, "key1", "value2") == STATUS_OK);
    
    const char *val = db_get(db, "key1");
    mu_assert("error, value not updated", strcmp(val, "value2") == 0);
    
    mu_assert("error, update non-existent key should fail", db_update(db, "key2", "val") == STATUS_NOT_FOUND);
    
    clean_test_db();
    return 0;
}

static const char *test_db_delete() {
    clean_test_db();
    db = db_open(TEST_DB_FILE);
    
    db_insert(db, "key1", "value1");
    mu_assert("error, delete failed", db_delete(db, "key1") == STATUS_OK);
    mu_assert("error, key should be gone", db_get(db, "key1") == NULL);
    
    // Count might still be 1 if we count tombstones, but let's check implementation details if needed.
    // Based on db_core.h, count includes deleted, so it might be 1.
    // But let's just check that we can't get it.
    
    mu_assert("error, delete non-existent should fail", db_delete(db, "key2") == STATUS_NOT_FOUND);
    
    clean_test_db();
    return 0;
}

static const char *test_db_compact() {
    clean_test_db();
    db = db_open(TEST_DB_FILE);
    
    db_insert(db, "key1", "value1");
    db_insert(db, "key2", "value2");
    db_delete(db, "key1");
    
    // Assuming db_compact is implemented and exposed
    db_compact(db);
    
    mu_assert("error, key2 should still exist", db_get(db, "key2") != NULL);
    mu_assert("error, key1 should be gone", db_get(db, "key1") == NULL);
    
    clean_test_db();
    return 0;
}

const char *all_db_tests() {
    mu_run_test(test_db_open_close);
    mu_run_test(test_db_insert_get);
    mu_run_test(test_db_update);
    mu_run_test(test_db_delete);
    mu_run_test(test_db_compact);
    return 0;
}
