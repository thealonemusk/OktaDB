#include "btree.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Forward declarations
Cursor* leaf_node_find(Pager* pager, uint32_t page_num, const char* key);
void leaf_node_split_and_insert(Cursor* cursor, const char* key, const char* value);
void create_new_root(Pager* pager, uint32_t right_child_page_num);
void internal_node_insert(Pager* pager, uint32_t parent_page_num, uint32_t child_page_num, const char* key);
void internal_node_split_and_insert(Pager* pager, uint32_t parent_page_num, uint32_t child_page_num, const char* key);

// Helper functions to access node fields
uint32_t* leaf_node_num_cells(void* node) {
    return (uint32_t*)(node + LEAF_NODE_NUM_CELLS_OFFSET);
}

void* leaf_node_cell(void* node, uint32_t cell_num) {
    return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

char* leaf_node_key(void* node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num);
}

char* leaf_node_value(void* node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

NodeType get_node_type(void* node) {
    uint8_t value = *((uint8_t*)(node + NODE_TYPE_OFFSET));
    return (NodeType)value;
}

void set_node_type(void* node, NodeType type) {
    uint8_t value = type;
    *((uint8_t*)(node + NODE_TYPE_OFFSET)) = value;
}

bool is_node_root(void* node) {
    uint8_t value = *((uint8_t*)(node + IS_ROOT_OFFSET));
    return (bool)value;
}

void set_node_root(void* node, bool is_root) {
    uint8_t value = is_root;
    *((uint8_t*)(node + IS_ROOT_OFFSET)) = value;
}

uint32_t* node_parent(void* node) {
    return (uint32_t*)(node + PARENT_POINTER_OFFSET);
}

void leaf_node_init(void* node) {
    set_node_type(node, NODE_LEAF);
    set_node_root(node, false);
    *leaf_node_num_cells(node) = 0;
    *node_parent(node) = 0;
}

uint32_t* internal_node_num_keys(void* node) {
    return (uint32_t*)(node + INTERNAL_NODE_NUM_KEYS_OFFSET);
}

uint32_t* internal_node_right_child(void* node) {
    return (uint32_t*)(node + INTERNAL_NODE_RIGHT_CHILD_OFFSET);
}

uint32_t* internal_node_cell(void* node, uint32_t cell_num) {
    return (uint32_t*)(node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE);
}

/**
 * Returns a pointer to the child at the given index in an internal node.
 * WARNING: May return NULL if child_num > num_keys. Callers must check for NULL before dereferencing.
 */
uint32_t* internal_node_child(void* node, uint32_t child_num) {
    uint32_t num_keys = *internal_node_num_keys(node);
    if (child_num > num_keys) {
        fprintf(stderr, "Tried to access child_num %d > num_keys %d\n", child_num, num_keys);
        // Defensive: abort to avoid undefined behavior if caller does not check for NULL
        abort();
        // return NULL;
    }
    if (child_num == num_keys) {
        return internal_node_right_child(node);
    }
    return internal_node_cell(node, child_num);
}

char* internal_node_key(void* node, uint32_t key_num) {
    return (char*)((void*)internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE);
}

void internal_node_init(void* node) {
    set_node_type(node, NODE_INTERNAL);
    set_node_root(node, false);
    *internal_node_num_keys(node) = 0;
    *node_parent(node) = 0;
}

// Cursor operations
Cursor* table_start(Pager* pager, uint32_t root_page_num) {
    Cursor* cursor = malloc(sizeof(Cursor));
    if (!cursor) {
        fprintf(stderr, "Failed to allocate memory for cursor\n");
        return NULL;
    }
    cursor->pager = pager;
    cursor->page_num = root_page_num;
    cursor->cell_num = 0;
    
    void* root_node = pager_get_page(pager, root_page_num);
    if (!root_node) {
        fprintf(stderr, "Failed to get root page in table_start\n");
        free(cursor);
        return NULL;
    }
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->end_of_table = (num_cells == 0);
    
    return cursor;
}

Cursor* table_find(Pager* pager, uint32_t root_page_num, const char* key) {
    void* root_node = pager_get_page(pager, root_page_num);
    if (!root_node) {
        fprintf(stderr, "Failed to get root page in table_find\n");
        return NULL;
    }
    
    if (get_node_type(root_node) == NODE_LEAF) {
        return leaf_node_find(pager, root_page_num, key);
    } else {
        // Internal node search
        uint32_t num_keys = *internal_node_num_keys(root_node);
        uint32_t min_index = 0;
        uint32_t max_index = num_keys;
        
        while (min_index != max_index) {
            uint32_t index = (min_index + max_index) / 2;
            char* key_at_index = internal_node_key(root_node, index);
            int cmp = strcmp(key, key_at_index);
            
            if (cmp >= 0) {
                min_index = index + 1;
            } else {
                max_index = index;
            }
        }
        
        uint32_t child_num = min_index;
        uint32_t child_page_num = *internal_node_child(root_node, child_num);
        
        return table_find(pager, child_page_num, key);
    }
}

// Find key in a leaf node
Cursor* leaf_node_find(Pager* pager, uint32_t page_num, const char* key) {
    void* node = pager_get_page(pager, page_num);
    if (!node) {
        fprintf(stderr, "Failed to get page %d in leaf_node_find\n", page_num);
        return NULL;
    }
    uint32_t num_cells = *leaf_node_num_cells(node);
    
    Cursor* cursor = malloc(sizeof(Cursor));
    if (!cursor) {
        fprintf(stderr, "Failed to allocate memory for cursor\n");
        return NULL;
    }
    cursor->pager = pager;
    cursor->page_num = page_num;
    
    // Binary search
    uint32_t min_index = 0;
    uint32_t max_index = num_cells;
    
    while (min_index != max_index) {
        uint32_t index = (min_index + max_index) / 2;
        char* key_at_index = leaf_node_key(node, index);
        int cmp = strcmp(key, key_at_index);
        
        if (cmp == 0) {
            cursor->cell_num = index;
            return cursor;
        }
        if (cmp < 0) {
            max_index = index;
        } else {
            min_index = index + 1;
        }
    }
    
    cursor->cell_num = min_index;
    return cursor;
}

void leaf_node_insert(Cursor* cursor, const char* key, const char* value) {
    void* node = pager_get_page(cursor->pager, cursor->page_num);
    if (!node) {
        fprintf(stderr, "Failed to get page %d in leaf_node_insert\n", cursor->page_num);
        return;
    }
    uint32_t num_cells = *leaf_node_num_cells(node);
    
    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        leaf_node_split_and_insert(cursor, key, value);
        return;
    }
    
    if (cursor->cell_num < num_cells) {
        // Make room for new cell
        for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }
    
    *(leaf_node_num_cells(node)) += 1;
    
    char* key_at = leaf_node_key(node, cursor->cell_num);
    char* value_at = leaf_node_value(node, cursor->cell_num);
    
    strncpy(key_at, key, LEAF_NODE_KEY_SIZE - 1);
    key_at[LEAF_NODE_KEY_SIZE - 1] = '\0';
    
    strncpy(value_at, value, LEAF_NODE_VALUE_SIZE - 1);
    value_at[LEAF_NODE_VALUE_SIZE - 1] = '\0';
    
    pager_flush(cursor->pager, cursor->page_num);
}

