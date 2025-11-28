#include <stdio.h>
#include "minunit.h"

int tests_run = 0;

// Forward declarations of test suites
extern const char *all_utility_tests();
extern const char *all_db_tests();
extern const char *all_btree_split_tests();

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    const char *result_utility = all_utility_tests();
    const char *result_db = all_db_tests();
    int failed = 0;
    if (result_utility != 0) {
        printf("UTILITY TESTS FAILED: %s\n", result_utility);
        failed = 1;
    }
    if (result_db != 0) {
        printf("DB TESTS FAILED: %s\n", result_db);
        failed = 1;
    }
    const char *result_split = all_btree_split_tests();
    if (result_split != 0) {
        printf("BTREE SPLIT TESTS FAILED: %s\n", result_split);
        failed = 1;
    }
    if (!failed) {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return failed;
}
