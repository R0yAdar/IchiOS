.PHONY: all clean boot

# Tools and flags
NASM := nasm -f elf64
CC := gcc
CFLAGS := -std=gnu99 -ffreestanding -m64 -mno-red-zone -fno-builtin -nostdinc -Wall -Wextra -fno-pic -fno-pie -mcmodel=large
LDFLAGS := -nostdlib -no-pie
# -mcmodel=large
# Directory structure
SRC_DIR := src
BUILD_DIR := build

INCLUDE_DIRS := src/include src/drivers/include
CFLAGS += $(addprefix -I,$(INCLUDE_DIRS))

# Recursively gather source files
rwildcard = $(foreach d, $(wildcard $1*), $(call rwildcard, $d/, $2) $(filter $(subst *,%,$2), $d))

C_SRCS := $(call rwildcard, $(SRC_DIR)/, *.c)
ASM_SRCS := $(call rwildcard, $(SRC_DIR)/, *.asm)
SRC := $(C_SRCS) $(ASM_SRCS)

# Map source files to build object files
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.c.o, $(C_SRCS)) \
        $(patsubst $(SRC_DIR)/%.asm, $(BUILD_DIR)/%.asm.o, $(ASM_SRCS))

# Final image
BOOT_IMAGE := $(BUILD_DIR)/boot_image

# Default target
all: $(BOOT_IMAGE)

# Launch in QEMU
boot: $(BOOT_IMAGE)
	qemu-system-x86_64 -M q35 -no-reboot -no-shutdown -drive file=$<,format=raw,index=0,media=disk -serial stdio

# Link object files into ELF
$(BUILD_DIR)/linked.elf: $(OBJS)
	@mkdir -p $(dir $@)
	ld $(LDFLAGS) -T linker.ld -o $@ $^ 

# Convert ELF to flat binary
$(BOOT_IMAGE): $(BUILD_DIR)/linked.elf
	@mkdir -p $(dir $@)
	objcopy -O binary --only-section=.boot_sector --only-section=.boot_sign build/linked.elf build/boot_sector.bin
	objcopy -O binary --only-section=.stage2 build/linked.elf build/stage2.bin
	objcopy -O binary --only-section=.text --only-section=.data --only-section=.rodata build/linked.elf build/kernel.bin
	
	cat build/boot_sector.bin build/stage2.bin build/kernel.bin > build/raw_boot_image
	dd if=build/raw_boot_image of=build/boot_image bs=1M count=1 conv=sync

	mkdir -p build/rootfs
	echo "hello world" > build/rootfs/readme.txt

	genext2fs -b 10240 -d build/rootfs build/fs.ext2

	cat build/fs.ext2 >> build/boot_image

# Compile assembly
$(BUILD_DIR)/%.asm.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) $< -o $@

# Compile C
$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	$(RM) -r $(BUILD_DIR)
