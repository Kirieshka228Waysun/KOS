# KOS Bootloader - Production-Ready i686 Bootloader

## Overview

KOS Bootloader is a multi-stage bootloader for the KOS operating system (i686 architecture). It supports:

- **Multi-stage boot**: Stage1 (MBR/VBR) → Stage2 (ISO9660 parser) → Kernel loader
- **Three boot modes**: Normal, Recovery Console, MemTest
- **Low-level drivers**: PS/2 Keyboard, VGA Mode 0x12, Serial COM1

## Project Structure

```
kos-bootloader/
├── build/                      # Build configuration
│   ├── Makefile                # Main build script
│   ├── linker.ld               # Linker script for stage2
│   └── config.mk               # Configuration constants
├── src/
│   ├── stage1/                 # 16-bit real mode bootloader
│   │   ├── boot_sector.asm     # MBR/VBR entry point
│   │   ├── disk_io.asm         # Disk I/O routines
│   │   └── iso9660_stub.asm    # ISO9660 minimal parser
│   ├── stage2/                 # 32-bit protected mode loader
│   │   ├── main.c              # Entry point and boot menu
│   │   ├── iso9660/            # ISO9660 filesystem driver
│   │   ├── drivers/            # Hardware drivers
│   │   ├── boot_modes/         # Boot mode implementations
│   │   ├── lib/                # Standard library replacements
│   │   └── include/            # Header files
│   └── tools/                  # Build utilities
├── binaries/                   # Generated binaries
├── docs/                       # Documentation
└── README.md
```

## Quick Start

### Prerequisites

```bash
# Install toolchain (Ubuntu/Debian)
sudo apt-get install nasm gcc make qemu-system-x86 genisoimage

# Or build cross-compiler (recommended)
# See docs/CROSS_COMPILE.md for instructions
```

### Build

```bash
cd kos-bootloader
make all          # Build all components
make iso          # Create bootable ISO
make qemu         # Run in QEMU
```

### Debug

```bash
make debug        # QEMU with GDB stub
# In another terminal: gdb -ex "target remote localhost:1234"
```

## Boot Modes

| Mode | Key | Description |
|------|-----|-------------|
| 🟢 Normal | Enter | Load `/boot/boot32.sys` kernel |
| 🟡 Recovery | R | Recovery console with CLI |
| 🔵 MemTest | M | Memory test utility |

## Technical Specifications

- **Stage1**: 512 bytes, loads at 0x7C00
- **Stage2**: Loads at 0x7E00, switches to protected mode
- **Kernel**: Loads at 0x10000 (1MB)
- **Stack**: 0x90000 in protected mode
- **VGA**: Mode 0x12 (640×480×16, linear framebuffer at 0xA0000)

## License

MIT License
