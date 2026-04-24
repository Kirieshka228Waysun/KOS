# KOS Bootloader - Testing Guide

## Prerequisites

- QEMU system emulator (`qemu-system-i386`)
- NASM assembler (v2.15+)
- GCC cross-compiler or native GCC with `-m32`
- GNU Make
- genisoimage (for ISO creation)

## Quick Start

### Build Everything

```bash
cd /workspace/kos-bootloader
make clean
make all
```

### Create Bootable ISO

```bash
make iso
```

This creates `KOS.iso` in the project root directory.

### Run in QEMU

```bash
make qemu
```

Or manually:

```bash
qemu-system-i386 \
    -cdrom KOS.iso \
    -m 128M \
    -boot d \
    -serial stdio \
    -no-reboot \
    -no-shutdown
```

## Debug Mode

### Run with GDB Stub

```bash
make debug
```

Then in another terminal:

```bash
gdb
(gdb) target remote localhost:1234
(gdb) symbol-file build/stage2.elf
(gdb) break _start
(gdb) continue
```

### QEMU Logging

Enable detailed QEMU logging:

```bash
qemu-system-i386 \
    -cdrom KOS.iso \
    -m 128M \
    -boot d \
    -serial stdio \
    -d int,cpu_reset,guest_errors \
    -D qemu.log
```

## Test Scenarios

### 1. Normal Boot Test

**Expected behavior:**
1. Stage 1 loads and displays "KOS Boot v0.1"
2. Stage 2 loads and shows boot menu
3. Press 'G' or Enter for Normal boot
4. Kernel loads and displays "KOS KERNEL LOADED"
5. Serial output shows boot progress

**QEMU command:**
```bash
qemu-system-i386 -cdrom KOS.iso -m 128M -boot d -serial stdio
```

**Check serial output for:**
```
[KOS] Stage 2 loaded successfully
[BOOT] Starting normal boot sequence...
[BOOT] Searching for /boot/boot32.sys...
[BOOT] Loading kernel...
[BOOT] Kernel loaded successfully
[BOOT] Jumping to kernel...
```

### 2. Recovery Console Test

**Steps:**
1. Boot from ISO
2. Press 'R' at boot menu
3. Type `help` and press Enter
4. Type `ver` and press Enter
5. Type `info` and press Enter
6. Type `mem` and press Enter
7. Type `reboot` to restart

**Expected commands:**
```
KOS> help
Available commands:
  ver      - Show version information
  info     - Show system information
  help     - Show this help message
  exec     - Execute .COM file: exec <file>
  mem      - Show memory map
  reboot   - Reboot the system

KOS> ver
KOS Bootloader v0.1-alpha
Build: [date] [time]
Target: i686, BIOS
```

### 3. Memory Test

**Steps:**
1. Boot from ISO
2. Press 'M' at boot menu
3. Watch memory test progress
4. Press ESC to abort or wait for completion

**Expected output:**
```
KOS Memory Test
v0.1-alpha

Testing memory range:
  Start: 0x00100000 (1 MB)
  End:   0x00800000 (8 MB)

Progress bar and pattern info...
```

### 4. VGA Mode Test

**Run VGA.COM from recovery console:**
```
KOS> exec boot/VGA.COM
```

**Expected:** Screen switches to graphics mode with test pattern.

### 5. Serial Loopback Test

If you have serial loopback hardware:

```bash
# In one terminal (QEMU)
qemu-system-i386 -cdrom KOS.iso -m 64M -serial pty

# Note the PTY path, then connect
screen /dev/pts/X

# In recovery console, run:
KOS> exec boot/SERIAL.COM
```

## Validation Checklist

Before considering the build complete, verify:

- [ ] `stage1.bin` is exactly 512 bytes
- [ ] `stage1.bin` ends with 0x55AA signature
- [ ] ISO mounts correctly on host OS
- [ ] `/boot/boot32.sys` visible in mounted ISO
- [ ] Boot menu appears in QEMU
- [ ] Normal boot loads kernel
- [ ] Recovery console accepts commands
- [ ] Memory test runs without crashes
- [ ] Serial output shows boot messages
- [ ] No compiler warnings with `-Wall -Wextra -Werror`

## Troubleshooting

### Problem: "Booting from CD-ROM..." hangs

**Solution:** Check that stage1.bin has valid boot signature:
```bash
hexdump -C binaries/stage1.bin | tail -1
# Should show: 55 aa at the end
```

### Problem: Triple fault on protected mode switch

**Solution:** Verify GDT is correctly loaded:
```bash
make debug
# Check GDT entries in debugger
```

### Problem: No serial output

**Solution:** Ensure QEMU is started with `-serial stdio` and check UART initialization in `serial.c`.

### Problem: Keyboard not responding

**Solution:** Check PS/2 controller initialization and verify scancode handling in `keyboard.c`.

### Problem: ISO won't mount

**Solution:** Verify genisoimage parameters:
```bash
genisoimage -o test.iso -b boot/stage1.bin -no-emul-boot \
    -boot-load-size 4 -boot-info-table -R -J iso_root/
```

## Performance Testing

### Boot Time Measurement

```bash
time qemu-system-i386 -cdrom KOS.iso -m 128M -boot d \
    -display none -serial null
```

### Memory Usage

Check stage2 size:
```bash
ls -la binaries/stage2.sys
# Should be < 32KB for efficient loading
```

## Known Limitations

1. **LBA48**: Not fully implemented; disks >8GB may have issues
2. **.COM Execution**: Stub implementation only
3. **USB Keyboard**: Not supported (PS/2 only)
4. **UEFI**: BIOS-only bootloader
5. **ACPI**: Not parsed or used

## Extending Tests

To add new test programs:

1. Create `.asm` or `.c` file in `src/tools/`
2. Add build rule to `Makefile`
3. Copy to ISO in `iso` target
4. Test from recovery console with `exec`

Example test program structure:
```asm
[BITS 16]
[ORG 0x0000]

start:
    ; Your code here
    int 0x20    ; Return to caller
```