void internal_node_insert(Pager* pager, uint32_t parent_page_num, uint32_t child_page_num, const char* key) {
    printf("DEBUG: internal_node_insert parent=%d child=%d key=%s\n", parent_page_num, child_page_num, key); fflush(stdout);
    void* node = pager_get_page(pager, parent_page_num);
    if (!node) {
        fprintf(stderr, "Failed to get parent page %d in internal_node_insert\n", parent_page_num);
        return;
    }
    
    uint32_t num_keys = *internal_node_num_keys(node);
    printf("DEBUG: num_keys=%d MAX=%d\n", num_keys, INTERNAL_NODE_MAX_CELLS); fflush(stdout);
    
    if (num_keys >= INTERNAL_NODE_MAX_CELLS) {
        printf("DEBUG: Calling internal_node_split_and_insert\n"); fflush(stdout);
        internal_node_split_and_insert(pager, parent_page_num, child_page_num, key);
        return;
    }
    
    uint32_t right_child_page_num = *internal_node_right_child(node);
    uint32_t child_max_key_index = num_keys; // Default to append
    
    // Find the index of the child that we are splitting (it's the left child of the new key)
    // We are inserting (Key, NewChild) effectively.
    // But we need to find WHERE.
    // We search for the child_page_num that matches the *left* child of the split.
    // Wait, the caller passes 'child_page_num' which is the RIGHT child of the split.
    // The LEFT child is already in the node.
    // We need to find the index where the LEFT child is.
    
    // Actually, simpler: Find the index where the key should go.
    // Since internal nodes are sorted, we can search for the key.
    // But wait, the key is the separator.
    // It's guaranteed to be > all keys in left child and <= all keys in right child.
    
    // Let's find the index i such that internal_node_child(node, i) == left_child_page_num?
    // No, we don't have left_child_page_num passed in.
    // But we know that 'key' is the smallest key in 'child_page_num' (the new right child).
    // So we can just search for 'key'.
    
    uint32_t index = 0;
    while (index < num_keys) {
        char* key_at_index = internal_node_key(node, index);
#ifdef DEBUG
        printf("DEBUG: checking index %d key_at=%p\n", index, key_at_index); fflush(stdout);
#endif
        if (strcmp(key, key_at_index) < 0) {
            break;
        }
        index++;
    }
    
    // Insert at 'index'
    // If index == num_keys, we are appending.
    
    if (index == num_keys) {
        // Append
        // New Cell at num_keys: (Old Right Child, Key)
        // New Right Child: child_page_num
        
        *internal_node_cell(node, num_keys) = right_child_page_num;
        char* key_at = internal_node_key(node, num_keys);
        strncpy(key_at, key, INTERNAL_NODE_KEY_SIZE - 1);
        key_at[INTERNAL_NODE_KEY_SIZE - 1] = '\0';
        
        *internal_node_right_child(node) = child_page_num;
    } else {
        // Insert in middle
        // Shift cells from index to num_keys-1 to index+1
        for (uint32_t i = num_keys; i > index; i--) {
            void* dest = internal_node_cell(node, i);
            void* src = internal_node_cell(node, i - 1);
            memcpy(dest, src, INTERNAL_NODE_CELL_SIZE);
        }
        
        // Update the child pointer of the shifted cell (index+1) to be the new child
        *internal_node_cell(node, index + 1) = child_page_num;
        
        // Set the key at index
        char* key_at = internal_node_key(node, index);
        strncpy(key_at, key, INTERNAL_NODE_KEY_SIZE - 1);
        key_at[INTERNAL_NODE_KEY_SIZE - 1] = '\0';
    }
    
    *internal_node_num_keys(node) += 1;
    pager_flush(pager, parent_page_num);
}

