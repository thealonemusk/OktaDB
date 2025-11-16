#include "types.h"
#include <stdlib.h>
#include <string.h>

// Custom Implementation of my_strdup
char* my_strdup(const char* s) {
    if (!s) {
        return NULL;
    }
    size_t len = strlen(s) + 1;
    char* copy = malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}