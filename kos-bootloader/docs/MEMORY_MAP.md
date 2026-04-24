# KOS Bootloader - Memory Map Documentation

## Overview

This document describes the memory layout used by the KOS bootloader and kernel.

## Real Mode Memory Map (Stage 1)

```
Address Range         Size      Description
-------------------------------------------------------------------------------
0x00000 - 0x003FF     1 KB      Real Mode IVT (Interrupt Vector Table)
0x00400 - 0x004FF     256 B     BIOS Data Area (BDA)
0x00500 - 0x7BFF      ~30 KB    Free conventional memory
0x7C00 - 0x7DFF       512 B     Stage 1 Bootloader (loaded by BIOS)
0x7E00 - 0x8FFF       ~48 KB    Stage 2 Loader (loaded by Stage 1)
0x9000 - 0x9FFFF      ~352 KB   Free conventional memory / Stack
0xA0000 - 0xBFFFF     128 KB    VGA Framebuffer (Mode 0x12)
0xC0000 - 0xFFFFF     256 KB    ROM Area (Video BIOS, Motherboard BIOS, etc.)
```

### Stage 1 Specific Layout

| Address | Content |
|---------|---------|
| 0x7C00 | Boot sector entry point |
| 0x7C00+ | Stack (grows downward) |
| DS, ES, FS, GS | All set to 0x0000 |
| SS | 0x0000 |
| SP | 0x7C00 |

## Protected Mode Memory Map (Stage 2)

```
Address Range         Size      Description
-------------------------------------------------------------------------------
0x00000000 - 0x0009FFFF   640 KB    Low Memory (Conventional)
0x000A0000 - 0x000BFFFF   128 KB    VGA Linear Framebuffer
0x000C0000 - 0x000FFFFF   256 KB    High Memory Area (ROM)
0x00100000 - 0x008FFFFF   8 MB      Kernel + Bootloader Data
  0x00100000              -         Kernel load address
  0x00007E00              -         Stage 2 code (after switch)
0x00900000 - 0x009FFFFF   1 MB      Stack Area
0x00A00000+               -         Extended Memory (Free)
```

### Stage 2 Segment Layout

| Segment | Selector | Base | Limit | Access |
|---------|----------|------|-------|--------|
| Null    | 0x00     | 0    | 0     | None   |
| Code    | 0x08     | 0    | 4GB   | R/X, DPL 0 |
| Data    | 0x10     | 0    | 4GB   | R/W, DPL 0 |
| Video   | 0x18     | 0    | 4GB   | R/W, DPL 0 |

## GDT Structure

```c
struct gdt_entry {
    uint16_t limit_low;    // Bits 0-15 of limit
    uint16_t base_low;     // Bits 0-15 of base
    uint8_t  base_middle;  // Bits 16-23 of base
    uint8_t  access;       // Access flags
    uint8_t  granularity;  // Limit bits 16-19 + flags
    uint8_t  base_high;    // Bits 24-31 of base
};
```

### Access Byte Format

```
Bit 7: Present (1 = present)
Bit 6-5: Descriptor Privilege Level (0 = kernel)
Bit 4: S flag (1 = code/data segment)
Bit 3: Type (1 = code for code seg, 1 = writable for data seg)
Bit 2: Direction/Conforming
Bit 1: Readable (code) / Writable (data)
Bit 0: Accessed
```

### Granularity Byte Format

```
Bit 7-4: Limit bits 16-19
Bit 3: Granularity (1 = 4KB pages, 0 = 1 byte)
Bit 2: Default operand size (1 = 32-bit)
Bit 1: L flag (reserved)
Bit 0: AVL (available for software)
```

## Boot Parameters Structure

The bootloader places a `boot_params_t` structure at physical address **0x9000** before jumping to the kernel.

```c
#define BOOT_PARAMS_ADDR    0x9000

typedef struct {
    uint32_t magic;              // 0x4B4F5342 ("KOSB")
    uint32_t size;               // sizeof(boot_params_t)
    uint8_t  version[3];         // Version info
    // ... memory map, video info, etc.
} boot_params_t;
```

## Stack Configuration

| Mode | Stack Segment | Stack Pointer | Location |
|------|---------------|---------------|----------|
| Real (Stage 1) | 0x0000 | 0x7C00 | Just below bootloader |
| Protected (Stage 2) | 0x10 (data) | 0x90000 | 1MB boundary |

## VGA Framebuffer Details

For mode 0x12 (640×480×16):

- **Physical Address**: 0xA0000
- **Size**: 153,600 bytes (640 × 480 × 4 bits / 8)
- **Organization**: Packed pixel, 4 bits per pixel
- **Pitch**: 320 bytes per scanline (640 pixels / 2)
- **Total Scanlines**: 480

Pixel calculation:
```c
uint8_t* fb = (uint8_t*)0xA0000;
int offset = (y * 320) + (x / 2);
if (x % 2 == 0) {
    color = (fb[offset] >> 4) & 0x0F;  // Even pixel (high nibble)
} else {
    color = fb[offset] & 0x0F;          // Odd pixel (low nibble)
}
```

## Notes

1. **A20 Gate**: Must be enabled to access memory above 1MB
2. **Identity Mapping**: In protected mode, linear = physical addresses
3. **No Paging**: Paging is not enabled in bootloader stage
4. **Cache**: Caches are not explicitly managed in bootloader
