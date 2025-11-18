# OktaDB

A simple key-value database implementation in C for learning database internals.

* [ ] Project Structure

```
oktadb/
├── src/
│   ├── db_core.c          # Core database logic
│   ├── db_core.h
│   ├── hashtable.c        # Hash table implementation
│   ├── hashtable.h
│   ├── main.c             # Entry point and REPL
│   ├── utility.h          # Utility functions
├── build/                 # Build artifacts (generated)
├── bin/                   # Compiled binaries (generated)
├── build.ps1              # PowerShell build script
├── README.md              # This file
├── documentation/         # Detailed documentation
│   ├── compaction.md      # Details about compaction
│   ├── general.md         # General documentation
```

## Building

### Windows (PowerShell)

```powershell
# Build the project
.\build.ps1

# Build and run
.\build.ps1 run

# Clean build files
.\build.ps1 clean

# Rebuild from scratch
.\build.ps1 rebuild
```

### Windows (Command Prompt)

```cmd
REM Build the project
build.bat

REM Build and run
build.bat run

REM Clean build files
build.bat clean

REM Rebuild from scratch
build.bat rebuild
```

### Linux/Mac (if using Makefile)

```bash
# Build the project
make

# Build with debug symbols
make debug

# Build optimized release version
make release

# Clean build files
make clean
```

## Running

```powershell
# Windows
.\bin\oktadb.exe mydata.db

# Or use the build script
.\build.ps1 run
```

```bash
# Linux/Mac
./bin/oktadb mydata.db
```

## Usage

Commands available in the REPL:

* `INSERT <key> <value>` - Insert a new key-value pair
* `UPDATE <key> <value>` - Update an existing key-value pair
* `GET <key>` - Retrieve value by key
* `DELETE <key>` - Delete a key-value pair
* `LIST` - List all keys
* `HELP` - Show help message
* `EXIT` - Exit the program

### Example Session

```
oktadb> INSERT user1 Alice
OK: Inserted key 'user1'
oktadb> INSERT user2 Bob
OK: Inserted key 'user2'
oktadb> GET user1
Alice
oktadb> LIST
Keys in database:
----------------------------------------
  user1
  user2
----------------------------------------
Total: 2 active record(s)
oktadb> DELETE user2
OK: Deleted key 'user2'
oktadb> EXIT

Database closed. Goodbye!
```

## Current Features

* Simple key-value storage
* Persistent storage to disk
* Basic CRUD operations (Create, Read, Update, Delete)
* In-memory indexing (linear search)
* Data persists between sessions
* Maximum 1000 records
* Keys up to 127 characters
* Values up to 255 characters

## Updated Features

* Removed all dynamic memory allocations (`malloc`) for improved stability.
* `db_get` now returns direct pointers to database records, avoiding unnecessary string duplication.
* Improved memory management to eliminate potential leaks.
* Added compaction functionality to remove deleted records and reclaim space.

## Roadmap

### Phase 1 (Current) ✅

* [X] Basic file I/O
* [X] Simple key-value operations
* [X] REPL interface
* [X] Persistent storage

### Phase 2 (Next)

* [X] Compaction (remove deleted records)
* [ ] Hash table indexing for O(1) lookups
* [ ] Improved serialization format
* [ ] Better error handling
* [ ] Unit tests

### Phase 3

* [ ] B-tree index for range queries
* [ ] Multiple data types (int, float, bool)
* [ ] Basic SQL parser
* [ ] WHERE clause support
* [ ] SELECT with filtering

### Phase 4

* [ ] JOIN operations
* [ ] Transactions (ACID properties)
* [ ] Write-ahead logging (WAL)
* [ ] Concurrent access support

## Prerequisites

* **Windows** : MinGW-w64 or similar GCC compiler
* **Linux** : GCC (usually pre-installed)
* **Mac** : Xcode Command Line Tools or GCC via Homebrew

### Installing GCC on Windows

1. **MinGW-w64** : Download from https://www.mingw-w64.org/
2. **MSYS2** : Install from https://www.msys2.org/ then run:

```bash
   pacman -S mingw-w64-x86_64-gcc
```

1. Make sure GCC is in your PATH

## Learning Resources

* **Database Systems Concepts** by Silberschatz, Korth, and Sudarshan
* **Architecture of a Database System** by Hellerstein et al.
* **SQLite source code** : https://www.sqlite.org/src/doc/trunk/README.md
* **PostgreSQL documentation** : https://www.postgresql.org/docs/
* **Database Internals** by Alex Petrov
* **CMU Database Systems Course** : https://15445.courses.cs.cmu.edu/

## Technical Details

### Storage Format

Currently using a simple binary format:

1. Record count (8 bytes)
2. Array of fixed-size records:
   * Key (128 bytes)
   * Value (256 bytes)
   * Deleted flag (4 bytes)

### Improvements

* Removed dynamic memory allocations for `Database` and `Record` structures.
* Static memory allocation ensures predictable memory usage.
* Compaction reclaims space by removing tombstones (deleted records).

## Contributing

This is a learning project! Feel free to:

* Add new features
* Improve existing code
* Fix bugs
* Add documentation
* Create tests

## License

MIT License - Feel free to use this for learning!

## Author

Built as a learning project to understand database internals.
