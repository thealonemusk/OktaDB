#include <stdio.h>
#include "minunit.h"

int tests_run = 0;

// Forward declaration of test suites
extern const char *all_tests();

int main(int argc, char **argv) {
    const char *result = all_tests();
    if (result != 0) {
        printf("FAILED: %s\n", result);
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}
