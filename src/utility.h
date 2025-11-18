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

#endif // UTILITY_H