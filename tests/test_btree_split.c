#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/btree.h"
#include "../src/pager.h"

void test_btree_split() {
    printf("Testing btree split...\n");
    
    const char* db_file = "test_split.db";
    remove(db_file);
    remove("test_split.db.wal");
    
    Pager* pager = pager_open(db_file);
    assert(pager != NULL);
    
    void* root_node = pager_get_page(pager, 0);
    leaf_node_init(root_node);
    set_node_root(root_node, true);
    
    // Insert enough keys to trigger split
    // Max cells is around 10-13 depending on header size
    // LEAF_NODE_SPACE_FOR_CELLS = 4096 - HEADER (approx 14) = 4082
    // CELL_SIZE = 128 + 256 = 384
    // MAX_CELLS = 4082 / 384 = 10
    
    char key[20];
    char value[20];
    
    for (int i = 0; i < 15; i++) {
        sprintf(key, "user%02d", i);
        sprintf(value, "Value%02d", i);
        
        Cursor* cursor = table_find(pager, 0, key);
        leaf_node_insert(cursor, key, value);
        free(cursor);
    }
    
    // Verify all keys are present
    for (int i = 0; i < 15; i++) {
        sprintf(key, "user%02d", i);
        sprintf(value, "Value%02d", i);
        
        Cursor* cursor = table_find(pager, 0, key);
        char* val = (char*)cursor_value(cursor);
        if (strcmp(val, value) != 0) {
            printf("Error: Key %s, Expected %s, Got %s\n", key, value, val);
            assert(0);
        }
        free(cursor);
    }
    
    pager_close(pager);
    
    // Cleanup
    remove(db_file);
    remove("test_split.db.wal");
    
    printf("Passed!\n");
}

int main() {
    test_btree_split();
    printf("All Split tests passed!\n");
    return 0;
}
