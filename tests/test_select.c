#include <stdio.h>
#include "../src/db_core.h"
#include "../src/utility.h"
#include "minunit.h"

static const char *test_select_basic() {
    const char *db_file = "test_select.db";
    remove(db_file);
    remove("test_select.db.wal");

    Database *db = db_open(db_file);
    mu_assert("Failed to open DB", db != NULL);

    // Insert records
    mu_assert("Insert a", db_insert(db, "a", "value_a") == STATUS_OK);
    mu_assert("Insert b", db_insert(db, "b", "value_b") == STATUS_OK);
    mu_assert("Insert c", db_insert(db, "c", "value_c") == STATUS_OK);

    // SELECT <key> equivalent using db_get
    const char *val = db_get(db, "b");
    mu_assert("GET b returns value_b", val && strcmp(val, "value_b") == 0);

    // SELECT * WHERE key = c
    const char *val2 = db_get(db, "c");
    mu_assert("WHERE key=c returns value_c", val2 && strcmp(val2, "value_c") == 0);

    // Cleanup
    db_close(db);
    remove(db_file);
    remove("test_select.db.wal");
    return 0;
}

const char *all_select_tests() {
    mu_run_test(test_select_basic);
    return 0;
}
