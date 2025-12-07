# Makefile for OktaDB
# Supports Linux and macOS

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Isrc
LDFLAGS = 
DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -O2 -DNDEBUG

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
TARGET = $(BIN_DIR)/oktadb

# Find all .c files in src directory
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean debug release run rebuild test

all: release

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

release: CFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	@echo "Linking..."
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR) $(BIN_DIR) test_runner test_runner.exe
	@echo "Clean complete"

rebuild: clean release

run: $(TARGET)
	@echo "Running OktaDB..."
	./$(TARGET) test.db

test:
	$(CC) $(CFLAGS) -o test_runner tests/test_main.c tests/test_utility.c tests/test_select.c src/*.c -I src
	./test_runner

install: release
	@echo "Installing OktaDB..."
	@mkdir -p /usr/local/bin
	@cp $(TARGET) /usr/local/bin/oktadb
	@echo "Installation complete"

uninstall:
	@echo "Uninstalling OktaDB..."
	@rm -f /usr/local/bin/oktadb
	@echo "Uninstallation complete"
