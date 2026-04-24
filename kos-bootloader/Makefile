# KOS Bootloader - Root Makefile
# ============================================================================
# This is the main entry point for building the KOS bootloader
# All build logic is in build/Makefile
# ============================================================================

BUILD_DIR := build

.PHONY: all clean iso qemu debug run_kernel help

# Forward all targets to build/Makefile
all clean iso qemu debug run_kernel help:
	$(MAKE) -C $(BUILD_DIR) $@
