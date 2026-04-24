# KOS Bootloader - Build Configuration
# ============================================================================

# Target architecture
ARCH := i686

# Memory layout
STAGE1_LOAD_ADDR  := 0x7C00
STAGE2_LOAD_ADDR  := 0x7E00
KERNEL_LOAD_ADDR  := 0x10000
STACK_ADDR        := 0x90000

# VGA settings
VGA_MODE          := 0x12
VGA_WIDTH         := 640
VGA_HEIGHT        := 480
VGA_BPP           := 4

# Serial settings
SERIAL_PORT       := 0x3F8
SERIAL_BAUD       := 9600

# ISO settings
ISO_VOLUME_ID     := KOS_BOOT
ISO_SECTOR_SIZE   := 2048

# Debug settings
DEBUG_SERIAL      := 1
DEBUG_VGA         := 1
