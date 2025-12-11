# Toolchain
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump

# Directories
BUILD_DIR = build
BOOT_DIR = boot
KERNEL_DIR = kernel
DRIVERS_DIR = drivers
SHELL_DIR = shell
SCHEDULER_DIR = kernel/scheduler

# Flags - Pi Zero 2W uses Cortex-A53
CFLAGS = -mcpu=cortex-a53 -O2 -ffreestanding -fno-pic -std=gnu11 -Wall -Wextra
CFLAGS += -I$(KERNEL_DIR) -I$(DRIVERS_DIR) -I$(SHELL_DIR) -I$(SCHEDULER_DIR)
ASFLAGS = -mcpu=cortex-a53
LDFLAGS = -nostdlib -T linker.ld

# Object files
OBJS = $(BUILD_DIR)/boot.o \
	   $(BUILD_DIR)/vectors.o \
       $(BUILD_DIR)/kernel.o \
       $(BUILD_DIR)/interrupts.o \
       $(BUILD_DIR)/uart.o \
       $(BUILD_DIR)/shell.o \
       $(BUILD_DIR)/fs.o \
       $(BUILD_DIR)/task.o \
       $(BUILD_DIR)/mutex.o \
       $(BUILD_DIR)/semaphore.o \
       $(BUILD_DIR)/spin_lock.o \
       $(BUILD_DIR)/context.o \
       $(BUILD_DIR)/gpio.o

all: $(BUILD_DIR) kernel.img

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Boot
$(BUILD_DIR)/boot.o: $(BOOT_DIR)/boot.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/vectors.o: $(BOOT_DIR)/vectors.S
	$(AS) $(ASFLAGS) $< -o $@

# Kernel
$(BUILD_DIR)/kernel.o: $(KERNEL_DIR)/kernel.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/interrupts.o: $(KERNEL_DIR)/interrupts/interrupts.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mutex.o: $(KERNEL_DIR)/sync/mutex.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semaphore.o: $(KERNEL_DIR)/sync/semaphore.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/spin_lock.o: $(KERNEL_DIR)/sync/spin_lock.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/fs.o: $(KERNEL_DIR)/fs.c
	$(CC) $(CFLAGS) -c $< -o $@

# Scheduler
$(BUILD_DIR)/task.o: $(KERNEL_DIR)/scheduler/task.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/context.o: $(KERNEL_DIR)/scheduler/context.S
	$(AS) $(ASFLAGS) $< -o $@

# Drivers
$(BUILD_DIR)/uart.o: $(DRIVERS_DIR)/uart/uart.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/gpio.o: $(DRIVERS_DIR)/gpio/gpio.c
	$(CC) $(CFLAGS) -c $< -o $@

# Shell
$(BUILD_DIR)/shell.o: $(SHELL_DIR)/shell.c
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
	qemu-system-arm -M raspi2b -serial stdio -kernel kernel.elf

qemu-debug: kernel.elf
	qemu-system-arm -M raspi2b -serial stdio -kernel kernel.elf -S -gdb tcp::1234

clean:
	rm -rf $(BUILD_DIR)
	rm -f *.elf *.img *.disasm

.PHONY: all clean disasm qemu qemu-debug