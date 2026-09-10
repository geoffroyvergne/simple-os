# SimpleOS

A minimalist operating system built from scratch, step by step.

- **Arch:** x86 (i686, 32-bit protected mode) first. May be extended later.
- **Boot:** custom 2-stage BIOS bootloader (no GRUB).
- **Kernel:** freestanding C.
- **Host:** Apple Silicon (ARM) macOS, cross-compiling with LLVM/clang, tested in QEMU (`qemu-system-i386`).

## Goal

A bootable OS with a terminal, a file system, and the ability to load and run
ELF binaries.

## Toolchain

Everything is installed via Homebrew. No GCC cross-compiler is required; we use
`clang` as a cross-compiler targeting `i686-elf` plus `ld.lld` and LLVM binutils.

```
brew install nasm qemu llvm lld
```

Paths are auto-detected in `toolchain.mk` (override any variable on the make
command line if your setup differs).

## Build & run

```
make            # build build/os.img
make run        # boot it in QEMU (graphical window)
make run-serial # boot headless, kernel log goes to the terminal via COM1
make debug      # boot with QEMU gdb stub on :1234, paused (see below)
make clean
```

### Debugging

`make debug` starts QEMU paused with a gdb stub on TCP 1234. Connect with a
gdb that can target i386 (e.g. `brew install i386-elf-gdb`) or lldb:

```
i386-elf-gdb build/kernel.elf -ex 'target remote :1234' -ex 'break kmain' -ex continue
```

## Disk image layout

| LBA (512-byte sectors) | Contents                       |
|------------------------|--------------------------------|
| 0                      | Stage 1 (MBR, 512 bytes)       |
| 1 .. 8                 | Stage 2 (up to 4 KiB)          |
| 9 ..                   | Kernel (flat binary)           |

Stage 1 (loaded by the BIOS at `0x7C00`) reads Stage 2 to `0x7E00` and jumps to
it in 16-bit real mode. Stage 2 loads the kernel to `0x10000`, enables A20,
installs a flat GDT, enters 32-bit protected mode, and jumps to the kernel.

## Roadmap

- [x] **Step 1 — Boot to a screen.** Custom bootloader, protected-mode entry,
      kernel prints to VGA text mode and COM1.
- [x] **Step 2 — Terminal output.** Real `printf`-style console: scrolling,
      cursor, colors, `kprintf`.
- [x] **Step 3 — Interrupts.** GDT reload from the kernel, IDT, PIC remap,
      exception handlers, PIT timer.
- [x] **Step 4 — Keyboard.** PS/2 keyboard driver, input line editing.
- [x] **Step 5 — Memory management.** Physical frame allocator (parse memory
      map), paging, `kmalloc`.
- [ ] **Step 6 — Storage + FS.** ATA PIO disk driver, a simple file system
      (custom read-only first, then read/write), VFS layer.
- [ ] **Step 7 — User mode.** Ring 3, TSS, syscalls (`int 0x80`).
- [ ] **Step 8 — ELF loader + processes.** Load static ELF executables from the
      FS into a user address space, `exec`, basic scheduler.
- [ ] **Step 9 — Shell.** A real terminal program running in user mode with
      built-in commands and the ability to launch ELF binaries.

Each step is a self-contained, buildable commit.
