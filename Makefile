SRC_DIR := src
BUILD_DIR := build
TARGET := $(BUILD_DIR)/main

# Compiler and safety warning flags
CC := cc
CFLAGS := $(shell cat compile_flags.txt) -g

$(TARGET): $(BUILD_DIR)/main.o
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: clean
