# Compaction in OktaDB

## What is Compaction?
Compaction is the process of removing deleted records (tombstones) from the database and reorganizing the remaining data. This helps to:
- Reclaim memory and disk space.
- Improve database performance.

---

## How Compaction Works
1. **Identify Active Records**:
   - Active records are those that are not marked as deleted (`deleted = 0`).

2. **Copy Active Records**:
   - A new array is created, and all active records are copied to it.

3. **Replace Old Data**:
   - The old array (containing tombstones) is discarded, and the new array replaces it.

4. **Reset Tombstone Count**:
   - The `tombstone_count` is reset to `0`.

---

## Automatic Compaction
Compaction is triggered automatically when the number of tombstones exceeds 20% of the database capacity. This threshold can be adjusted in the `db_delete` function:
```c
size_t tombstone_threshold = db->capacity / 5;  // 20% of capacity
if (db->tombstone_count > tombstone_threshold) {
    db_compact(db);
}
```

---

## Manual Compaction
You can manually trigger compaction by calling the `db_compact` function:
```c
db_compact(db);
```

---

## Example
```c
Database *db = db_open("test.db");

// Insert some data
db_insert(db, "key1", "value1");
db_insert(db, "key2", "value2");

// Delete a key
db_delete(db, "key1");

// Trigger compaction manually
db_compact(db);

// Close the database
db_close(db);
```

---

## Benefits of Compaction
- **Space Efficiency**: Removes unnecessary data from memory and disk.
- **Performance**: Reduces the size of the database, improving lookup and write speeds.
- **Data Integrity**: Ensures that only active records are retained.

---

## Notes
- Compaction is a resource-intensive operation and may temporarily impact performance.
- Ensure that the database is not being accessed by other processes during compaction.