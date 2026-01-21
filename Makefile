CC=gcc
CFLAGS = -g -Wall -O0 -std=c11 -D_POSIX_C_SOURCE=199309L
LDFLAGS = -lm

SRC_DIR = src
DEP_DIR = deps
BUILD_DIR = build

HELPERS = $(SRC_DIR)/gpio.c $(SRC_DIR)/smi.c $(SRC_DIR)/dma.c $(SRC_DIR)/sram.c
SOURCES = $(SRC_DIR)/main.c $(HELPERS) 
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = $(BUILD_DIR)/smi

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)/*