# Toolchain
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump

# Flags
CFLAGS = -mcpu=arm1176jzf-s -O2 -ffreestanding -fno-pic -std=gnu99 -Wall -Wextra
ASFLAGS = -mcpu=arm1176jzf-s
LDFLAGS = -nostdlib -T linker.ld

# Files
OBJS = boot.o vectors.o kernel.o uart.o interrupts.o

all: kernel.img

boot.o: boot.S
	$(AS) $(ASFLAGS) boot.S -o boot.o

vectors.o: vectors.S
	$(AS) $(ASFLAGS) vectors.S -o vectors.o

kernel.o: kernel.c uart.h interrupts.h
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

uart.o: uart.c uart.h
	$(CC) $(CFLAGS) -c uart.c -o uart.o

interrupts.o: interrupts.c interrupts.h uart.h
	$(CC) $(CFLAGS) -c interrupts.c -o interrupts.o

kernel.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o kernel.elf $(OBJS)

kernel.img: kernel.elf
	$(OBJCOPY) kernel.elf -O binary kernel.img

# Useful targets
disasm: kernel.elf
	$(OBJDUMP) -d kernel.elf > kernel.disasm

qemu: kernel.elf
	qemu-system-arm -M raspi0 -serial stdio -kernel kernel.elf

qemu-debug: kernel.elf
	qemu-system-arm -M raspi0 -serial stdio -kernel kernel.elf -S -gdb tcp::1234

clean:
	rm -f *.o *.elf *.img *.disasm

.PHONY: all clean disasm qemu qemu-debug