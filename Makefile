ifneq (,$(shell which x86_64-elf-gcc 2>/dev/null))
	CROSS ?= x86_64-elf-
else
	CROSS ?=
endif

CC   := $(CROSS)gcc
LD   := $(CROSS)ld
AS   := $(CROSS)gcc
QEMU := qemu-system-x86_64

BUILD    := build
ISO_DIR  := $(BUILD)/iso
GRUB_DIR := $(ISO_DIR)/boot/grub
KERNEL   := $(BUILD)/kernel.elf
ISO      := $(BUILD)/microkernel.iso
QEMU_FLAGS ?=

CFLAGS  := -m64 -ffreestanding -fno-pie -fno-stack-protector \
           -fno-builtin -mno-red-zone -mno-sse -mno-mmx -mno-avx \
           -mno-80387 -fno-asynchronous-unwind-tables \
           -Wall -Wextra -O2 -Iinclude
ASFLAGS := -m64 -c
LDFLAGS := -T linker.ld -nostdlib -m elf_x86_64 -z max-page-size=0x1000


C_SRCS := $(shell find . -name '*.c' -not -path './$(BUILD)/*' -not -path './.git/*' 2>/dev/null)
S_SRCS := $(shell find . -name '*.S' -not -path './$(BUILD)/*' -not -path './.git/*' 2>/dev/null)

# ./foo/bar.c -> build/foo/bar.o
C_OBJS := $(patsubst ./%.c,$(BUILD)/%.o,$(C_SRCS))
S_OBJS := $(patsubst ./%.S,$(BUILD)/%.o,$(S_SRCS))
OBJS   := $(C_OBJS) $(S_OBJS)

.PHONY: all clean iso run run-kernel

all: $(KERNEL)

$(KERNEL): $(OBJS) linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) $< -o $@

iso: $(ISO)

$(ISO): $(KERNEL) grub.cfg
	@mkdir -p $(GRUB_DIR)
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.elf
	cp grub.cfg  $(GRUB_DIR)/grub.cfg
	grub-mkrescue -o $(ISO) $(ISO_DIR)

run: $(ISO)
	$(QEMU) -cdrom $(ISO) -serial stdio -m 512M -no-reboot $(QEMU_FLAGS)

run-kernel: $(KERNEL)
	$(QEMU) -kernel $(KERNEL) -serial stdio -m 512M -no-reboot $(QEMU_FLAGS)

clean:
	rm -rf $(BUILD)
