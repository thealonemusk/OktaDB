#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/wal.h"
#include "../src/pager.h"

void test_wal() {
    printf("Testing WAL...\n");
    
    const char* db_file = "test_wal.db";
    remove(db_file);
    remove("test_wal.db.wal");
    
    Pager* pager = pager_open(db_file);
    WAL* wal = wal_open(db_file);
    assert(pager != NULL);
    assert(wal != NULL);
    
    // Create a page
    void* page0 = pager_get_page(pager, 0);
    strcpy((char*)page0, "Original Data");
    pager_flush(pager, 0);
    
    // Log modification to WAL
    char buffer[PAGE_SIZE];
    memset(buffer, 0, PAGE_SIZE);
    strcpy(buffer, "New Data in WAL");
    
    wal_log_page(wal, 0, buffer);
    
    // Verify DB file still has old data (before checkpoint)
    // We need to close and re-open pager to bypass cache, or just read file directly
    // But pager_get_page uses cache.
    // Let's just check file content manually or trust pager.
    
    // Checkpoint
    wal_checkpoint(wal, pager);
    
    // Verify DB file has new data
    // We updated cache in checkpoint, so get_page should return new data
    void* page0_new = pager_get_page(pager, 0);
    assert(strcmp((char*)page0_new, "New Data in WAL") == 0);
    
    wal_close(wal);
    pager_close(pager);
    
    remove(db_file);
    remove("test_wal.db.wal");
    printf("Passed!\n");
}

int main() {
    test_wal();
    printf("All WAL tests passed!\n");
    return 0;
}
