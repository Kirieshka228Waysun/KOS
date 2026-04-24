# KOS Bootloader - Makefile
# ============================================================================
# Build system for multi-stage bootloader (Stage1 + Stage2)
# Target: i686, BIOS boot, ISO9660 filesystem
# ============================================================================

# Configuration
include config.mk

# Directories (relative to project root)
PROJECT_ROOT  := ..
BUILD_DIR       := .
SRC_DIR         := $(PROJECT_ROOT)/src
BINARIES_DIR    := $(PROJECT_ROOT)/binaries
STAGE1_DIR      := $(SRC_DIR)/stage1
STAGE2_DIR      := $(SRC_DIR)/stage2
TOOLS_DIR       := $(SRC_DIR)/tools
DOCS_DIR        := $(PROJECT_ROOT)/docs

# Output files
STAGE1_BIN      := $(BINARIES_DIR)/stage1.bin
STAGE2_ELF      := $(BUILD_DIR)/stage2.elf
STAGE2_BIN      := $(BINARIES_DIR)/stage2.sys
KERNEL_BIN      := $(BINARIES_DIR)/boot32.sys
SERIAL_COM      := $(BINARIES_DIR)/SERIAL.COM
VGA_COM         := $(BINARIES_DIR)/VGA.COM
FILEINFO_COM    := $(BINARIES_DIR)/fileinfo.com
ISO_IMAGE       := KOS.iso

# Tools
NASM            := nasm
GCC             := gcc
LD              := ld
OBJCOPY         := objcopy
GENISOIMAGE     := genisoimage
QEMU            := qemu-system-i386

# NASM flags
NASM_FLAGS      := -f bin -i $(STAGE1_DIR)/

# GCC flags for freestanding 32-bit code
GCC_FLAGS       := -m32 \
                   -ffreestanding \
                   -fno-builtin \
                   -fno-stack-protector \
                   -nostdlib \
                   -nodefaultlibs \
                   -Wall \
                   -Wextra \
                   -Werror \
                   -pedantic \
                   -O2 \
                   -I$(STAGE2_DIR)/include \
                   -I$(STAGE2_DIR)/drivers \
                   -I$(STAGE2_DIR)/lib \
                   -I$(STAGE2_DIR)/iso9660 \
                   -I$(STAGE2_DIR)/boot_modes

# Linker flags
LD_FLAGS        := -m elf_i386 \
                   -T $(BUILD_DIR)/linker.ld \
                   -nostdlib

