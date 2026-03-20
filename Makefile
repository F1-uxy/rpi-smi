CC=gcc
CFLAGS = -g -Wall -O3 -march=native -std=c11 -D_POSIX_C_SOURCE=199309L
SMIFLAGS = -DSMI_COLOURFULL_ERRORS -DSMI_VERBOSE
LDFLAGS = -lm

SRC_DIR = src
DEP_DIR = deps
BUILD_DIR = build

HELPERS = $(SRC_DIR)/gpio.c $(SRC_DIR)/smi.c $(SRC_DIR)/dma.c $(SRC_DIR)/sram.c
SOURCES = $(SRC_DIR)/main.c $(HELPERS) 
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = $(BUILD_DIR)/smi

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(SMIFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SMIFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)/*