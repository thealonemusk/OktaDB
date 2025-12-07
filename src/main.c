#include <stdio.h>
#include <string.h>
#include "db_core.h"
#include "utility.h"

// Callback for SELECT * WHERE to print value
static void print_value_cb(const char *key, const char *value, void *ctx) {
    (void)key; // key not needed for simple output
    (void)ctx;
    if (value) {
        printf("%s\n", value);
    }
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

    char command[MAX_VALUE_LEN];
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];

    while (1) {
        printf("oktadb> ");
        fflush(stdout);

        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }

        // Remove newline
        command[strcspn(command, "\n")] = 0;
        // Skip empty
        if (strlen(command) == 0) {
            continue;
        }

        // EXIT
        if (oktadb_strcasecmp(command, "EXIT") == 0 ||
            oktadb_strcasecmp(command, "QUIT") == 0 ||
            oktadb_strcasecmp(command, "CLOSE") == 0) {
            break;
        }

        // HELP
        if (oktadb_strcasecmp(command, "HELP") == 0) {
            print_help();
            continue;
        }

        // INSERT / ADD
        if (oktadb_strncasecmp(command, "INSERT ", 7) == 0 ||
            oktadb_strncasecmp(command, "ADD ", 4) == 0) {
            const char *cmd_ptr = oktadb_strncasecmp(command, "INSERT ", 7) == 0 ? command + 7 : command + 4;
            if (sscanf(cmd_ptr, "%127s %255s", key, value) == 2) {
                int status = db_insert(db, key, value);
                if (status == STATUS_OK) {
                    printf("OK: Inserted key '%s'\n", key);
                } else if (status == STATUS_EXISTS) {
                    fprintf(stderr, "Error: Key '%s' already exists\n", key);
                } else if (status == STATUS_FULL) {
                    fprintf(stderr, "Error: Database is full\n");
                } else {
                    fprintf(stderr, "Error: Failed to insert key '%s'\n", key);
                }
            } else {
                fprintf(stderr, "Error: Invalid syntax. Use: INSERT <key> <value>\n");
            }
            continue;
        }

        // UPDATE
        if (oktadb_strncasecmp(command, "UPDATE ", 7) == 0) {
            if (sscanf(command + 7, "%127s %255s", key, value) == 2) {
                int status = db_update(db, key, value);
                if (status == STATUS_OK) {
                    printf("OK: Updated key '%s'\n", key);
                } else if (status == STATUS_NOT_FOUND) {
                    fprintf(stderr, "Error: Key '%s' not found\n", key);
                } else {
                    fprintf(stderr, "Error: Failed to update key '%s'\n", key);
                }
            } else {
                fprintf(stderr, "Error: Invalid syntax. Use: UPDATE <key> <value>\n");
            }
            continue;
        }

        // SELECT
        if (oktadb_strncasecmp(command, "SELECT ", 7) == 0) {
            const char *rest = command + 7;
            while (*rest == ' ') rest++;
            // SELECT *
            if (strcmp(rest, "*") == 0) {
                db_list(db);
                continue;
            }
            // SELECT * WHERE key = <key>
            char where_key[128];
            char where_val[256];
            if (sscanf(rest, "* WHERE %127s = %255s", where_key, where_val) == 2) {
                if (strcmp(where_key, "key") == 0) {
                    db_select_where(db, where_val, print_value_cb, NULL);
                } else {
                    fprintf(stderr, "Unsupported WHERE field: %s\n", where_key);
                }
                continue;
            }
            // SELECT <key>
            char select_key[128];
            if (sscanf(rest, "%127s", select_key) == 1) {
                const char *val = db_get(db, select_key);
                if (val) {
                    printf("%s\n", val);
                } else {
                    fprintf(stderr, "Key not found: %s\n", select_key);
                }
                continue;
            }
            fprintf(stderr, "Error: Invalid SELECT syntax\n");
            continue;
        }

        // LIST / LS
        if (oktadb_strcasecmp(command, "LIST") == 0 ||
            oktadb_strcasecmp(command, "LS") == 0) {
            db_list(db);
            continue;
        }

        // GET / FETCH (legacy)
        if (oktadb_strncasecmp(command, "GET ", 4) == 0 ||
            oktadb_strncasecmp(command, "FETCH ", 6) == 0) {
            const char *cmd_ptr = oktadb_strncasecmp(command, "GET ", 4) == 0 ? command + 4 : command + 6;
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

        // DELETE / DEL
        if (oktadb_strncasecmp(command, "DELETE ", 7) == 0 ||
            oktadb_strncasecmp(command, "DEL ", 4) == 0) {
            const char *cmd_ptr = oktadb_strncasecmp(command, "DELETE ", 7) == 0 ? command + 7 : command + 4;
            if (sscanf(cmd_ptr, "%127s", key) == 1) {
                int status = db_delete(db, key);
                if (status == STATUS_OK) {
                    printf("OK: Deleted key '%s'\n", key);
                } else if (status == STATUS_NOT_FOUND) {
                    fprintf(stderr, "Error: Key not found '%s'\n", key);
                } else {
                    fprintf(stderr, "Error: Failed to delete key '%s'\n", key);
                }
            } else {
                fprintf(stderr, "Error: Invalid syntax. Use: DELETE <key>\n");
            }
            continue;
        }

        // CLS / CLEAR
        if (oktadb_strcasecmp(command, "CLS") == 0 ||
            oktadb_strcasecmp(command, "CLEAR") == 0) {
            clear_screen();
            continue;
        }

        // Unknown command
        fprintf(stderr, "Unknown command: %s\n", command);
        fprintf(stderr, "Type 'HELP' for available commands\n");
    }

    db_close(db);
    printf("\nDatabase closed. Goodbye!\n");
    return 0;
}