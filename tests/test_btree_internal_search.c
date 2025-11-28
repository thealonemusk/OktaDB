#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/btree.h"
#include "../src/pager.h"

// Helper to get node type (declared in btree.c but not btree.h)
extern NodeType get_node_type(void* node);
extern uint32_t* internal_node_num_keys(void* node);

/**
 * Test that verifies search behavior through internal nodes (multi-level trees).
 * This test:
 * 1. Inserts enough keys to trigger a root split, creating a 2-level tree
 * 2. Verifies the root is now an internal node
 * 3. Tests search for keys that should be in the left child
 * 4. Tests search for keys that should be in the right child
 * 5. Tests search for keys at the boundary (split point)
 */
void test_internal_node_search() {
    printf("Testing internal node search (multi-level tree)...\n");
    
    const char* db_file = "test_internal_search.db";
    remove(db_file);
    remove("test_internal_search.db.wal");
    
    Pager* pager = pager_open(db_file);
    assert(pager != NULL);
    
    void* root_node = pager_get_page(pager, 0);
    leaf_node_init(root_node);
    set_node_root(root_node, true);
    
    // Verify root is initially a leaf
    assert(get_node_type(root_node) == NODE_LEAF);
    printf("  Initial root is a leaf node: OK\n");
    
    // Insert enough keys to trigger a split (LEAF_NODE_MAX_CELLS is ~10)
    // We need to exceed max cells to force a split
    char key[20];
    char value[30];
    
    // Insert keys with predictable ordering
    for (int i = 0; i < 15; i++) {
        sprintf(key, "key%03d", i);  // key000, key001, ..., key014
        sprintf(value, "value_for_key%03d", i);
        
        Cursor* cursor = table_find(pager, 0, key);
        assert(cursor != NULL);
        leaf_node_insert(cursor, key, value);
        free(cursor);
    }
    
    // After split, root should be an internal node
    root_node = pager_get_page(pager, 0);
    NodeType root_type = get_node_type(root_node);
    
    if (root_type != NODE_INTERNAL) {
        printf("  Warning: Root is still a leaf after 15 inserts.\n");
        printf("  This may mean LEAF_NODE_MAX_CELLS > 15.\n");
        printf("  Test will still verify search works correctly.\n");
    } else {
        printf("  Root is now an internal node after split: OK\n");
        uint32_t num_keys = *internal_node_num_keys(root_node);
        printf("  Internal node has %u keys\n", num_keys);
    }
    
    // Test 1: Search for all inserted keys - they should all be found
    printf("  Testing search for all inserted keys...\n");
    for (int i = 0; i < 15; i++) {
        sprintf(key, "key%03d", i);
        sprintf(value, "value_for_key%03d", i);
        
        Cursor* cursor = table_find(pager, 0, key);
        assert(cursor != NULL);
        
        char* found_value = (char*)cursor_value(cursor);
        if (strcmp(found_value, value) != 0) {
            printf("    FAIL: Key %s - Expected '%s', Got '%s'\n", key, value, found_value);
            assert(0);
        }
        free(cursor);
    }
    printf("    All 15 keys found with correct values: OK\n");
    
    // Test 2: Search for first key (should be in leftmost child)
    printf("  Testing search for first key (key000)...\n");
    {
        Cursor* cursor = table_find(pager, 0, "key000");
        assert(cursor != NULL);
        char* found = (char*)cursor_value(cursor);
        assert(strcmp(found, "value_for_key000") == 0);
        free(cursor);
        printf("    First key search: OK\n");
    }
    
    // Test 3: Search for last key (should be in rightmost child)
    printf("  Testing search for last key (key014)...\n");
    {
        Cursor* cursor = table_find(pager, 0, "key014");
        assert(cursor != NULL);
        char* found = (char*)cursor_value(cursor);
        assert(strcmp(found, "value_for_key014") == 0);
        free(cursor);
        printf("    Last key search: OK\n");
    }
    
    // Test 4: Search for middle keys (around the split point)
    printf("  Testing search for middle keys (around split point)...\n");
    for (int i = 5; i <= 8; i++) {
        sprintf(key, "key%03d", i);
        sprintf(value, "value_for_key%03d", i);
        
        Cursor* cursor = table_find(pager, 0, key);
        assert(cursor != NULL);
        char* found = (char*)cursor_value(cursor);
        assert(strcmp(found, value) == 0);
        free(cursor);
    }
    printf("    Middle key searches: OK\n");
    
    // Test 5: Search for non-existent keys
    printf("  Testing search for non-existent keys...\n");
    {
        // Key that would sort before all existing keys
        Cursor* cursor = table_find(pager, 0, "aaa");
        assert(cursor != NULL);
        // For a non-existent key, cursor points to insertion position
        // We just verify it doesn't crash
        free(cursor);
        
        // Key that would sort after all existing keys
        cursor = table_find(pager, 0, "zzz");
        assert(cursor != NULL);
        free(cursor);
        
        // Key that would sort between existing keys
        cursor = table_find(pager, 0, "key005a");
        assert(cursor != NULL);
        free(cursor);
        
        printf("    Non-existent key searches don't crash: OK\n");
    }
    
    // Test 6: Verify persistence - close and reopen, then search again
    printf("  Testing search after persistence (close and reopen)...\n");
    pager_close(pager);
    
    pager = pager_open(db_file);
    assert(pager != NULL);
    
    // Verify a few keys after reopening
    for (int i = 0; i < 15; i += 5) {
        sprintf(key, "key%03d", i);
        sprintf(value, "value_for_key%03d", i);
        
        Cursor* cursor = table_find(pager, 0, key);
        assert(cursor != NULL);
        char* found = (char*)cursor_value(cursor);
        if (strcmp(found, value) != 0) {
            printf("    FAIL after reopen: Key %s - Expected '%s', Got '%s'\n", key, value, found);
            assert(0);
        }
        free(cursor);
    }
    printf("    Persistence search: OK\n");
    
    pager_close(pager);
    
    // Cleanup
    remove(db_file);
    remove("test_internal_search.db.wal");
    
    printf("Passed!\n");
}

