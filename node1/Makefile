# List all source files to be compiled; separate with space
SOURCE_FILES := main.c uart.c sram.c xmem.c adc.c

# ADC clock on PD5: f = 4.9152 MHz / (2 * (1 + ADC_CLOCK_OCR)). MAX156 needs 0.5-5.0 MHz.
# Override on the command line, e.g. `make clean flash ADC_CLOCK_OCR=3` or `ADC_CLOCK=no`.
# Run `make clean` when changing these, objects do not depend on the flags.
ADC_CLOCK ?= yes
ADC_CLOCK_OCR ?= 1

# Set this flag to "yes" (no quotes) to use JTAG; otherwise ISP (SPI) is used
PROGRAM_WITH_JTAG := yes

# Feel free to ignore anything below this line
PROGRAMMER := atmelice_isp
ifeq ($(PROGRAM_WITH_JTAG), yes)
	PROGRAMMER := atmelice
endif

BUILD_DIR := build
TARGET_CPU := atmega162
TARGET_DEVICE := m162

CC := avr-gcc
CFLAGS := -O -std=c11 -mmcu=$(TARGET_CPU) -ggdb -Wall -Wextra -DF_CPU=4915200UL
# Generate .d files so editing a header rebuilds the objects that include it
DEPFLAGS := -MMD -MP

CFLAGS += -DADC_CLOCK_OCR=$(ADC_CLOCK_OCR)
ifneq ($(ADC_CLOCK), yes)
	CFLAGS += -DADC_CLOCK_DISABLED
endif

OBJECT_FILES = $(SOURCE_FILES:%.c=$(BUILD_DIR)/%.o)

.DEFAULT_GOAL := $(BUILD_DIR)/main.hex

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/main.hex: $(OBJECT_FILES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJECT_FILES) -o $(BUILD_DIR)/a.out
	avr-objcopy -j .text -j .data -O ihex $(BUILD_DIR)/a.out $(BUILD_DIR)/main.hex

.PHONY: flash
flash: $(BUILD_DIR)/main.hex
	avrdude -p $(TARGET_DEVICE) -c $(PROGRAMMER) -U flash:w:$(BUILD_DIR)/main.hex:i

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: erase
erase:
	avrdude -p $(TARGET_DEVICE) -c $(PROGRAMMER) -e
	
.PHONY: debug
debug:
	if pgrep avarice; then pkill avarice; fi
	avrdude -p $(TARGET_DEVICE) -c $(PROGRAMMER) -U flash:w:$(BUILD_DIR)/main.hex:i
	x-terminal-emulator -e avarice --edbg --ignore-intr :4242
	sleep 2
	avr-gdb -tui -iex "target remote localhost:4242" $(BUILD_DIR)/a.out
	killall -s 9 avarice

-include $(OBJECT_FILES:.o=.d)
