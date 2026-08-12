SRC_DIR := src
BUILD_DIR := build
TARGET := $(BUILD_DIR)/main

# Compiler and safety warning flags
CC := cc
CFLAGS := $(shell cat compile_flags.txt) -g

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TEST_SRCS := tests/test_parser.c tests/munit.c src/parser.c
TEST_TARGET := build/test_parser

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_SRCS) -o $(TEST_TARGET)

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: clean
.PHONY: test
