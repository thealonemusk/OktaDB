#include "types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Custom Implementation of my_strdup
char* my_strdup(const char* s) {
    if (!s) {
        return NULL;
    }
    size_t len = strlen(s) + 1;
    char* copy = malloc(len);
    if (!copy) {
        fprintf(stderr, "Error: Memory allocation failed in my_strdup\n");
        return NULL;
    }
    memcpy(copy, s, len);
    return copy;
}