void internal_node_split_and_insert(Pager* pager, uint32_t parent_page_num, uint32_t child_page_num, const char* key) {
    fprintf(stderr, "Error: Internal node split not implemented. Database full.\n");
    abort();
    (void)pager; (void)parent_page_num; (void)child_page_num; (void)key;
}

void leaf_node_split_and_insert(Cursor* cursor, const char* key, const char* value) {
    printf("DEBUG: leaf_node_split_and_insert page=%d key=%s\n", cursor->page_num, key); fflush(stdout);
    void* old_node = pager_get_page(cursor->pager, cursor->page_num);
    if (!old_node) {
        fprintf(stderr, "Failed to get page %d in leaf_node_split_and_insert\n", cursor->page_num);
        return;
    }
    
    if (is_node_root(old_node)) {
        create_new_root(cursor->pager, cursor->pager->num_pages + 1); // We will allocate two pages
        // Re-read root because create_new_root modified it
        void* root = pager_get_page(cursor->pager, 0);
        if (!root) {
            fprintf(stderr, "Failed to get root page after split\n");
            return;
        }
        uint32_t left_child_page_num = *internal_node_child(root, 0);
        uint32_t right_child_page_num = *internal_node_right_child(root);
        
        void* left_child = pager_get_page(cursor->pager, left_child_page_num);
        if (!left_child) {
            fprintf(stderr, "Failed to get left child page %d\n", left_child_page_num);
            return;
        }
        void* right_child = pager_get_page(cursor->pager, right_child_page_num);
        if (!right_child) {
            fprintf(stderr, "Failed to get right child page %d\n", right_child_page_num);
            return;
        }
        
        // Left child is a copy of old root, so it is full.
        // We need to move the upper half of cells to the right child.
        uint32_t num_cells = *leaf_node_num_cells(left_child);
        uint32_t split_index = (num_cells + 1) / 2;
        
        leaf_node_init(right_child);
        
        // Move cells
        for (uint32_t i = split_index; i < num_cells; i++) {
            void* dest = leaf_node_cell(right_child, i - split_index);
            void* src = leaf_node_cell(left_child, i);
            memcpy(dest, src, LEAF_NODE_CELL_SIZE);
        }
        
        *leaf_node_num_cells(left_child) = split_index;
        *leaf_node_num_cells(right_child) = num_cells - split_index;
        
        // Update parent pointers
        *node_parent(left_child) = 0; // Root is always page 0
        *node_parent(right_child) = 0;
        
        // Update root key (first key of right child)
        // Internal node keys are the maximum key of the left child? 
        // Or the first key of the right child?
        // B+Tree: Internal nodes contain keys that separate children.
        // Usually: Key[i] <= Child[i] < Key[i+1] ...
        // Or: Child[i] <= Key[i] < Child[i+1]
        // Let's use: Key[i] is the smallest key in Child[i+1] (Right child).
        // So Key[0] should be the first key of the Right Child.
        
        char* right_first_key = leaf_node_key(right_child, 0);
        char* root_key = internal_node_key(root, 0);
        strncpy(root_key, right_first_key, INTERNAL_NODE_KEY_SIZE - 1);
        root_key[INTERNAL_NODE_KEY_SIZE - 1] = '\0';
        
        // Now insert the new key into the appropriate child
        // We need to find which child to insert into.
        // Since we just split, we can check the key against the split key.
        
        if (strcmp(key, right_first_key) < 0) {
            // Insert into left child
            // We need a new cursor for the left child
            
            // Re-find in left child
            Cursor* left_cursor = leaf_node_find(cursor->pager, left_child_page_num, key);
            leaf_node_insert(left_cursor, key, value);
            free(left_cursor);
        } else {
            // Insert into right child
            Cursor* right_cursor = leaf_node_find(cursor->pager, right_child_page_num, key);
            leaf_node_insert(right_cursor, key, value);
            free(right_cursor);
        }
        
        return;
    }
    
    // Non-root leaf node split
    uint32_t right_child_page_num = cursor->pager->num_pages;
    cursor->pager->num_pages++;
    
    void* right_child = pager_get_page(cursor->pager, right_child_page_num);
    if (!right_child) {
        fprintf(stderr, "Failed to allocate right child page %d\n", right_child_page_num);
        return;
    }
    
    leaf_node_init(right_child);
    
    // Move upper half of cells to right child
    uint32_t num_cells = *leaf_node_num_cells(old_node);
    uint32_t split_index = (num_cells + 1) / 2;
    
    for (uint32_t i = split_index; i < num_cells; i++) {
        void* dest = leaf_node_cell(right_child, i - split_index);
        void* src = leaf_node_cell(old_node, i);
        memcpy(dest, src, LEAF_NODE_CELL_SIZE);
    }
    
    *leaf_node_num_cells(old_node) = split_index;
    *leaf_node_num_cells(right_child) = num_cells - split_index;
    
    // Update parent pointers
    // Right child shares the same parent as the old node (left child)
    uint32_t parent_page_num = *node_parent(old_node);
    *node_parent(right_child) = parent_page_num;
    
    // Insert the new key into the parent
    // The key is the first key of the right child
    char* right_first_key = leaf_node_key(right_child, 0);
    
    internal_node_insert(cursor->pager, parent_page_num, right_child_page_num, right_first_key);
    
    // Insert the new key/value into the appropriate child
    if (strcmp(key, right_first_key) < 0) {
        // Insert into left child (old_node)
        // We need to re-find the position because we modified the node
        // But we can just use the existing cursor?
        // No, the cursor might be invalid if we moved cells?
        // Actually, leaf_node_insert handles finding position if we pass a cursor.
        // But we should probably re-find to be safe and simple.
        Cursor* left_cursor = leaf_node_find(cursor->pager, cursor->page_num, key);
        leaf_node_insert(left_cursor, key, value);
        free(left_cursor);
    } else {
        // Insert into right child
        Cursor* right_cursor = leaf_node_find(cursor->pager, right_child_page_num, key);
        leaf_node_insert(right_cursor, key, value);
        free(right_cursor);
    }
}

