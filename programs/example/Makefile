CC := gcc
LD := ld
CFLAGS := -std=gnu99 -ffreestanding -m64 -mno-red-zone -fno-builtin -nostdlib -Wall
LDFLAGS := -n -T user.ld

BUILD_DIR ?= ../build/programs

SRCS := $(wildcard *.c)
ELFS := $(patsubst %.c, $(BUILD_DIR)/%.elf, $(SRCS))

all: $(ELFS)

$(BUILD_DIR)/%.elf: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $(BUILD_DIR)/$*.o
	$(LD) $(LDFLAGS) $(BUILD_DIR)/$*.o -o $@

clean:
	rm -rf $(BUILD_DIR)