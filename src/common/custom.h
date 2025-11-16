#ifndef CUSTOM_H
#define CUSTOM_H

#include <stdint.h>
#include <stddef.h>

// Status codes for database operations
typedef enum {
    STATUS_OK = 0,           // Operation successful
    STATUS_ERROR = -1,       // General error
    STATUS_NOT_FOUND = -2,   // Key not found
    STATUS_DUPLICATE = -3,   // Key already exists
    STATUS_FULL = -4         // Database is full
} Status;

// Data types (for future use)
typedef enum {
    TYPE_INT,
    TYPE_STRING,
    TYPE_FLOAT,
    TYPE_BOOL
} DataType;
char* my_strdup(const char* s);
#endif // CUSTOM_H