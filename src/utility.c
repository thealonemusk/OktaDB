#include "utility.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Portable case-insensitive string comparison functions
#ifdef _WIN32
// Windows: Provide our own implementations to avoid conflicts with MinGW
int oktadb_strcasecmp(const char *s1, const char *s2) {
    if (!s1 || !s2) {
        if (!s1 && !s2) return 0;
        return s1 ? 1 : -1;
    }
    
    while (*s1 && *s2) {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0) return diff;
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

int oktadb_strncasecmp(const char *s1, const char *s2, size_t n) {
    if (n == 0) return 0;
    if (!s1 || !s2) {
        if (!s1 && !s2) return 0;
        return s1 ? 1 : -1;
    }
    
    // Compare up to n characters
    while (n > 0 && *s1 && *s2) {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0) return diff;
        s1++;
        s2++;
        n--;
    }
    
    // If we exhausted all n characters without finding a difference, strings match
    if (n == 0) return 0;
    
    // Otherwise, one string ended before n characters - compare remaining
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}
#else
// Unix/Linux has these in strings.h
#include <strings.h>
#endif

// Function to print help documentation
void print_help(void) {
    printf("OktaDB - A learning database implementation\n");
    printf("Usage:\n");
    printf("  INSERT/ADD <key> <value>  - Insert a key-value pair\n");
    printf("  GET/FETCH <key>           - Retrieve value by key\n");
    printf("  DELETE <key>              - Delete a key-value pair\n");
    printf("  UPDATE <key> <value>      - Update a key-value pair\n");
    printf("  LIST                      - List all keys\n");
    printf("  HELP                      - Show this help\n");
    printf("  CLS/CLEAR                 - Clear the screen\n");
    printf("  EXIT/QUIT/CLOSE           - Exit the program\n");
}

// Function to clear the screen in a cross-platform way
void clear_screen(void) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    DWORD cellCount;
    COORD homeCoords = { 0, 0 };

    if (hConsole == INVALID_HANDLE_VALUE) return;

    // Get the number of cells in the current buffer
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    cellCount = csbi.dwSize.X * csbi.dwSize.Y;

    // Fill the entire buffer with spaces
    if (!FillConsoleOutputCharacter(hConsole, (TCHAR)' ', cellCount, homeCoords, &count)) return;

    // Fill the entire buffer with the current colors and attributes
    if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count)) return;

    // Move the cursor home
    SetConsoleCursorPosition(hConsole, homeCoords);
#else
    // Unix/Linux/Mac - use ANSI escape codes
    printf("\033[2J\033[H");
    fflush(stdout);
#endif
}
