# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -Isrc
LDFLAGS = 

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Source files
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/storage/storage.c

# Object files
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

# Target executable
TARGET = $(BIN_DIR)/oktadb

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)/storage
	@mkdir -p $(BIN_DIR)

# Link
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete"

# Run the program
run: all
	./$(TARGET) test.db

# Debug build
debug: CFLAGS += -DDEBUG -O0
debug: clean all

# Release build
release: CFLAGS += -O3 -DNDEBUG
release: clean all

.PHONY: all clean run debug release directories