void create_new_root(Pager* pager, uint32_t right_child_page_num) {
    void* root = pager_get_page(pager, 0);
    if (!root) {
        fprintf(stderr, "Failed to get root page in create_new_root\n");
        return;
    }
    
    uint32_t left_child_page_num = pager->num_pages; // Allocate new page for left child
    pager->num_pages++; // Increment manually since we are "allocating"
    
    // We expect right_child_page_num to be passed in as the next available page
    // So we should ensure we allocate it too if it wasn't already.
    if (right_child_page_num >= pager->num_pages) {
        pager->num_pages = right_child_page_num + 1;
    }
    
    void* left_child = pager_get_page(pager, left_child_page_num);
    if (!left_child) {
        fprintf(stderr, "Failed to get left child page %d in create_new_root\n", left_child_page_num);
        return;
    }
    
    // Copy root to left child
    memcpy(left_child, root, PAGE_SIZE);
    set_node_root(left_child, false);
    
    // Initialize root as internal node
    internal_node_init(root);
    set_node_root(root, true);
    *internal_node_num_keys(root) = 1;
    *internal_node_right_child(root) = right_child_page_num;
    *internal_node_child(root, 0) = left_child_page_num;
}

void* cursor_value(Cursor* cursor) {
    void* page = pager_get_page(cursor->pager, cursor->page_num);
    if (!page) {
        fprintf(stderr, "Failed to get page %d in cursor_value\n", cursor->page_num);
        return NULL;
    }
    return leaf_node_value(page, cursor->cell_num);
}

