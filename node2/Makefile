LDSCRIPT = sam/flash.ld
BOOTUP = sam/sam3x/source/exceptions.c sam/sam3x/source/startup_sam3x.c sam/sam3x/source/system_sam3x.c
MCUTYPE = __SAM3X8E__
# List all source files (i.e. .c files) to be compiled; separate with whitespace.
# In particular, remember to add the UART and CAN starter files.
# Use relative file paths and a backslash before newline.
# Do NOT use a backslash after the last file.
SOURCE_FILES = \
	$(BOOTUP) \
	main.c \
	uart.c \
	can.c
#	../path_to/uart.c \
#   filename1.c \
#   filename2.c \
#	filenameN.c

# Feel free to ignore anything below this line
VPATH:= $(dir $(SOURCE_FILES))
BUILD_DIR := build
OBJS := $(patsubst %.c, $(BUILD_DIR)/%.o, $(notdir $(SOURCE_FILES)))
#Tools
CC:=arm-none-eabi-gcc
LD:=arm-none-eabi-gcc
AR:=arm-none-eabi-ar
AS:=arm-none-eabi-as
# Ubuntu ships the ARM debugger as gdb-multiarch; override with e.g. make debug GDB=arm-none-eabi-gdb
GDB ?= gdb-multiarch

ELF=$(BUILD_DIR)/main.elf

LDFLAGS:= -T$(LDSCRIPT) -mthumb -mcpu=cortex-m3 -Wl,--gc-sections
CFLAGS:= -mcpu=cortex-m3 -mthumb -g -std=c11 -MMD -MP
CFLAGS+= -I sam -I sam/sam3x/include -I sam/sam3x/source -I sam/cmsis -I .
CFLAGS+= -D $(MCUTYPE)

.DEFAULT_GOAL := $(ELF)
# compile and generate dependancy info
$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@
	#$(CC) -MM $(CFLAGS) $< > $@.d

$(BUILD_DIR)/%.o : %.s | $(BUILD_DIR)
	$(AS) $< -o $@

$(ELF) : $(OBJS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)


.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: debug
debug: $(ELF)
	if pgrep openocd; then killall -s 9 openocd; fi
	x-terminal-emulator -e openocd -f sam/openocd.cfg -c "program $(ELF) verify"
	sleep 5
# Kill openocd even if gdb fails, but still report gdb's exit status
	$(GDB) -tui -iex "target extended-remote localhost:3333" $(ELF); \
	status=$$?; killall -s 9 openocd; exit $$status

.PHONY: flash
flash: $(ELF)
	#./sam/write_bin.sh sam/openocd.cfg $(BUILD_DIR)/main.elf
	if pgrep openocd; then killall -s 9 openocd; fi
	openocd -f sam/openocd.cfg -c "program ${BUILD_DIR}/main.elf verify reset exit"


# pull in dependencies
-include	$(OBJS:.o=.d)

