#include "utility.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Portable case-insensitive string comparison functions
#ifdef _WIN32
// Windows: Provide our own implementations to avoid conflicts with MinGW
int oktadb_strcasecmp(const char *s1, const char *s2) {
    if (!s1 || !s2) {
        if (!s1 && !s2) return 0;
        return s1 ? 1 : -1;
    }
    
    while (*s1 && *s2) {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0) return diff;
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

int oktadb_strncasecmp(const char *s1, const char *s2, size_t n) {
    if (n == 0) return 0;
    if (!s1 || !s2) {
        if (!s1 && !s2) return 0;
        return s1 ? 1 : -1;
    }
    
    // Compare up to n characters
    while (n > 0 && *s1 && *s2) {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0) return diff;
        s1++;
        s2++;
        n--;
    }
    
    // If we exhausted all n characters without finding a difference, strings match
    if (n == 0) return 0;
    
    // Otherwise, one string ended before n characters - compare remaining
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}
#else
// Unix/Linux has these in strings.h
#include <strings.h>
#endif

// Function to print help documentation
void print_help(void) {
    printf("OktaDB - A learning database implementation\n");
    printf("Usage:\n");
    printf("  INSERT/ADD <key> <value>  - Insert a key-value pair\n");
    printf("  GET/FETCH <key>           - Retrieve value by key\n");
    printf("  DELETE <key>              - Delete a key-value pair\n");
    printf("  UPDATE <key> <value>      - Update a key-value pair\n");
    printf("  LIST                      - List all keys\n");
    printf("  HELP                      - Show this help\n");
    printf("  CLS                       - Clear the screen\n");
    printf("  EXIT/QUIT/CLOSE           - Exit the program\n");
}

