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

int main() {
    test_btree_insert_find();
    printf("All BTree tests passed!\n");
    return 0;
}
