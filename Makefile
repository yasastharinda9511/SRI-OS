# Toolchain
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump

# Directories
BOOT_DIR = boot
KERNEL_DIR = kernel
DRIVERS_DIR = drivers
SHELL_DIR = shell
SCHEDULER_DIR = kernel/scheduler

# Flags
CFLAGS = -mcpu=arm1176jzf-s -O2 -ffreestanding -fno-pic -std=gnu99 -Wall -Wextra
CFLAGS += -I$(KERNEL_DIR) -I$(DRIVERS_DIR) -I$(SHELL_DIR) -I$(SCHEDULER_DIR)
ASFLAGS = -mcpu=arm1176jzf-s
LDFLAGS = -nostdlib -T linker.ld

# Object files
OBJS = $(BOOT_DIR)/boot.o \
       $(BOOT_DIR)/vectors.o \
       $(KERNEL_DIR)/kernel.o \
       $(KERNEL_DIR)/interrupts.o \
       $(DRIVERS_DIR)/uart.o \
       $(SHELL_DIR)/shell.o \
	   $(KERNEL_DIR)/fs.o \
	   $(SCHEDULER_DIR)/task.o \
	   $(SCHEDULER_DIR)/context.o

all: kernel.img

# Boot
$(BOOT_DIR)/boot.o: $(BOOT_DIR)/boot.S
	$(AS) $(ASFLAGS) $< -o $@

$(BOOT_DIR)/vectors.o: $(BOOT_DIR)/vectors.S
	$(AS) $(ASFLAGS) $< -o $@

# Kernel
$(KERNEL_DIR)/kernel.o: $(KERNEL_DIR)/kernel.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_DIR)/interrupts.o: $(KERNEL_DIR)/interrupts.c $(KERNEL_DIR)/interrupts.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_DIR)/fs.o: $(KERNEL_DIR)/fs.c $(KERNEL_DIR)/fs.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SCHEDULER_DIR)/task.o: $(SCHEDULER_DIR)/task.c $(SCHEDULER_DIR)/task.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SCHEDULER_DIR)/context.o: $(SCHEDULER_DIR)/context.S
	$(AS) $(ASFLAGS) $< -o $@

# Drivers
$(DRIVERS_DIR)/uart.o: $(DRIVERS_DIR)/uart.c $(DRIVERS_DIR)/uart.h
	$(CC) $(CFLAGS) -c $< -o $@

# Shell
$(SHELL_DIR)/shell.o: $(SHELL_DIR)/shell.c $(SHELL_DIR)/shell.h
	$(CC) $(CFLAGS) -c $< -o $@

# Linking
kernel.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o kernel.elf $(OBJS)

kernel.img: kernel.elf
	$(OBJCOPY) kernel.elf -O binary kernel.img

# Utilities
disasm: kernel.elf
	$(OBJDUMP) -d kernel.elf > kernel.disasm

qemu: kernel.elf
	qemu-system-arm -M raspi0 -serial stdio -kernel kernel.elf

qemu-debug: kernel.elf
	qemu-system-arm -M raspi0 -serial stdio -kernel kernel.elf -S -gdb tcp::1234

clean:
	rm -f $(BOOT_DIR)/*.o
	rm -f $(KERNEL_DIR)/*.o
	rm -f $(DRIVERS_DIR)/*.o
	rm -f $(SHELL_DIR)/*.o
	rm -f $(SCHEDULER_DIR)/*.o
	rm -f *.elf *.img *.disasm

.PHONY: all clean disasm qemu qemu-debug