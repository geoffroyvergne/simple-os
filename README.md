# SimpleOS

A minimalist operating system built from scratch, step by step, for x86.

- **Arch:** i686 (32-bit protected mode). Architecture code is isolated under
  `kernel/arch/x86/` so another target can be added as a sibling.
- **Boot:** custom 2-stage BIOS bootloader (no GRUB).
- **Kernel:** freestanding C + a little assembly.
- **Host:** Apple Silicon (ARM) macOS, cross-compiling with LLVM/clang, run and
  debugged in QEMU (`qemu-system-i386`).

## Goal

A bootable OS with a terminal, a file system, and the ability to load and run
ELF binaries. **All three work today** (through Step 8); see the roadmap.

## Toolchain

Everything comes from Homebrew. No GCC cross-compiler is needed — `clang` is used
as a cross-compiler targeting `i686-elf`, with `ld.lld` and the LLVM binutils.

```
brew install nasm qemu llvm lld
```

Paths are auto-detected in `toolchain.mk` (override any variable on the `make`
command line if your setup differs). `mksfs`, the filesystem-image builder, is
compiled with the host compiler (`cc`).

## Build & run

```
make            # build build/os.img
make run        # boot it in QEMU (graphical window)
make run-serial # boot headless; the kernel log comes out on COM1 / the terminal
make debug      # boot paused with a QEMU gdb stub on :1234
make clean
```

Rebuilding regenerates `build/os.img` from scratch, which resets the on-disk
filesystem to its packed contents. Files created at runtime persist only as long
as you keep booting the same image.

### Debugging

`make debug` starts QEMU paused with a gdb stub on TCP 1234. Connect with a gdb
that can target i386 (`brew install i386-elf-gdb`):

```
i386-elf-gdb build/kernel.elf -ex 'target remote :1234' -ex 'break kmain' -ex continue
```

## Source layout

```
boot/            stage 1 + stage 2 bootloader (NASM)
kernel/
  main.c           entry point (kmain) — brings up every subsystem, then the shell
  shell.c          in-kernel shell / command dispatch
  arch/x86/        CPU + platform: GDT, IDT, ISRs, TSS, PIC, paging, ring-3
                   switch, port I/O, the E820 boot handoff, linker script
  mm/              bitmap frame allocator, paging setup, kmalloc, per-process
                   address spaces (vmm), the mm_init orchestrator
  drivers/         serial (COM1), PS/2 keyboard, ATA PIO disk, PIT timer
  fs/              VFS layer + SimpleFS (SFS1) implementation
  proc/            ELF loader, process/exec, syscall dispatch
  term/            VGA text console (scrolling terminal)
  lib/             freestanding string.h + kprintf
user/            freestanding libc, crt0, and user programs (hello, echo, cat)
tools/           mksfs — host tool that builds the SimpleFS image
fsroot/          plain files packed into the FS image alongside the user programs
```

Kernel headers are included path-qualified from `kernel/`, e.g.
`#include "mm/pmm.h"`. Object files mirror the source tree under `build/kernel/`.

## Disk image layout

`build/os.img` is a 16 MiB raw disk (attached as the primary IDE master).

| LBA (512-byte sectors) | Size        | Contents                                  |
|------------------------|-------------|-------------------------------------------|
| 0                      | 512 B       | Stage 1 (MBR)                             |
| 1 .. 8                 | ≤ 4 KiB     | Stage 2                                   |
| 9 .. 264               | ≤ 128 KiB   | Kernel (flat binary)                      |
| 2048 ..                | 8 MiB       | SimpleFS volume                          |

### Boot sequence

1. The BIOS loads **Stage 1** to `0x7C00`. It reads Stage 2 (LBA 1–8) to
   `0x0000:0x7E00` with INT 13h/AH=42h (LBA) and jumps to it in real mode.
2. **Stage 2** collects the BIOS memory map (INT 15h/E820) into a `bootinfo`
   block at physical `0x50000`, loads the kernel to `0x10000` in 64-sector
   chunks, enables A20 (port `0x92`), installs a flat GDT, sets `CR0.PE`, and
   far-jumps into 32-bit code.
3. The kernel entry stub zeroes `.bss`, sets up a stack, and calls `kmain`,
   which initializes: console + serial → GDT → TSS → IDT + PIC + syscall gate →
   PIT (100 Hz) → PMM + paging + kmalloc → keyboard → `sti` → ATA + filesystem →
   process subsystem → shell.

## Memory map

**Physical.** Stage 2 loads the kernel flat at `0x00010000`. The PMM is a
bitmap over all RAM reported usable by E820; the bitmap is placed immediately
after the kernel image. Low memory (`< 1 MiB`), the kernel image, and the bitmap
are reserved; everything else is free. Paging then identity-maps all RAM (capped
at 1 GiB) with 4 KiB pages and enables `CR0.PG`. `kmalloc` works out of a single
4 MiB physically-contiguous region (implicit free list, first-fit, coalescing).

