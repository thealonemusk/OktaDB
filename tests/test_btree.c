#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/btree.h"
#include "../src/pager.h"

void test_btree_insert_find() {
    printf("Testing btree_insert and btree_find...\n");
    
    // Setup
    const char* db_file = "test_btree.db";
    remove(db_file);
    remove("test_btree.db.wal");
    
    Pager* pager = pager_open(db_file);
    assert(pager != NULL);
    
    // Initialize root page
    void* root_node = pager_get_page(pager, 0);
    leaf_node_init(root_node);
    
    // Insert
    Cursor* cursor = table_find(pager, 0, "user1");
    leaf_node_insert(cursor, "user1", "Alice");
    free(cursor);
    
    cursor = table_find(pager, 0, "user2");
    leaf_node_insert(cursor, "user2", "Bob");
    free(cursor);
    
    // Verify
    cursor = table_find(pager, 0, "user1");
    assert(strcmp((char*)cursor_value(cursor), "Alice") == 0);
    free(cursor);
    
    cursor = table_find(pager, 0, "user2");
    assert(strcmp((char*)cursor_value(cursor), "Bob") == 0);
    free(cursor);
    
    // Test persistence
    pager_close(pager);
    
    pager = pager_open(db_file);
    cursor = table_find(pager, 0, "user1");
    assert(strcmp((char*)cursor_value(cursor), "Alice") == 0);
    free(cursor);
    
    pager_close(pager);
    
    // Cleanup
    remove(db_file);
    remove("test_btree.db.wal");
    
    printf("Passed!\n");
}

/**
 * Test cursor_advance() function for traversing cells within a single leaf node.
 * 
 * This test verifies that:
 * 1. cursor_advance() correctly moves to the next cell in the leaf node
 * 2. end_of_table is set to true when reaching the last cell
 * 3. All cells can be iterated in order
 * 
 * NOTE: This test only covers single-leaf-node traversal. The cursor_advance()
 * function currently does not support multi-leaf-node traversal - see the
 * function documentation in btree.c for details on this limitation.
 */
void test_cursor_advance_single_leaf() {
    printf("Testing cursor_advance within single leaf node...\n");
    
    const char* db_file = "test_cursor_advance.db";
    remove(db_file);
    remove("test_cursor_advance.db.wal");
    
    Pager* pager = pager_open(db_file);
    assert(pager != NULL);
    
    // Initialize root page
    void* root_node = pager_get_page(pager, 0);
    leaf_node_init(root_node);
    set_node_root(root_node, true);
    
    // Insert multiple keys (ensure they don't trigger a split)
    // With LEAF_NODE_MAX_CELLS = 10, insert fewer than that
    char key[32];
    char value[32];
    int num_inserts = 5;
    
    for (int i = 0; i < num_inserts; i++) {
        sprintf(key, "key%02d", i);
        sprintf(value, "value%02d", i);
        
        Cursor* cursor = table_find(pager, 0, key);
        leaf_node_insert(cursor, key, value);
        free(cursor);
    }
    
    // Start traversal from beginning
    Cursor* cursor = table_start(pager, 0);
    assert(cursor != NULL);
    assert(cursor->end_of_table == false);
    
    // Traverse all cells and verify values are in sorted order
    int count = 0;
    while (!cursor->end_of_table) {
        char* val = (char*)cursor_value(cursor);
        
        // Keys should be in order: key00, key01, key02, key03, key04
        char expected_value[32];
        sprintf(expected_value, "value%02d", count);
        assert(strcmp(val, expected_value) == 0);
        
        count++;
        cursor_advance(cursor);
    }
    
    // Verify we traversed all cells
    assert(count == num_inserts);
    assert(cursor->end_of_table == true);
    
    free(cursor);
    pager_close(pager);
    
    // Cleanup
    remove(db_file);
    remove("test_cursor_advance.db.wal");
    
    printf("Passed!\n");
}

int main() {
    test_btree_insert_find();
    test_cursor_advance_single_leaf();
    printf("All BTree tests passed!\n");
    return 0;
}
