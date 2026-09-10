CC := gcc

TARGET := microhttps

SRC_DIR := src
INC_DIR := include
BUILD_DIR := build

CFLAGS := \
	-std=c17 \
	-D_POSIX_C_SOURCE=200809L \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Wconversion \
	-Wshadow \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-g \
	-I$(INC_DIR)

SRC := $(wildcard $(SRC_DIR)/*.c)

OBJ := $(patsubst \
	$(SRC_DIR)/%.c, \
	$(BUILD_DIR)/%.o, \
	$(SRC))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
