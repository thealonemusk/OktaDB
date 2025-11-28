# OktaDB Storage Format

OktaDB uses a custom binary format with fixed-size pages, B+Tree indexing, and Write-Ahead Logging (WAL).

## File Structure

The database file consists of a sequence of 4KB (4096 bytes) pages.
Page 0 is the Root Page (and also contains metadata).

## Page Format

Each page starts with a header.

### Common Header (All Pages)
| Offset | Size | Description |
|--------|------|-------------|
| 0      | 1    | Page Type (0=Internal, 1=Leaf) |
| 1      | 1    | Is Root (0=No, 1=Yes) |
| 2      | 4    | Parent Page ID |

### Leaf Node
Stores actual Key-Value pairs.

**Header:**
| Offset | Size | Description |
|--------|------|-------------|
| 6      | 4    | Number of Cells |

**Body:**
Array of Cells. Each Cell:
| Size | Description |
|------|-------------|
| 128  | Key (Null-terminated string) |
| 256  | Value (Null-terminated string) |

Total Cell Size: 384 bytes.
Max Cells per Page: 10.

### Internal Node
Stores Keys and Child Pointers.

**Header:**
| Offset | Size | Description |
|--------|------|-------------|
| 6      | 4    | Number of Keys |
| 10     | 4    | Rightmost Child Page ID |

**Body:**
Array of Cells. Each Cell:
| Size | Description |
|------|-------------|
| 4    | Child Page ID |
| 128  | Key (Max key in child subtree) |

## Write-Ahead Log (WAL)

All modifications are first written to `<db_filename>.wal`.
The WAL consists of a sequence of frames.

**Frame Format:**
| Size | Description |
|------|-------------|
| 4    | Page Number |
| 4    | Checksum |
| 4096 | Page Data |

During `db_open`, if a WAL file exists, it is checkpointed (replayed) into the main database file.
