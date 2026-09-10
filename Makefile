include toolchain.mk

BUILD := build

# ---- flags -----------------------------------------------------------------
CFLAGS := --target=i686-elf -march=i686 -std=gnu11 -ffreestanding -nostdlib \
          -O2 -g -Wall -Wextra -Wshadow -Wpointer-arith \
          -fno-pic -fno-pie -fno-stack-protector -fno-builtin \
          -fno-asynchronous-unwind-tables -fno-unwind-tables \
          -ffunction-sections -fdata-sections \
          -mno-sse -mno-mmx -mno-80387 \
          -Ikernel

LDFLAGS := -m elf_i386 -T kernel/linker.ld -nostdlib --gc-sections

# ---- sources -------------------------------------------------------------
KERNEL_C   := $(wildcard kernel/*.c)
KERNEL_ASM := $(wildcard kernel/*.asm)
KERNEL_OBJ := $(patsubst kernel/%.c,$(BUILD)/%.o,$(KERNEL_C)) \
              $(patsubst kernel/%.asm,$(BUILD)/%.o,$(KERNEL_ASM))

IMG := $(BUILD)/os.img

# ---- filesystem ---------------------------------------------------------
HOSTCC     ?= cc
FS_IMG     := $(BUILD)/fs.img
FS_SIZE    := 8388608           # 8 MiB SimpleFS volume
FS_LBA     := 2048             # must match SFS_DISK_LBA in kernel/sfs.h
FS_FILES   := $(wildcard fsroot/*)
IMG_SECTORS := 32768           # 16 MiB disk image

.PHONY: all run run-serial debug clean dirs

all: $(IMG)

dirs:
	@mkdir -p $(BUILD)

# ---- boot loader ----------------------------------------------------------
$(BUILD)/stage1.bin: boot/stage1.asm | dirs
	$(NASM) -f bin $< -o $@
	@test $$(stat -f%z $@) -eq 512 || { echo "stage1.bin must be 512 bytes"; exit 1; }

$(BUILD)/stage2.bin: boot/stage2.asm | dirs
	$(NASM) -f bin $< -o $@
	@test $$(stat -f%z $@) -le 4096 || { echo "stage2.bin exceeds 8 sectors"; exit 1; }

# ---- kernel -------------------------------------------------------------
$(BUILD)/%.o: kernel/%.asm | dirs
	$(NASM) -f elf32 $< -o $@

$(BUILD)/%.o: kernel/%.c | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/kernel.elf: $(KERNEL_OBJ) kernel/linker.ld
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJ)

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	@test $$(stat -f%z $@) -le 131072 || { echo "kernel.bin exceeds 256 sectors"; exit 1; }

# ---- filesystem image -------------------------------------------------
$(BUILD)/mksfs: tools/mksfs.c | dirs
	$(HOSTCC) -O2 -Wall -Wextra -o $@ $<

$(FS_IMG): $(BUILD)/mksfs $(FS_FILES)
	$(BUILD)/mksfs $@ $(FS_SIZE) $(FS_FILES)

# ---- disk image --------------------------------------------------------
$(IMG): $(BUILD)/stage1.bin $(BUILD)/stage2.bin $(BUILD)/kernel.bin $(FS_IMG)
	dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	dd if=$(BUILD)/stage1.bin of=$@ conv=notrunc bs=512 seek=0 status=none
	dd if=$(BUILD)/stage2.bin of=$@ conv=notrunc bs=512 seek=1 status=none
	dd if=$(BUILD)/kernel.bin  of=$@ conv=notrunc bs=512 seek=9 status=none
	dd if=$(FS_IMG)            of=$@ conv=notrunc bs=512 seek=$(FS_LBA) status=none
	@echo "built $@"

# ---- run / debug -----------------------------------------------------
QEMU_FLAGS := -drive format=raw,file=$(IMG),index=0,if=ide -no-reboot

run: $(IMG)
	$(QEMU) $(QEMU_FLAGS)

run-serial: $(IMG)
	$(QEMU) $(QEMU_FLAGS) -display none -serial stdio

debug: $(IMG)
	$(QEMU) $(QEMU_FLAGS) -serial stdio -s -S

clean:
	rm -rf $(BUILD)
