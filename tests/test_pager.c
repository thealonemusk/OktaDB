#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/pager.h"

void test_pager_open_close() {
    printf("Testing pager_open and pager_close...\n");
    const char* db_file = "test_pager.db";
    
    Pager* pager = pager_open(db_file);
    assert(pager != NULL);
    pager_close(pager);
    
    // Cleanup
    remove(db_file);
    remove("test_pager.db.wal");
    
    printf("Passed!\n");
}

void test_pager_read_write() {
    printf("Testing pager_read and pager_write...\n");
    const char* db_file = "test_pager.db";
    
    Pager* pager = pager_open(db_file);
    assert(pager != NULL);

    void* page0 = pager_get_page(pager, 0);
    assert(page0 != NULL);

    char* data = (char*)page0;
    strcpy(data, "Hello, Pager!");

    pager_flush(pager, 0);
    pager_close(pager);

    // Re-open and verify
    pager = pager_open(db_file);
    assert(pager != NULL);
    
    page0 = pager_get_page(pager, 0);
    assert(page0 != NULL);
    
    data = (char*)page0;
    assert(strcmp(data, "Hello, Pager!") == 0);
    
    pager_close(pager);
    
    // Cleanup
    remove(db_file);
    remove("test_pager.db.wal");
    
    printf("Passed!\n");
}

int main() {
    test_pager_open_close();
    test_pager_read_write();
    printf("All Pager tests passed!\n");
    return 0;
}
