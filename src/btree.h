#ifndef BTREE_H
#define BTREE_H

#include <stdint.h>
#include "pager.h"

// Node Types
typedef enum { 
    NODE_INTERNAL, 
    NODE_LEAF 
} NodeType;

// Common Node Header Layout
#define NODE_TYPE_SIZE sizeof(uint8_t)
#define NODE_TYPE_OFFSET 0
#define IS_ROOT_SIZE sizeof(uint8_t)
#define IS_ROOT_OFFSET (NODE_TYPE_SIZE)
#define PARENT_POINTER_SIZE sizeof(uint32_t)
#define PARENT_POINTER_OFFSET (IS_ROOT_OFFSET + IS_ROOT_SIZE)
#define COMMON_NODE_HEADER_SIZE (NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE)

// Leaf Node Header Layout
#define LEAF_NODE_NUM_CELLS_SIZE sizeof(uint32_t)
#define LEAF_NODE_NUM_CELLS_OFFSET COMMON_NODE_HEADER_SIZE
#define LEAF_NODE_HEADER_SIZE (COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE)

// Leaf Node Body Layout
#define LEAF_NODE_KEY_SIZE 128
#define LEAF_NODE_VALUE_SIZE 256
#define LEAF_NODE_CELL_SIZE (LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE)
#define LEAF_NODE_SPACE_FOR_CELLS (PAGE_SIZE - LEAF_NODE_HEADER_SIZE)
#define LEAF_NODE_MAX_CELLS (LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE)

// Internal Node Header Layout
#define INTERNAL_NODE_NUM_KEYS_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_NUM_KEYS_OFFSET COMMON_NODE_HEADER_SIZE
#define INTERNAL_NODE_RIGHT_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_RIGHT_CHILD_OFFSET (INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE)
#define INTERNAL_NODE_HEADER_SIZE (COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE)

// Internal Node Body Layout
#define INTERNAL_NODE_KEY_SIZE 128
#define INTERNAL_NODE_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_CELL_SIZE (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE)
#define INTERNAL_NODE_SPACE_FOR_CELLS (PAGE_SIZE - INTERNAL_NODE_HEADER_SIZE)
#define INTERNAL_NODE_MAX_CELLS (INTERNAL_NODE_SPACE_FOR_CELLS / INTERNAL_NODE_CELL_SIZE)

// Cursor for iterating
typedef struct {
    Pager* pager;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table; // Indicates we are past the last element
} Cursor;

// Function Declarations
void leaf_node_init(void* node);
void internal_node_init(void* node);

// Cursor operations
Cursor* table_start(Pager* pager, uint32_t root_page_num);
Cursor* table_find(Pager* pager, uint32_t root_page_num, const char* key);
/**
 * Advances cursor to the next cell. NOTE: Only traverses within a single leaf node.
 * Does not navigate to sibling leaf nodes. See btree.c for full documentation.
 */
void cursor_advance(Cursor* cursor);
void* cursor_value(Cursor* cursor);

// Modification operations
void leaf_node_insert(Cursor* cursor, const char* key, const char* value);
void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level);

// Accessor functions
uint32_t* leaf_node_num_cells(void* node);
void* leaf_node_cell(void* node, uint32_t cell_num);
char* leaf_node_key(void* node, uint32_t cell_num);
char* leaf_node_value(void* node, uint32_t cell_num);
void set_node_root(void* node, bool is_root);
bool is_node_root(void* node);
NodeType get_node_type(void* node);
#endif // BTREE_H
