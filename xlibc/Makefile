CC := gcc
AR := ar
CFLAGS := -std=gnu99 -ffreestanding -m64 -mno-red-zone -fno-builtin -nostdlib -Wall

# Directories
SRC_DIR   := src
BUILD_DIR := ../build/xlibc

INCLUDE_DIRS := src/include
CFLAGS += $(addprefix -I,$(INCLUDE_DIRS))
# Files
# This finds all .c files inside xlibc/src
SRCS      := $(wildcard $(SRC_DIR)/*.c)
# This converts src/file.c into ../build/xlibc/file.o
OBJS      := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
LIB       := $(BUILD_DIR)/xlibc.a

.PHONY: all clean

all: $(LIB)

# Link the library using the archiver
$(LIB): $(OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $(OBJS)

# Compile .c files from src/ into .o files in build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)