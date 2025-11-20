#ifndef MINUNIT_H
#define MINUNIT_H

#include <stdio.h>

// Simple assertion macro
#define mu_assert(message, test) do { if (!(test)) return message; } while (0)

// Macro to run a test function
#define mu_run_test(test) do { \
    const char *message = test(); \
    tests_run++; \
    if (message) return message; \
} while (0)

// Global variable to count run tests
extern int tests_run;

#endif // MINUNIT_H
