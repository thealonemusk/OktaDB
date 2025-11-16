#include <stdio.h>
#include <stdlib.h>  // For memory allocation functions
#include <string.h>  // For strdup
#include <strings.h> // For strncasecmp and strcasecmp
#include "storage/storage.h"
#include "common/types.h"

// Define strdup for Windows if not already defined
#ifndef strdup
#define strdup _strdup
#endif

void print_help() {
    printf("OktaDB - A learning database implementation\n");
    printf("Usage:\n");
    printf("  INSERT <key> <value>  - Insert a key-value pair\n");
    printf("  GET <key>             - Retrieve value by key\n");
    printf("  DELETE <key>          - Delete a key-value pair\n");
    printf("  LIST                  - List all keys\n");
    printf("  HELP                  - Show this help\n");
    printf("  EXIT                  - Exit the program\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Error: Database file not specified\n");
        printf("Usage: %s <database_file>\n", argv[0]);
        return 1;
    }

    const char *db_file = argv[1];
    Database *db = db_open(db_file);
    
    if (!db) {
        fprintf(stderr, "Error: Could not open database file: %s\n", db_file);
        return 1;
    }

    printf("Database opened: %s\n", db_file);
    printf("Type 'HELP' for available commands\n\n");

    char command[256];
    char key[128];
    char value[256];

    while (1) {
        printf("oktadb> ");
        fflush(stdout);
        
        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }

        // Remove newline
        command[strcspn(command, "\n")] = 0;

        // Skip empty commands
        if (strlen(command) == 0) {
            continue;
        }

        // EXIT command
        if (strcasecmp(command, "EXIT") == 0 || strcasecmp(command, "QUIT") == 0) {
            break;
        }

        // HELP command
        if (strcasecmp(command, "HELP") == 0) {
            print_help();
            continue;
        }

        // INSERT command
        if (strncasecmp(command, "INSERT ", 7) == 0) {
            if (sscanf(command + 7, "%127s %255s", key, value) == 2) {
                if (db_insert(db, key, value) == STATUS_OK) {
                    printf("OK: Inserted key '%s'\n", key);
                } else {
                    printf("Error: Failed to insert key '%s'\n", key);
                }
            } else {
                printf("Error: Invalid syntax. Use: INSERT <key> <value>\n");
            }
            continue;
        }

        // GET command
        if (strncasecmp(command, "GET ", 4) == 0) {
            if (sscanf(command + 4, "%127s", key) == 1) {
                char *result = db_get(db, key);
                if (result) {
                    printf("%s\n", result);
                    free(result);
                } else {
                    printf("Key not found: %s\n", key);
                }
            } else {
                printf("Error: Invalid syntax. Use: GET <key>\n");
            }
            continue;
        }

        // DELETE command
        if (strncasecmp(command, "DELETE ", 7) == 0) {
            if (sscanf(command + 7, "%127s", key) == 1) {
                if (db_delete(db, key) == STATUS_OK) {
                    printf("OK: Deleted key '%s'\n", key);
                } else {
                    printf("Error: Key not found '%s'\n", key);
                }
            } else {
                printf("Error: Invalid syntax. Use: DELETE <key>\n");
            }
            continue;
        }

        // LIST command
        if (strcasecmp(command, "LIST") == 0) {
            db_list(db);
            continue;
        }

        printf("Unknown command: %s\n", command);
        printf("Type 'HELP' for available commands\n");
    }

    db_close(db);
    printf("\nDatabase closed. Goodbye!\n");
    return 0;
}