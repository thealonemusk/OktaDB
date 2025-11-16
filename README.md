# OktaDB


**#**OktaDB **-** A Learning Database Project

A simple key**-**value database implementation in C **for** learning database internals**.**

Project Structure

```
oktadb**/**
├── src**/**
│   ├── main**.**c              # Entry point and REPL
│   ├── common**/**
│   │   └── types**.**h         # Common type definitions
│   └── storage**/**
│       ├── storage**.**h       # Storage engine interface
│       └── storage**.**c       # Storage engine implementation
├── build**/**                  # Build **artifacts**(generated**)**
├── bin**/**                    # Compiled **binaries**(generated**)**
├── Makefile               # Build configuration
└── README**.**md              # This file
```

## Building

```bash
**#**Build the project
make

**#**Build with debug symbols
make debug

**#**Build optimized release version
make release

**#**Clean build files
make clean
```

## Running

```bash
**#**Run with a database file
**.**/bin**/**oktadb mydata**.**db

**#**Or use make
make run
```

## Usage

Commands available in the REPL**:**

**-** `INSERT **<**key**>**<value**>**` **-** Insert or update a key**-**value pair
**-** `GET **<**key**>**` **-** Retrieve value by key
**-** `DELETE **<**key**>**` **-** Delete a key**-**value pair
**-** `LIST` **-** List all keys
**-** `HELP` **-** Show help message
**-** `EXIT` **-** Exit the program

Example session**:**

```
oktadb**>** INSERT user1 Alice
OK**:** Inserted key **'user1'**
oktadb**>** INSERT user2 Bob
OK**:** Inserted key **'user2'**
oktadb**>** GET user1
Alice
oktadb**>** LIST
Keys in database**:**
  user1
  user2
Total**:**2 active records
oktadb**>** EXIT
```

## Current Features

**-** Simple key**-**value storage
**-** Persistent storage to disk
**-** Basic CRUD operations
**-** In**-**memory **indexing**(linear search**)**

## Roadmap

### Phase **1**(Current**)**

**-**[x**]** Basic file I**/**O
**-**[x**]** Simple key**-**value operations
**-**[x**]** REPL interface

### Phase **2**(Next**)**

**-**[**]** Hash table indexing
**-**[**]** Improved serialization format
**-**[**]** Error handling improvements
**-**[**]** Unit tests

### Phase **3**

**-**[**]** B**-**tree index
**-**[**]** Multiple data types
**-**[**]** Basic SQL parser
**-**[**]** WHERE clause support

### Phase **4**

**-**[**]** JOIN operations
**-**[**]** Transactions
**-**[**]** Write**-**ahead logging

## Learning Resources

**-** Database Systems **Concepts**(Silberschatz**)**
**-** Architecture of a Database **System**(Hellerstein et al**.**)
**-** SQLite source code
**-** PostgreSQL documentation

**#**oktadb **-** A Learning Database Project

A simple key**-**value database implementation in C **for** learning database internals**.**

## Project Structure

```
oktadb**/**
├── src**/**
│   ├── main**.**c              # Entry point and REPL
│   ├── common**/**
│   │   └── types**.**h         # Common type definitions
│   └── storage**/**
│       ├── storage**.**h       # Storage engine interface
│       └── storage**.**c       # Storage engine implementation
├── build**/**                  # Build **artifacts**(generated**)**
├── bin**/**                    # Compiled **binaries**(generated**)**
├── Makefile               # Build configuration
└── README**.**md              # This file
```

## Building

```bash
**#**Build the project
make

**#**Build with debug symbols
make debug

**#**Build optimized release version
make release

**#**Clean build files
make clean
```

## Running

```bash
**#**Run with a database file
**.**/bin**/**oktadb mydata**.**db

**#**Or use make
make run
```

## Usage

Commands available in the REPL**:**

**-** `INSERT **<**key**>**<value**>**` **-** Insert or update a key**-**value pair
**-** `GET **<**key**>**` **-** Retrieve value by key
**-** `DELETE **<**key**>**` **-** Delete a key**-**value pair
**-** `LIST` **-** List all keys
**-** `HELP` **-** Show help message
**-** `EXIT` **-** Exit the program

Example session**:**

```
oktadb**>** INSERT user1 Alice
OK**:** Inserted key **'user1'**
oktadb**>** INSERT user2 Bob
OK**:** Inserted key **'user2'**
oktadb**>** GET user1
Alice
oktadb**>** LIST
Keys in database**:**
  user1
  user2
Total**:**2 active records
oktadb**>** EXIT
```

## Current Features

**-** Simple key**-**value storage
**-** Persistent storage to disk
**-** Basic CRUD operations
**-** In**-**memory **indexing**(linear search**)**

## Roadmap

### Phase **1**(Current**)**

**-**[x**]** Basic file I**/**O
**-**[x**]** Simple key**-**value operations
**-**[x**]** REPL interface

### Phase **2**(Next**)**

**-**[**]** Hash table indexing
**-**[**]** Improved serialization format
**-**[**]** Error handling improvements
**-**[**]** Unit tests

### Phase **3**

**-**[**]** B**-**tree index
**-**[**]** Multiple data types
**-**[**]** Basic SQL parser
**-**[**]** WHERE clause support

### Phase **4**

**-**[**]** JOIN operations
**-**[**]** Transactions
**-**[**]** Write**-**ahead logging

## Learning Resources

**-** Database Systems **Concepts**(Silberschatz**)**
**-** Architecture of a Database **System**(Hellerstein et al**.**)
**-** SQLite source code
**-** PostgreSQL documentation