/**
 * Advances the cursor to the next cell in the current leaf node.
 * 
 * LIMITATION: This function only handles advancing within a single leaf node.
 * When the cursor reaches the end of the current leaf node, it sets end_of_table
 * to true rather than navigating to the next sibling leaf node. This means that
 * iteration using cursor_advance() will stop at the end of each leaf node, even
 * if there are more records in sibling leaf nodes after a B-tree split.
 * 
 * To properly support multi-node traversal, the leaf nodes would need to maintain
 * sibling pointers (next_leaf field), or the cursor would need to track the path
 * through internal nodes.
 */
void cursor_advance(Cursor* cursor) {
    void* node = pager_get_page(cursor->pager, cursor->page_num);
    if (!node) {
        fprintf(stderr, "Failed to get page %d in cursor_advance\n", cursor->page_num);
        cursor->end_of_table = true;
        return;
    }
    uint32_t num_cells = *leaf_node_num_cells(node);
    
    cursor->cell_num += 1;
    if (cursor->cell_num >= num_cells) {
        // NOTE: Does not navigate to next leaf node - see function documentation
        cursor->end_of_table = true;
    }
}

void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level) {
    void* node = pager_get_page(pager, page_num);
    if (!node) {
        fprintf(stderr, "Failed to get page %d in print_tree\n", page_num);
        return;
    }
    NodeType type = get_node_type(node);
    uint32_t i, j;
    switch (type) {
        case NODE_LEAF: {
            uint32_t num_cells = *leaf_node_num_cells(node);
            for (i = 0; i < indentation_level; i++) {
                printf("  ");
            }
            printf("- leaf (size %d)\n", num_cells);
            for (i = 0; i < num_cells; i++) {
                for (j = 0; j < indentation_level + 1; j++) {
                    printf("  ");
                }
                printf("%s\n", leaf_node_key(node, i));
            }
            break;
        }
        case NODE_INTERNAL: {
            uint32_t num_keys = *internal_node_num_keys(node);
            for (i = 0; i < indentation_level; i++) {
                printf("  ");
            }
            printf("- internal (size %d)\n", num_keys);
            for (i = 0; i < num_keys; i++) {
                uint32_t child_page_num = *internal_node_child(node, i);
                print_tree(pager, child_page_num, indentation_level + 1);
                for (j = 0; j < indentation_level + 1; j++) {
                    printf("  ");
                }
                printf("%s\n", internal_node_key(node, i));
            }
            // Print rightmost child
            uint32_t right_child_page_num = *internal_node_right_child(node);
            print_tree(pager, right_child_page_num, indentation_level + 1);
            break;
        }
    }
}
