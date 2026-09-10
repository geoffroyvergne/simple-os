# Toolchain configuration. Override any of these on the make command line.
#
#   make CC=/path/to/clang
#
# We cross-compile with clang targeting i686-elf; no GCC cross-compiler needed.

BREW_PREFIX := $(shell brew --prefix 2>/dev/null)
LLVM_BIN    := $(BREW_PREFIX)/opt/llvm/bin

CC      ?= $(LLVM_BIN)/clang
LD      ?= $(BREW_PREFIX)/bin/ld.lld
OBJCOPY ?= $(LLVM_BIN)/llvm-objcopy
OBJDUMP ?= $(LLVM_BIN)/llvm-objdump
NASM    ?= $(BREW_PREFIX)/bin/nasm
QEMU    ?= $(BREW_PREFIX)/bin/qemu-system-i386

# Fall back to bare names if the Homebrew paths are not present.
CC      := $(if $(wildcard $(CC)),$(CC),clang)
LD      := $(if $(wildcard $(LD)),$(LD),ld.lld)
OBJCOPY := $(if $(wildcard $(OBJCOPY)),$(OBJCOPY),llvm-objcopy)
OBJDUMP := $(if $(wildcard $(OBJDUMP)),$(OBJDUMP),llvm-objdump)
NASM    := $(if $(wildcard $(NASM)),$(NASM),nasm)
QEMU    := $(if $(wildcard $(QEMU)),$(QEMU),qemu-system-i386)
