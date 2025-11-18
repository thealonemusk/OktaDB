#include "utility.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Custom Implementation of my_strdup
char* my_strdup(const char* s) {
    if (!s) {
        return NULL;
    }

    static char buffer[MAX_VALUE_LEN]; // Use a static buffer
    size_t len = strlen(s);

    if (len >= MAX_VALUE_LEN) {
        fprintf(stderr, "Error: String too long for buffer in my_strdup\n");
        return NULL;
    }

    memcpy(buffer, s, len + 1); // Copy string including null terminator
    return buffer;
}