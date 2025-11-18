# OktaDB Documentation

## Overview
OktaDB is a lightweight, in-memory database with persistent storage. It supports basic operations such as inserting, retrieving, deleting, and compacting data.

---

## Features
- **Insert**: Add new key-value pairs.
- **Update**: Modify existing key-value pairs.
- **Retrieve**: Fetch values by key.
- **Delete**: Mark records as deleted (tombstones).
- **Compaction**: Reclaim space by removing tombstones.
- **Persistence**: Save and load the database from disk.

---

## File Structure
```
OktaDB/
├── src/
│   ├── db_core.c       # Core database logic
│   ├── db_core.h
│   ├── hashtable.c     # Hash table implementation
│   ├── hashtable.h
│   ├── main.c          # Entry point of the application
│   ├── utility.h       # Utility functions
│   
├── build.ps1           # Build script
├── README.md           # Project overview
├── documentation/      # Detailed documentation
│   ├── compaction.md   # Details about compaction
│   ├── general.md      # General documentation
```

---

## Usage

### Opening a Database
```c
Database *db = db_open("test.db");
```

### Inserting Data
```c
db_insert(db, "key1", "value1");
```

### Retrieving Data
```c
char *value = db_get(db, "key1");
if (value) {
    printf("Value: %s\n", value);
    free(value);
}
```

### Deleting Data
```c
db_delete(db, "key1");
```

### Compacting the Database
Compaction is triggered automatically when tombstones exceed 20% of the database capacity. You can also call it manually:
```c
db_compact(db);
```

### Closing the Database
```c
db_close(db);
```

---

## Compaction
Compaction removes deleted records (tombstones) and reorganizes the database to reclaim space. For more details, see `documentation/compaction.md`.

---

## Build and Run
1. Run the build script:
   ```
   ./build.ps1 rebuild
   ```
2. Execute the compiled binary.

---

## License
This project is licensed under the MIT License.