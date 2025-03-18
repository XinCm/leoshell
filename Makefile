.DEFAULT_GOAL := all
CC = gcc
CFLAGS = -Wall -Iinclude 
LDFLAGS = -lm -lreadline -ltermcap


SRC_DIR = ./src
BUILD_DIR = ./build
BIN_DIR = ./bin
TARGET = $(BIN_DIR)/lxsh

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d)
-include $(DEPS)

create_dirs:
		@mkdir -p $(BUILD_DIR) $(BIN_DIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
		$(CC) $(CFLAGS) -MMD -c $< -o $@

all: create_dirs $(TARGET)


clean:
		 rm -rf $(BIN_DIR) $(BUILD_DIR)


.PHONY: all clean create_dirs