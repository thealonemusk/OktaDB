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

static const char *test_db_delete_success() {
    printf("Running test_db_delete_success...\n");
    clean_test_db();
    db = db_open(TEST_DB_FILE);
    
    // Insert a key and delete it
    mu_assert("error, insert failed", db_insert(db, "key1", "value1") == STATUS_OK);
    mu_assert("error, delete failed", db_delete(db, "key1") == STATUS_OK);
    
    // Verify the key is gone
    mu_assert("error, key should not exist after deletion", db_get(db, "key1") == NULL);
    
    clean_test_db();
    printf("[Pass]  test_db_delete_success PASSED\n");
    return 0;
}

static const char *test_db_delete_nonexistent() {
    printf("Running test_db_delete_nonexistent...\n");
    clean_test_db();
    db = db_open(TEST_DB_FILE);
    
    // Try to delete a key that doesn't exist
    mu_assert("error, delete should return NOT_FOUND", db_delete(db, "nonexistent") == STATUS_NOT_FOUND);
    
    clean_test_db();
    printf("[Pass]  test_db_delete_nonexistent PASSED\n");
    return 0;
}

static const char *test_db_delete_from_empty() {
    printf("Running test_db_delete_from_empty...\n");
    clean_test_db();
    db = db_open(TEST_DB_FILE);
    
    // Try to delete from empty database
    mu_assert("error, delete from empty should return NOT_FOUND", db_delete(db, "key1") == STATUS_NOT_FOUND);
    
    clean_test_db();
    printf("[Pass]  test_db_delete_from_empty PASSED\n");
    return 0;
}

static const char *test_db_delete_first_key() {
    printf("Running test_db_delete_first_key...\n");
    clean_test_db();
    db = db_open(TEST_DB_FILE);
    
    // Insert multiple keys
    db_insert(db, "aaa", "value1");
    db_insert(db, "bbb", "value2");
    db_insert(db, "ccc", "value3");
    
    // Delete the first key (alphabetically first)
    mu_assert("error, delete first key failed", db_delete(db, "aaa") == STATUS_OK);
    
    // Verify the key is gone and others remain
    mu_assert("error, first key should be deleted", db_get(db, "aaa") == NULL);
    mu_assert("error, second key should exist", db_get(db, "bbb") != NULL);
    mu_assert("error, third key should exist", db_get(db, "ccc") != NULL);
    
    clean_test_db();
    printf("[Pass]  test_db_delete_first_key PASSED\n");
    return 0;
}

static const char *test_db_delete_last_key() {
    printf("Running test_db_delete_last_key...\n");
    clean_test_db();
    db = db_open(TEST_DB_FILE);
    
    // Insert multiple keys
    db_insert(db, "aaa", "value1");
    db_insert(db, "bbb", "value2");
    db_insert(db, "ccc", "value3");
    
    // Delete the last key (alphabetically last)
    mu_assert("error, delete last key failed", db_delete(db, "ccc") == STATUS_OK);
    
    // Verify the key is gone and others remain
    mu_assert("error, first key should exist", db_get(db, "aaa") != NULL);
    mu_assert("error, second key should exist", db_get(db, "bbb") != NULL);
    mu_assert("error, last key should be deleted", db_get(db, "ccc") == NULL);
    
    clean_test_db();
    printf("[Pass]  test_db_delete_last_key PASSED\n");
    return 0;
}

static const char *test_db_delete_middle_key() {
    printf("Running test_db_delete_middle_key...\n");
    clean_test_db();
    db = db_open(TEST_DB_FILE);
    
    // Insert multiple keys
    db_insert(db, "aaa", "value1");
    db_insert(db, "bbb", "value2");
    db_insert(db, "ccc", "value3");
    
    // Delete the middle key
    mu_assert("error, delete middle key failed", db_delete(db, "bbb") == STATUS_OK);
    
    // Verify the key is gone and others remain
    mu_assert("error, first key should exist", db_get(db, "aaa") != NULL);
    mu_assert("error, middle key should be deleted", db_get(db, "bbb") == NULL);
    mu_assert("error, last key should exist", db_get(db, "ccc") != NULL);
    
    clean_test_db();
    printf("[Pass]  test_db_delete_middle_key PASSED\n");
    return 0;
}

static const char *test_db_delete_only_key() {
    printf("Running test_db_delete_only_key...\n");
    clean_test_db();
    db = db_open(TEST_DB_FILE);
    
    // Insert one key and delete it
    db_insert(db, "onlykey", "onlyvalue");
    mu_assert("error, delete only key failed", db_delete(db, "onlykey") == STATUS_OK);
    
    // Verify the database is empty
    mu_assert("error, key should be deleted", db_get(db, "onlykey") == NULL);
    
    clean_test_db();
    printf("[Pass]  test_db_delete_only_key PASSED\n");
    return 0;
}

const char *all_db_tests() {
    printf("\n=== Running Database Core Tests ===\n");
    mu_run_test(test_db_open_close);
    mu_run_test(test_db_insert_get);
    mu_run_test(test_db_update);
    mu_run_test(test_db_delete_success);
    mu_run_test(test_db_delete_nonexistent);
    mu_run_test(test_db_delete_from_empty);
    mu_run_test(test_db_delete_first_key);
    mu_run_test(test_db_delete_last_key);
    mu_run_test(test_db_delete_middle_key);
    mu_run_test(test_db_delete_only_key);
    printf("=== Database Core Tests Complete ===\n\n");
    return 0;
}
