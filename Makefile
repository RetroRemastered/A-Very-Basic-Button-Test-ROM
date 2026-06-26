TARGET := button_test
BUILD := build
SRC := src

PREFIX ?= arm-none-eabi-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
NODE ?= node

CFLAGS := -std=c99 -mthumb -mthumb-interwork -mcpu=arm7tdmi -O2 -Wall -Wextra -ffreestanding -fno-builtin -I$(SRC)
ASFLAGS := -marm -mcpu=arm7tdmi
LDFLAGS := -T gba.ld -nostdlib -Wl,--gc-sections

OBJS := $(BUILD)/start.o $(BUILD)/main.o

.PHONY: all clean

all: $(BUILD)/$(TARGET).gba

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/start.o: $(SRC)/start.s | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/main.o: $(SRC)/main.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS) gba.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(BUILD)/$(TARGET).gba: $(BUILD)/$(TARGET).elf tools/fix-gba-header.js
	$(OBJCOPY) -O binary $< $@
	$(NODE) tools/fix-gba-header.js $@

clean:
	rm -rf $(BUILD)
