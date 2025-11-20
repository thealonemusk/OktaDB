#include "hashtable.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

static HashNode hash_table[HASH_TABLE_SIZE];
static HashNode hash_node_pool[MAX_RECORDS]; // Static memory pool sized to database capacity
static size_t hash_node_pool_index = 0;

// Hash function (djb2-like algorithm)
static uint32_t hash_function(const char *key) {
    if (!key || !*key) {
        return 0;
    }
    uint32_t hash = 5381;
    int c;
    while ((c = (unsigned char)*key++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % HASH_TABLE_SIZE;
}

// Allocate a new HashNode from the pool
static HashNode* allocate_hash_node() {
    if (hash_node_pool_index >= MAX_RECORDS) {
        fprintf(stderr, "Error: HashNode pool exhausted\n");
        return NULL;
    }
    return &hash_node_pool[hash_node_pool_index++];
}

// Insert into hashtable
void hash_table_insert(const char *key, size_t index) {
    if (!key) {
        return;
    }
    
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
    if (!key) {
        return -1;
    }
    
    uint32_t hash = hash_function(key);
    HashNode *node = hash_table[hash].next;
    while (node) {
        if (strcmp(node->key, key) == 0) {
            return (int)node->index;
        }
        node = node->next;
    }
    return -1; // Not found
}

// Delete from hashtable
void hash_table_delete(const char *key) {
    if (!key) {
        return;
    }
    
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
#ifdef DEBUG
    printf("Hashtable cleared.\n");
#endif
}