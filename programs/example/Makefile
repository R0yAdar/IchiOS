CC := gcc
LD := ld

CFLAGS := -std=gnu99 -ffreestanding -m64 -mno-red-zone -fno-builtin \
          -nostdlib -fno-stack-protector -msoft-float -Wall -O2 \
          -nostdinc -I. -I../../libc

LDFLAGS := -n -T user.ld

BUILD_DIR := ../build/programs
LIBC_DIR  := ../../libc

APP_SRCS  := $(wildcard *.c)
LIBC_SRCS := $(wildcard $(LIBC_DIR)/*.c)

APP_OBJS  := $(patsubst %.c, $(BUILD_DIR)/obj/%.o, $(APP_SRCS))
LIBC_OBJS := $(patsubst $(LIBC_DIR)/%.c, $(BUILD_DIR)/obj/libc/%.o, $(LIBC_SRCS))

TARGET := $(BUILD_DIR)/example.elf

all: $(TARGET)

$(TARGET): $(APP_OBJS) $(LIBC_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/obj/libc/%.o: $(LIBC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)/obj $(TARGET)