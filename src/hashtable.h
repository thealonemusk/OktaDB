#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stddef.h>
#include "utility.h" // Corrected include path for custom.h

// Define the hashtable structure
#define HASH_TABLE_SIZE 1024

typedef struct HashNode {
    char key[MAX_KEY_LEN];
    size_t index; // Index in the records array
    struct HashNode *next; // For separate chaining
} HashNode;

// Hashtable operations
void hash_table_insert(const char *key, size_t index);
int hash_table_find(const char *key);
void hash_table_delete(const char *key);
void hash_table_clear();

#endif // HASHTABLE_H