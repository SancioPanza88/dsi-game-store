# Makefile for DSi Game Store
# Requires devkitPro with devkitARM, libnds, libfat, dswifi
#
# Build: make
# Clean: make clean
# Output: dsi-game-store.nds

# Project info
TARGET := dsi-game-store
TITLE := DSi Game Store
DESCRIPTION := Digital game store for Nintendo DSi

# Directories
SOURCE := source
INCLUDE := include
BUILD := build
DATA := data

# devkitPro paths
ifeq ($(strip $(DEVKITARM)),)
$(error "DEVKITARM environment variable not set. Please set it to your devkitARM path.")
endif

ifeq ($(strip $(DEVKITPRO)),)
$(error "DEVKITPRO environment variable not set. Please set it to your devkitPro path.")
endif

# Tools
CC := $(DEVKITARM)/bin/arm-none-eabi-gcc
CXX := $(DEVKITARM)/bin/arm-none-eabi-g++
AS := $(DEVKITARM)/bin/arm-none-eabi-as
LD := $(DEVKITARM)/bin/arm-none-eabi-gcc
OBJCOPY := $(DEVKITARM)/bin/arm-none-eabi-objcopy
NDSTOOL := $(DEVKITPRO)/tools/bin/ndstool
GRIT := $(DEVKITPRO)/tools/bin/grit
# ARM7 binary: auto-detect across devkitPro layouts; override with ARM7=...
ARM7 ?= $(shell ls $(DEVKITPRO)/libnds/nds7.elf $(DEVKITPRO)/libnds/bin/nds7.elf $(DEVKITPRO)/nds7.elf 2>/dev/null | head -n1)
ifeq ($(ARM7),)
$(error "nds7.elf not found under $(DEVKITPRO)/libnds. Set ARM7=/path/to/nds7.elf")
endif

# Flags
ARCH := -mthumb -mthumb-interwork
CFLAGS := $(ARCH) -O2 -Wall -Wno-unused -I$(INCLUDE) -I$(DEVKITPRO)/libnds/include
CXXFLAGS := $(CFLAGS) -fno-exceptions -fno-rtti
LDFLAGS := $(ARCH) -L$(DEVKITPRO)/libnds/lib -L$(DEVKITPRO)/portlibs/arm/lib
LIBS := -lnds9 -lfat -ldswifi9 -lz -lc -lm

# Source files
C_SOURCES := $(wildcard $(SOURCE)/*.c) \
             $(wildcard $(SOURCE)/ui/*.c) \
             $(wildcard $(SOURCE)/network/*.c) \
             $(wildcard $(SOURCE)/storage/*.c) \
             $(wildcard $(SOURCE)/catalog/*.c) \
             $(wildcard $(SOURCE)/download/*.c) \
             $(wildcard $(SOURCE)/common/*.c)

# Object files
OBJECTS := $(C_SOURCES:$(SOURCE)/%.c=$(BUILD)/%.o)

# Main target
all: $(TARGET).nds

# Create build directory
$(BUILD):
	mkdir -p $(BUILD)/ui $(BUILD)/network $(BUILD)/storage $(BUILD)/catalog $(BUILD)/download $(BUILD)/common

# Compile source files
$(BUILD)/%.o: $(SOURCE)/%.c | $(BUILD)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(BUILD)/$(TARGET).elf: $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

# Create NDS file
$(TARGET).nds: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $(BUILD)/$(TARGET).bin
	$(NDSTOOL) -c $@ -7 $(ARM7) -9 $(BUILD)/$(TARGET).elf \
		-b $(DATA)/banner.bin -t "$(TITLE)" -d "$(DESCRIPTION)"

# Clean
clean:
	rm -rf $(BUILD) $(TARGET).nds $(TARGET).bin

# Phony targets
.PHONY: all clean