**Per-process virtual.** Each process gets its own page directory that copies
the kernel's page-directory entries (`0..255`, the low 1 GiB) so kernel code and
data stay mapped, and adds user pages above 1 GiB:

| Region        | Address                         |
|---------------|---------------------------------|
| user image    | from `0x40000000` (ELFs linked here) |
| user stack    | `0x4FFFC000 .. 0x50000000` (16 KiB) |

The kernel is still identity-mapped low (no higher-half yet).

## System calls

`int 0x80`, with `eax` = call number, `ebx`/`ecx`/`edx` = arguments, result in
`eax`. User pointers are bounds-checked against the user region.

| # | name     | args                    | notes                              |
|---|----------|-------------------------|------------------------------------|
| 0 | `exit`   | `code`                  | tears down the process            |
| 1 | `write`  | `fd, buf, len`          | `fd` 1/2 → console + serial        |
| 2 | `read`   | `fd, buf, len`          | `fd` 0 → one line from the keyboard |
| 3 | `getpid` | –                       | returns 1 (single process for now) |
| 4 | `yield`  | –                       | no-op until there is a scheduler   |

## Filesystem — SimpleFS (SFS1)

A deliberately tiny format on the volume at LBA 2048:

- **block 0** — superblock (magic, block size, totals, layout).
- **blocks 1–8** — a flat directory table of 64-byte entries (64 files max, no
  subdirectories).
- **blocks 9+** — file data; each file is one contiguous run with a fixed
  capacity chosen at creation.

`fs/vfs.c` keeps a 16-entry open-file table over the single volume. Reads and
writes go straight to the disk via the polled ATA PIO driver; directory and
superblock changes are flushed back, so files created at runtime survive a
reboot of the same image. `tools/mksfs.c` packs `fsroot/*` plus the built user
programs into `build/fs.img` at build time.

## The shell

Runs in the kernel (for now) and reads lines from the keyboard driver.

| command | description |
|---------|-------------|
| `help` | list commands |
| `echo <text>` | print text |
| `clear` | clear the screen |
| `ticks` / `uptime` | time since boot |
| `mem` / `e820` | memory summary / BIOS memory map |
| `memtest` | exercise `kmalloc`/`kfree` |
| `ls` / `cat <f>` / `hexdump <f>` / `stat <f>` / `df` | filesystem inspection |
| `touch <f>` / `write <f> <text>` / `rm <f>` | filesystem mutation |
| `reboot` | reset via the 8042 controller |
| `<name> [args]` | load and run an ELF program of that name from the filesystem |

### Bundled user programs

Built from `user/` as `ET_EXEC` ELFs linked at `0x40000000`, packed into the FS
image. They talk to the kernel only through `int 0x80` (see `user/libc.c`).

| program | behavior |
|---------|----------|
| `hello` | prints `pid`, `argc`, and each `argv[i]`; exits 7 |
| `echo`  | prints its arguments |
| `cat`   | echoes stdin a line at a time until an empty line |

## Roadmap

- [x] **Step 1 — Boot to a screen.** Custom 2-stage bootloader, protected-mode
      entry, kernel prints to VGA text mode and COM1.
- [x] **Step 2 — Terminal.** Scrolling VGA console with a hardware cursor and a
      `kprintf` that also mirrors to serial.
- [x] **Step 3 — Interrupts.** Kernel-owned GDT, IDT with all 32 exception
      stubs, 8259 PIC remap, panic + register dump, PIT timer on IRQ0.
- [x] **Step 4 — Keyboard.** PS/2 driver on IRQ1 (scancode set 1, modifiers),
      lock-free ring buffer, line editing (Backspace, Ctrl-U, Ctrl-C).
- [x] **Step 5 — Memory management.** E820 parsing, bitmap physical frame
      allocator, 4 KiB paging, `kmalloc`/`kfree` with coalescing.
- [x] **Step 6 — Storage + FS.** Polled ATA PIO driver, SimpleFS (read **and**
      write, persisted), a VFS layer, and a host image builder.
- [x] **Step 7 — User mode.** TSS, ring-3 execution, `int 0x80` syscall gate,
      per-page user access control.
- [x] **Step 8 — ELF loader + processes.** Per-process page directories, an
      ELF32 loader, `argc`/`argv`, and synchronous `exec` from the filesystem.
      *(A scheduler and concurrent processes are deferred to Step 9.)*
- [ ] **Step 9 — Real shell + multitasking.** Filesystem syscalls
      (`open`/`read`/`write`/`readdir`), move the shell into a user ELF, a
      round-robin timer scheduler, `spawn`/`wait`.
- [ ] **Later.** `p_flags`-accurate segment permissions, `copy_from_user`
      validation, a higher-half kernel, demand paging / `sbrk`, and an x86_64
      port.

Each step is a self-contained, buildable commit.
