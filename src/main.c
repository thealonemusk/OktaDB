#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "db_core.h"
#include "utility.h"

// Function to print help documentation
void print_help() {
    printf("OktaDB - A learning database implementation\n");
    printf("Usage:\n");
    printf("  INSERT/ADD <key> <value>  - Insert a key-value pair\n");
    printf("  GET/FETCH <key>           - Retrieve value by key\n");
    printf("  DELETE <key>              - Delete a key-value pair\n");
    printf("  UPDATE <key> <value>      - Update a key-value pair\n");
    printf("  LIST                      - List all keys\n");
    printf("  HELP                      - Show this help\n");
    printf("  CLS                       - Clear the screen\n");
    printf("  EXIT/QUIT/CLOSE           - Exit the program\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: Database file not specified\n");
        fprintf(stderr, "Usage: %s <database_file>\n", argv[0]);
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
        if (strcasecmp(command, "EXIT") == 0 || strcasecmp(command, "QUIT") == 0 || strcasecmp(command, "CLOSE") == 0) {
            break;
        }

        // HELP command
        if (strcasecmp(command, "HELP") == 0) {
            print_help();
            continue;
        }

        // INSERT command
        if (strncasecmp(command, "INSERT ", 7) == 0 || strncasecmp(command, "ADD ", 4) == 0) {
            const char *cmd_ptr = (strncasecmp(command, "INSERT ", 7) == 0) ? command + 7 : command + 4;
            if (sscanf(cmd_ptr, "%127s %255s", key, value) == 2) {
                if (db_insert(db, key, value) == STATUS_OK) {
                    printf("OK: Inserted key '%s'\n", key);
                } else {
                    fprintf(stderr, "Error: Failed to insert key '%s'\n", key);
                }
            } else {
                fprintf(stderr, "Error: Invalid syntax. Use: INSERT <key> <value>\n");
            }
            continue;
        }

        // GET command
        if (strncasecmp(command, "GET ", 4) == 0 || strncasecmp(command, "FETCH ", 6) == 0) {
            const char *cmd_ptr = (strncasecmp(command, "GET ", 4) == 0) ? command + 4 : command + 6;
            if (sscanf(cmd_ptr, "%127s", key) == 1) {
                const char *result = db_get(db, key);
                if (result) {
                    printf("%s\n", result);
                } else {
                    fprintf(stderr, "Key not found: %s\n", key);
                }
            } else {
                fprintf(stderr, "Error: Invalid syntax. Use: GET <key>\n");
            }
            continue;
        }

        // DELETE command
        if (strncasecmp(command, "DELETE ", 7) == 0  || strncasecmp(command, "DEL ", 4) == 0) {
            const char *cmd_ptr = (strncasecmp(command, "DELETE ", 7) == 0) ? command + 7 : command + 4;
            if (sscanf(cmd_ptr, "%127s", key) == 1) {
                if (db_delete(db, key) == STATUS_OK) {
                    printf("OK: Deleted key '%s'\n", key);
                } else {
                    fprintf(stderr, "Error: Key not found '%s'\n", key);
                }
            } else {
                fprintf(stderr, "Error: Invalid syntax. Use: DELETE <key>\n");
            }
            continue;
        }

        // LIST command
        if (strcasecmp(command, "LIST") == 0 || strcasecmp(command, "LS") == 0) {
            db_list(db);
            continue;
        }

        // UPDATE command
        if (strncasecmp(command, "UPDATE ", 7) == 0) {
            if (sscanf(command + 7, "%127s %255s", key, value) == 2) {
                if (db_update(db, key, value) == STATUS_OK) {
                    printf("OK: Updated key '%s'\n", key);
                } else {
                    fprintf(stderr, "Error: Failed to update key'%s'. Key not found or invalid.\n", key);
                }
            } else {
                fprintf(stderr, "Error: Invalid syntax. Use: UPDATE <key> <value>\n");
            }
            continue;
        }
        
        // CLS command
        if (strcasecmp(command, "CLS") == 0) {
#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif
            continue;
        }
        fprintf(stderr, "Unknown command: %s\n", command);
        fprintf(stderr, "Type 'HELP' for available commands\n");
    }

    db_close(db);
    printf("\nDatabase closed. Goodbye!\n");
    return 0;
}