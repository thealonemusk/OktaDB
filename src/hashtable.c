#include "hashtable.h"
#include <string.h>
#include <stdint.h> // For uint32_t
#include <stdio.h>  // For debugging

static HashNode hash_table[HASH_TABLE_SIZE];
static HashNode hash_node_pool[HASH_TABLE_SIZE]; // Static memory pool
static size_t hash_node_pool_index = 0;

// Hash function
static uint32_t hash_function(const char *key) {
    uint32_t hash = 0;
    while (*key) {
        hash = (hash * 31) + (unsigned char)(*key++);
    }
    return hash % HASH_TABLE_SIZE;
}

// Allocate a new HashNode from the pool
static HashNode* allocate_hash_node() {
    if (hash_node_pool_index >= HASH_TABLE_SIZE) {
        fprintf(stderr, "Error: HashNode pool exhausted\n");
        return NULL;
    }
    return &hash_node_pool[hash_node_pool_index++];
}

// Insert into hashtable
void hash_table_insert(const char *key, size_t index) {
    uint32_t hash = hash_function(key);
    HashNode *new_node = allocate_hash_node();
    if (!new_node) {
        return;
    }
    strncpy(new_node->key, key, MAX_KEY_LEN - 1);
    new_node->key[MAX_KEY_LEN - 1] = '\0';
    new_node->index = index;
    new_node->next = hash_table[hash].next;
    hash_table[hash].next = new_node;
}

// Find in hashtable
int hash_table_find(const char *key) {
    uint32_t hash = hash_function(key);
    HashNode *node = hash_table[hash].next;
    while (node) {
        if (strcmp(node->key, key) == 0) {
            return node->index;
        }
        node = node->next;
    }
    return -1; // Not found
}

// Delete from hashtable
void hash_table_delete(const char *key) {
    uint32_t hash = hash_function(key);
    HashNode *node = hash_table[hash].next;
    HashNode *prev = &hash_table[hash];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            prev->next = node->next;
            return;
        }
        prev = node;
        node = node->next;
    }
}

// Clear the hashtable
void hash_table_clear() {
    memset(hash_table, 0, sizeof(hash_table));
    hash_node_pool_index = 0; // Reset the memory pool
    printf("Hashtable cleared.\n");
}