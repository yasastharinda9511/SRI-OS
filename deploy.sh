#!/bin/bash

#===========================================================================
# SriOS Deploy Script - Copy to SD Card (Updated for Pi Zero 2W Firmware)
#===========================================================================

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# SD card mount point (default)
SD_CARD="/run/media/yasas/5666-0AB1"

# Check if custom path passed
if [ -n "$1" ]; then
    SD_CARD="$1"
fi

echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}  SriOS Deploy Script${NC}"
echo -e "${GREEN}================================${NC}"

# Check SD card mount
if [ ! -d "$SD_CARD" ]; then
    echo -e "${RED}✗ SD card not found at: $SD_CARD${NC}"
    echo -e "${YELLOW}Usage: ./deploy.sh [/path/to/sdcard]${NC}"
    exit 1
fi

# Check kernel
if [ ! -f "kernel.img" ]; then
    echo -e "${RED}✗ kernel.img not found. Run ./build.sh first${NC}"
    exit 1
fi

#---------------------------------------------
# REQUIRED RASPBERRY PI FIRMWARE FILES
#---------------------------------------------
FILES=(
    "bootcode.bin"
    "start.elf"
    "fixup.dat"
)

echo -e "${YELLOW}Checking required firmware...${NC}"

for FILE in "${FILES[@]}"; do
    if [ ! -f "$SD_CARD/$FILE" ]; then
        echo -e "${YELLOW}Downloading $FILE...${NC}"
        wget -q -O "$SD_CARD/$FILE" \
            "https://github.com/raspberrypi/firmware/raw/master/boot/$FILE"
        echo -e "${GREEN}✓ Downloaded $FILE${NC}"
    else
        echo -e "${GREEN}✓ $FILE already exists${NC}"
    fi
done

#---------------------------------------------
# Copy kernel and config
#---------------------------------------------
echo -e "${YELLOW}Copying kernel.img...${NC}"
cp kernel.img "$SD_CARD/"
echo -e "${GREEN}✓ Copied kernel.img${NC}"

echo -e "${YELLOW}Copying config.txt...${NC}"
cp config.txt "$SD_CARD/"
echo -e "${GREEN}✓ Copied config.txt${NC}"

#---------------------------------------------
# Sync
#---------------------------------------------
echo -e "${YELLOW}Syncing...${NC}"
sync

# Show contents
echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}SD Card Contents:${NC}"
ls -lh "$SD_CARD"/* 2>/dev/null || true

echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}✓ Deploy complete!${NC}"
echo -e "${YELLOW}You can now safely eject the SD card${NC}"