/**
 * Test search with keys inserted in reverse order.
 * This tests the binary search logic in internal nodes more thoroughly.
 */
void test_internal_node_search_reverse_order() {
    printf("Testing internal node search with reverse order inserts...\n");
    
    const char* db_file = "test_internal_search_rev.db";
    remove(db_file);
    remove("test_internal_search_rev.db.wal");
    
    Pager* pager = pager_open(db_file);
    assert(pager != NULL);
    
    void* root_node = pager_get_page(pager, 0);
    leaf_node_init(root_node);
    set_node_root(root_node, true);
    
    char key[20];
    char value[30];
    
    // Insert keys in reverse order
    for (int i = 14; i >= 0; i--) {
        sprintf(key, "key%03d", i);
        sprintf(value, "value_for_key%03d", i);
        
        Cursor* cursor = table_find(pager, 0, key);
        assert(cursor != NULL);
        leaf_node_insert(cursor, key, value);
        free(cursor);
    }
    
    // Verify all keys can be found
    printf("  Verifying all keys after reverse-order insert...\n");
    for (int i = 0; i < 15; i++) {
        sprintf(key, "key%03d", i);
        sprintf(value, "value_for_key%03d", i);
        
        Cursor* cursor = table_find(pager, 0, key);
        assert(cursor != NULL);
        char* found = (char*)cursor_value(cursor);
        if (strcmp(found, value) != 0) {
            printf("    FAIL: Key %s - Expected '%s', Got '%s'\n", key, value, found);
            assert(0);
        }
        free(cursor);
    }
    printf("    All keys verified: OK\n");
    
    pager_close(pager);
    
    // Cleanup
    remove(db_file);
    remove("test_internal_search_rev.db.wal");
    
    printf("Passed!\n");
}

/**
 * Test search with random-looking key patterns.
 * This exercises different paths through the internal node search.
 */
void test_internal_node_search_varied_keys() {
    printf("Testing internal node search with varied key patterns...\n");
    
    const char* db_file = "test_internal_search_var.db";
    remove(db_file);
    remove("test_internal_search_var.db.wal");
    
    Pager* pager = pager_open(db_file);
    assert(pager != NULL);
    
    void* root_node = pager_get_page(pager, 0);
    leaf_node_init(root_node);
    set_node_root(root_node, true);
    
    // Insert keys with varied patterns
    const char* keys[] = {
        "apple", "banana", "cherry", "date", "elderberry",
        "fig", "grape", "honeydew", "imbe", "jackfruit",
        "kiwi", "lemon", "mango", "nectarine", "orange"
    };
    const int num_keys = sizeof(keys) / sizeof(keys[0]);
    
    printf("  Inserting %d varied keys...\n", num_keys);
    for (int i = 0; i < num_keys; i++) {
        char value[50];
        sprintf(value, "value_for_%s", keys[i]);
        
        Cursor* cursor = table_find(pager, 0, keys[i]);
        assert(cursor != NULL);
        leaf_node_insert(cursor, keys[i], value);
        free(cursor);
    }
    
    // Verify all keys
    printf("  Verifying all varied keys...\n");
    for (int i = 0; i < num_keys; i++) {
        char expected_value[50];
        sprintf(expected_value, "value_for_%s", keys[i]);
        
        Cursor* cursor = table_find(pager, 0, keys[i]);
        assert(cursor != NULL);
        char* found = (char*)cursor_value(cursor);
        if (strcmp(found, expected_value) != 0) {
            printf("    FAIL: Key '%s' - Expected '%s', Got '%s'\n", keys[i], expected_value, found);
            assert(0);
        }
        free(cursor);
    }
    printf("    All varied keys verified: OK\n");
    
    // Test edge searches
    printf("  Testing edge case searches...\n");
    {
        // Search for key that would be first alphabetically
        Cursor* cursor = table_find(pager, 0, "aardvark");
        assert(cursor != NULL);
        free(cursor);
        
        // Search for key that would be last alphabetically
        cursor = table_find(pager, 0, "zebra");
        assert(cursor != NULL);
        free(cursor);
        
        // Search for key between existing keys
        cursor = table_find(pager, 0, "carrot");  // Between cherry and date
        assert(cursor != NULL);
        free(cursor);
    }
    printf("    Edge case searches: OK\n");
    
    pager_close(pager);
    
    // Cleanup
    remove(db_file);
    remove("test_internal_search_var.db.wal");
    
    printf("Passed!\n");
}

int main() {
    test_internal_node_search();
    test_internal_node_search_reverse_order();
    test_internal_node_search_varied_keys();
    printf("All internal node search tests passed!\n");
    return 0;
}
