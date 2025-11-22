#include "minunit.h"
#include "../src/utility.h"
#include <string.h>

// Test oktadb_strcasecmp
static const char *test_strcasecmp() {
    mu_assert("error, test_strcasecmp: 'a' != 'A'", oktadb_strcasecmp("a", "A") == 0);
    mu_assert("error, test_strcasecmp: 'abc' != 'ABC'", oktadb_strcasecmp("abc", "ABC") == 0);
    mu_assert("error, test_strcasecmp: 'abc' == 'abd'", oktadb_strcasecmp("abc", "abd") < 0);
    mu_assert("error, test_strcasecmp: 'abd' == 'abc'", oktadb_strcasecmp("abd", "abc") > 0);
    return 0;
}

// Test oktadb_strncasecmp
static const char *test_strncasecmp() {
    mu_assert("error, test_strncasecmp: 'abc' != 'ABD' (n=2)", oktadb_strncasecmp("abc", "ABD", 2) == 0);
    mu_assert("error, test_strncasecmp: 'abc' == 'ABD' (n=3)", oktadb_strncasecmp("abc", "ABD", 3) < 0);
    return 0;
}

const char *all_utility_tests() {
    mu_run_test(test_strcasecmp);
    mu_run_test(test_strncasecmp);
    return 0;
}
