CC = gcc
NASM = nasm
OBJCOPY = objcopy

CFLAGS_BL = -mno-80387 -mno-mmx -mno-sse -mno-avx -mgeneral-regs-only -ffreestanding -fno-stack-protector -fno-ident -Wall -fno-pie -fno-pic -Wextra -m32

NASMFLAGS_BL_ASM = -f elf32

LDBL2FLAGS = -nostdlib --static -T $(BL_DIR)/linker-bls2.ld

BL_DIR = .
BUILD_DIR = path/to/build

SOURCES_BL_C = $(shell find $(BL_DIR) -name '*.c')
SOURCES_BL_ASM = $(shell find $(BL_DIR) -name '*.asm')

OBJS_BL_C = $(patsubst $(BL_DIR)/%.c, $(BUILD_DIR)/bl_c/%.o, $(SOURCES_BL_C))
OBJS_BL_ASM = $(patsubst $(BL_DIR)/%.asm, $(BUILD_DIR)/bl_asm/%.o, $(SOURCES_BL_ASM))
OBJS_BL = $(OBJS_BL_ASM) $(OBJS_BL_C)

STAGE2_ELF = $(BUILD_DIR)/stage2.elf
KERNEL_BIN = path/to/kernel/bin
DISK_IMG = $(BUILD_DIR)/os.iso
DISK_DIR = path/to/disk/dir

.PHONY: all clean run

all: $(DISK_IMG)

$(BUILD_DIR)/bl_c/%.o: $(BL_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_BL) -c $< -o $@

$(BUILD_DIR)/bl_asm/%.o: $(BL_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS_BL_ASM) $< -o $@

$(STAGE2_ELF): $(OBJS_BL) $(BL_DIR)/linker-bls2.ld
	ld -m elf_i386 $(LDBL2FLAGS) $(OBJS_BL) -o $@

$(DISK_IMG): $(STAGE2_ELF) $(KERNEL_BIN)
	cp $(STAGE2_ELF) $(DISK_DIR)/stage2.elf
	cp $(KERNEL_BIN) $(DISK_DIR)/kernel.bin
	grub-mkrescue -o $(DISK_IMG) $(DISK_DIR)

clean:
	rm -rf $(BUILD_DIR)