# Source files
STAGE1_SRCS     := $(STAGE1_DIR)/boot_sector.asm
STAGE2_SRCS_C   := $(wildcard $(STAGE2_DIR)/*.c) \
                   $(wildcard $(STAGE2_DIR)/drivers/*.c) \
                   $(wildcard $(STAGE2_DIR)/lib/*.c) \
                   $(wildcard $(STAGE2_DIR)/iso9660/*.c) \
                   $(wildcard $(STAGE2_DIR)/boot_modes/*.c) \
                   $(wildcard $(STAGE2_DIR)/boot_modes/recovery/*.c)

STAGE2_OBJS     := $(patsubst $(STAGE2_DIR)/%.c,$(BUILD_DIR)/%.o,$(STAGE2_SRCS_C))

# Test kernel sources
KERNEL_SRC      := $(TOOLS_DIR)/test_kernel.c

# Phony targets
.PHONY: all clean iso qemu debug run_serial help

# Default target
all: $(BINARIES_DIR) $(STAGE1_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	@echo "============================================"
	@echo "KOS Bootloader build complete!"
	@echo "============================================"
	@echo "Stage1: $(STAGE1_BIN)"
	@ls -la $(STAGE1_BIN)
	@echo "Stage2: $(STAGE2_BIN)"
	@ls -la $(STAGE2_BIN)
	@echo "Kernel: $(KERNEL_BIN)"
	@ls -la $(KERNEL_BIN)

# Create directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BINARIES_DIR):
	@mkdir -p $(BINARIES_DIR)

# Build Stage 1 (boot sector)
$(STAGE1_BIN): $(STAGE1_SRCS) | $(BINARIES_DIR)
	@echo "[NASM] Building $@"
	$(NASM) $(NASM_FLAGS) -o $@ $(STAGE1_SRCS)
	@# Verify size is exactly 512 bytes
	@SIZE=$$(stat -c%s $@); \
	if [ $$SIZE -ne 512 ]; then \
		echo "ERROR: Stage1 must be exactly 512 bytes, got $$SIZE"; \
		exit 1; \
	fi
	@# Verify boot signature
	@tail -c 2 $@ | od -An -tx1 | grep -q "55 aa" || \
		(echo "ERROR: Invalid boot signature"; exit 1)
	@echo "[OK] Stage1: 512 bytes with valid signature"

# Build Stage 2 object files
$(BUILD_DIR)/%.o: $(STAGE2_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "[GCC]  Compiling $<"
	$(GCC) $(GCC_FLAGS) -c -o $@ $<

# Build Stage 2 ELF
$(STAGE2_ELF): $(STAGE2_OBJS) | $(BUILD_DIR)
	@echo "[LD]   Linking $@"
	$(LD) $(LD_FLAGS) -o $@ $(STAGE2_OBJS)

# Convert Stage 2 to raw binary
$(STAGE2_BIN): $(STAGE2_ELF) | $(BINARIES_DIR)
	@echo "[OBJ]  Converting to raw binary"
	$(OBJCOPY) -O binary -S $(STAGE2_ELF) $(STAGE2_BIN)
	@echo "[OK] Stage2: $$(stat -c%s $(STAGE2_BIN)) bytes"

# Build test kernel
$(KERNEL_BIN): $(KERNEL_SRC) | $(BINARIES_DIR)
	@echo "[GCC]  Building test kernel"
	$(GCC) $(GCC_FLAGS) -nostartfiles -o $(BUILD_DIR)/kernel.elf $<
	$(OBJCOPY) -O binary -S $(BUILD_DIR)/kernel.elf $(KERNEL_BIN)
	@echo "[OK] Kernel: $$(stat -c%s $(KERNEL_BIN)) bytes"

# Build SERIAL.COM test program
$(SERIAL_COM): $(TOOLS_DIR)/serial_test.asm | $(BINARIES_DIR)
	@echo "[NASM] Building SERIAL.COM"
	$(NASM) -f bin -o $@ $(TOOLS_DIR)/serial_test.asm

# Build VGA.COM test program  
$(VGA_COM): $(TOOLS_DIR)/vga_test.asm | $(BINARIES_DIR)
	@echo "[NASM] Building VGA.COM"
	$(NASM) -f bin -o $@ $(TOOLS_DIR)/vga_test.asm

# Build fileinfo.COM utility
$(FILEINFO_COM): $(TOOLS_DIR)/fileinfo.asm | $(BINARIES_DIR)
	@echo "[NASM] Building fileinfo.COM"
	$(NASM) -f bin -o $@ $(TOOLS_DIR)/fileinfo.asm

# Create bootable ISO image
iso: all $(SERIAL_COM) $(VGA_COM) $(FILEINFO_COM)
	@echo "[ISO]  Creating bootable ISO..."
	@mkdir -p $(BUILD_DIR)/iso_root/boot
	@cp $(STAGE1_BIN) $(BUILD_DIR)/iso_root/boot/stage1.bin
	@cp $(STAGE2_BIN) $(BUILD_DIR)/iso_root/boot/stage2.sys
	@cp $(KERNEL_BIN) $(BUILD_DIR)/iso_root/boot/boot32.sys
	@cp $(SERIAL_COM) $(BUILD_DIR)/iso_root/boot/SERIAL.COM
	@cp $(VGA_COM) $(BUILD_DIR)/iso_root/boot/VGA.COM
	@cp $(FILEINFO_COM) $(BUILD_DIR)/iso_root/boot/fileinfo.COM
	@echo "KOS Bootloader" > $(BUILD_DIR)/iso_root/README.TXT
	@echo "Version 0.1-alpha" >> $(BUILD_DIR)/iso_root/README.TXT
	$(GENISOIMAGE) -quiet \
		-o $(ISO_IMAGE) \
		-b boot/stage1.bin \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		-R -J \
		-V "KOS_BOOT" \
		$(BUILD_DIR)/iso_root
	@echo "[OK] Created $(ISO_IMAGE)"
	@ls -la $(ISO_IMAGE)

# Run in QEMU
qemu: iso
	@echo "[QEMU] Starting emulator..."
	$(QEMU) \
		-cdrom $(ISO_IMAGE) \
		-m 128M \
		-boot d \
		-serial stdio \
		-no-reboot \
		-no-shutdown

# Run with GDB debugging
debug: iso
	@echo "[QEMU] Starting emulator with GDB stub on port 1234..."
	@echo "In another terminal, run: gdb -ex 'target remote localhost:1234'"
	$(QEMU) \
		-cdrom $(ISO_IMAGE) \
		-m 128M \
		-boot d \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-s -S

# Run without ISO (direct kernel boot for testing)
run_kernel: $(KERNEL_BIN)
	@echo "[QEMU] Testing kernel directly..."
	$(QEMU) \
		-kernel $(KERNEL_BIN) \
		-m 64M \
		-serial stdio

# Clean build artifacts
clean:
	@echo "[CLEAN] Removing build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -rf $(BINARIES_DIR)
	rm -f $(ISO_IMAGE)
	@echo "[OK] Clean complete"

# Help target
help:
	@echo "KOS Bootloader Build System"
	@echo "==========================="
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build all components (default)"
	@echo "  iso       - Create bootable ISO image"
	@echo "  qemu      - Run in QEMU emulator"
	@echo "  debug     - Run in QEMU with GDB stub"
	@echo "  clean     - Remove all build artifacts"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Output files:"
	@echo "  $(STAGE1_BIN)  - Stage 1 bootloader (512 bytes)"
	@echo "  $(STAGE2_BIN)  - Stage 2 loader (protected mode)"
	@echo "  $(KERNEL_BIN)  - Test kernel"
	@echo "  $(ISO_IMAGE)   - Bootable ISO image"
