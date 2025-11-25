#ifndef UTILITY_H
#define UTILITY_H

#include <stdint.h>
#include <stddef.h>

// Status codes for database operations
typedef enum {
    STATUS_OK = 0,           // Operation successful
    STATUS_ERROR = -1,       // General error
    STATUS_NOT_FOUND = -2,   // Key not found
    STATUS_DUPLICATE = -3,   // Key already exists
    STATUS_FULL = -4,         // Database is full
    STATUS_EXISTS = 1         // Key exists
} Status;

// Centralized constants
#define MAX_RECORDS 1000
#define MAX_KEY_LEN 128
#define MAX_VALUE_LEN 256

// Data types (for future use)
typedef enum {
    TYPE_INT,
    TYPE_STRING,
    TYPE_FLOAT,
    TYPE_BOOL
} DataType;

// Portable case-insensitive string comparison functions
// These provide cross-platform case-insensitive string comparison
#ifndef _WIN32
// Unix/Linux has these in strings.h, include it
#include <strings.h>
// Use standard library functions on Unix
#define oktadb_strcasecmp strcasecmp
#define oktadb_strncasecmp strncasecmp
#else
// Windows: Provide our own implementations to avoid conflicts
int oktadb_strcasecmp(const char *s1, const char *s2);
int oktadb_strncasecmp(const char *s1, const char *s2, size_t n);
#endif

// Utility function declarations
void print_help(void);
void clear_screen(void);

#endif // UTILITY_H